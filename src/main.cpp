#include <iostream>
#include <memory>
#include <sstream>
#include <iomanip> 
#include <unordered_map>
#include "crow.h" 
#include "EventFactory.h"
#include "DiscountStrategy.h"
#include "ApiResponse.h"
#include "Exceptions.h"
#include "CinemaRepository.h"
#include <mutex>

int main() {
    crow::SimpleApp app;
    CinemaRepository repo;

    // In-memory registry: Cod Rezervare -> {ID_Film, Numar_Bilete}
    std::unordered_map<std::string, std::pair<int, int>> rezervariActive;
    std::mutex rezervariMutex;
    
    // Populare baza de date cu evenimente initiale de test
    repo.addEvent(EventFactory::createEvent("2D", 1, "Interstellar", 169, 30.0, 100, 0.0));
    repo.addEvent(EventFactory::createEvent("3D", 2, "Avatar", 192, 40.0, 50, 15.0));

    // GET /movies - Fetch all events in schedule
    CROW_ROUTE(app, "/movies").methods(crow::HTTPMethod::GET)([&repo]() {
        try {
            nlohmann::json moviesArray = nlohmann::json::array();
            
            for (const auto& event : repo.getAllEvents()) {
                moviesArray.push_back(event->toJson());
            }

            ApiResponse<nlohmann::json> apiResponse(true, "Filme gasite", moviesArray);
            crow::response res(apiResponse.toJson().dump());
            res.add_header("Content-Type", "application/json");
            return res;
        } 
        catch (const std::exception& e) {
            ApiResponse<std::string> errorResponse(false, "Eroare Interna", e.what());
            crow::response res(errorResponse.toJson().dump());
            res.code = 500; 
            res.add_header("Content-Type", "application/json");
            return res;
        }
    });

    // POST /movies/<id>/book - Process ticket reservation
    CROW_ROUTE(app, "/movies/<int>/book").methods(crow::HTTPMethod::POST)([&repo, &rezervariActive, &rezervariMutex](const crow::request& req, int idFilm) {
        try {
            auto body = nlohmann::json::parse(req.body);

            if (!body.contains("bilete")) {
                throw InvalidDataException("Trebuie sa trimiti numarul de bilete!");
            }
            int bileteDorite = body["bilete"];

            Event* filmGasit = repo.getEventById(idFilm);
            if (!filmGasit) {
                throw EventNotFoundException("Filmul cu acest ID nu exista!");
            }

            // Injectare dinamica a strategiei de reducere (Business logic)
            if (body.contains("status") && body["status"] == "student") {
                filmGasit->setDiscountStrategy(std::make_shared<StudentDiscount>());
            } else {
                filmGasit->setDiscountStrategy(std::make_shared<NoDiscount>()); 
            }

            // Sectiune critica: blocam accesul concurent pentru a garanta
            // consistenta intregii tranzactii (RAM + DB + istoric)
            std::lock_guard<std::mutex> lock(rezervariMutex);
            
            filmGasit->bookSeats(bileteDorite);
            repo.updateEvent(filmGasit); 

            int randomNum = rand() % 9000 + 1000;
            std::string codRezervare = "#TKT-" + std::to_string(randomNum);
            rezervariActive[codRezervare] = {idFilm, bileteDorite};

            // Calcul final aplicand Design Pattern-ul Strategy
            double totalPlata = filmGasit->getFinalPrice() * bileteDorite;

            std::stringstream streamPret;
            streamPret << std::fixed << std::setprecision(2) << totalPlata;
            std::string mesaj = "Ai rezervat " + std::to_string(bileteDorite) + (bileteDorite == 1 ? " bilet" : " bilete") +
                                ". Total de plata: " + streamPret.str() + " RON. Cod intrare: " + codRezervare;

            ApiResponse<std::string> apiResponse(true, "Rezervare Confirmata", mesaj);
            crow::response res(apiResponse.toJson().dump());
            res.add_header("Content-Type", "application/json");
            return res;

        } 
        catch (const InvalidDataException& e) {
            ApiResponse<std::string> errorResponse(false, "Eroare Date", e.what());
            crow::response res(errorResponse.toJson().dump());
            res.code = 400; 
            res.add_header("Content-Type", "application/json");
            return res;
        } 
        catch (const EventNotFoundException& e) {
            ApiResponse<std::string> errorResponse(false, "Eroare Cautare", e.what());
            crow::response res(errorResponse.toJson().dump());
            res.code = 404; 
            res.add_header("Content-Type", "application/json");
            return res;
        }
        catch (const nlohmann::json::exception& e) {
            ApiResponse<std::string> errorResponse(false, "JSON Invalid", "Te rog verifica formatul JSON");
            crow::response res(errorResponse.toJson().dump());
            res.code = 400;
            res.add_header("Content-Type", "application/json");
            return res;
        }
    });

    // DELETE /cancel - Cancel reservation and refund seats
    CROW_ROUTE(app, "/cancel").methods(crow::HTTPMethod::Delete)([&repo, &rezervariActive, &rezervariMutex](const crow::request& req) {
        try {
            auto body = nlohmann::json::parse(req.body);
            if (!body.contains("cod_intrare")) {
                throw InvalidDataException("Trebuie sa introduci codul de rezervare!");
            }
            std::string cod = body["cod_intrare"];

            // Sectiune critica: sincronizam cautarea chitantei si refund-ul locurilor
            std::lock_guard<std::mutex> lock(rezervariMutex);
            
            if (rezervariActive.find(cod) == rezervariActive.end()) {
                throw EventNotFoundException("Cod invalid sau rezervarea a fost deja anulata!");
            }

            int idFilm = rezervariActive[cod].first;
            int bileteDeAnulat = rezervariActive[cod].second;

            Event* filmGasit = repo.getEventById(idFilm);
            
            // Refund efectiv al locurilor in RAM si DB
            filmGasit->setAvailableSeats(filmGasit->getAvailableSeats() + bileteDeAnulat);
            repo.updateEvent(filmGasit); 

            rezervariActive.erase(cod);

            ApiResponse<std::string> apiResponse(true, "Anulare reusita", 
                "Au fost returnate " + std::to_string(bileteDeAnulat) + " locuri pentru codul " + cod);
            
            crow::response res(apiResponse.toJson().dump());
            res.add_header("Content-Type", "application/json");
            return res;

        } catch (const std::exception& e) {
            ApiResponse<std::string> errorResponse(false, "Eroare Anulare", e.what());
            crow::response res(errorResponse.toJson().dump());
            res.code = 400; 
            res.add_header("Content-Type", "application/json");
            return res;
        }
    });

    std::cout << ">>> Serverul a pornit pe portul 8080 <<<" << std::endl;
    app.port(8080).multithreaded().run();

    return 0;
}