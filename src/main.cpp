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

int main() {
    // initializare framework web Crow (motorul care asculta cererile HTTP)
    crow::SimpleApp app;

    // instantiem clasa Repository
    CinemaRepository repo;

    // registru ce retine Codul de intrare -> {ID_Film, Numar_Bilete}
    std::unordered_map<std::string, std::pair<int, int>> rezervariActive;
    
    // bagam cateva filme de test
    repo.addEvent(EventFactory::createEvent("2D", 1, "Interstellar", 169, 30.0, 100, 0.0));
    repo.addEvent(EventFactory::createEvent("3D", 2, "Avatar", 192, 40.0, 50, 15.0));

    // RUTA 1: citeste filmele (GET)
    // punem [&repo] ca sa aiba voie sa foloseasca lista de mai sus
    CROW_ROUTE(app, "/movies").methods(crow::HTTPMethod::GET)([&repo]() {
        
        try {
            // construim un array JSON gol pentru a stoca lista de evenimente
            nlohmann::json moviesArray = nlohmann::json::array();

            // iteram prin toate evenimentele si apelam metoda polimorfica toJson()
            for (const auto& event : repo.getAllEvents()) {
                moviesArray.push_back(event->toJson());
            }

            // impachetam datele folosind clasa template ApiResponse
            ApiResponse<nlohmann::json> apiResponse(true, "Filme gasite", moviesArray);

            // generam raspunsul HTTP si ii setam header ul corect
            crow::response res(apiResponse.toJson().dump());
            res.add_header("Content-Type", "application/json");
            return res;
        } 
        catch (const std::exception& e) {
            ApiResponse<std::string> errorResponse(false, "Eroare Interna", e.what());
            crow::response res(errorResponse.toJson().dump());
            
            res.code = 500; // 500 = internal server error
            res.add_header("Content-Type", "application/json");
            return res;
        }
    });

    // RUTA 2: rezervare bilete (POST)
    // parametrul <int> din ruta este extras automat de Crow in variabila 'idFilm'.
    // 'req' contine payload-ul (corpul) trimis de client prin internet
    CROW_ROUTE(app, "/movies/<int>/book").methods(crow::HTTPMethod::POST)([&repo, &rezervariActive](const crow::request& req, int idFilm) {
        try {
            // parsam textul venit de pe internet intr-un obiect de tip JSON
            auto body = nlohmann::json::parse(req.body);

            // validam prezenta cheii obligatorii
            if (!body.contains("bilete")) {
                throw InvalidDataException("Trebuie sa trimiti numarul de bilete!");
            }
            int bileteDorite = body["bilete"];

            // cautam pointer ul catre instanta filmului in memorie
            Event* filmGasit = repo.getEventById(idFilm);

            // verificam daca utilizatorul are un status special (ex: student)
            if (body.contains("status") && body["status"] == "student") {
                // injectam strategia de reducere in film
                filmGasit->setDiscountStrategy(std::make_shared<StudentDiscount>());
            } else {
                // ne asiguram ca filmul are strategia standard (fara reducere)
                filmGasit->setDiscountStrategy(std::make_shared<NoDiscount>()); 
            }

            // daca filmul nu exista aruncam exceptia custom
            if (!filmGasit) {
                throw EventNotFoundException("Filmul cu acest ID nu exista!");
            }

            // facem actiunea de rezervare (aici pica in eroare 400 daca nu sunt destule locuri)
            filmGasit->bookSeats(bileteDorite);
            
            // dupa ce modificarea s a facut in RAM sincronizam datele pe hard disk (SQLite)
            repo.updateEvent(filmGasit);

            // logica pentru gramatica corecta
            std::string cuvantBilet = (bileteDorite == 1) ? " bilet" : " bilete";

            // generarea unui cod de rezervare random (ex: #TKT-4829)
            int randomNum = rand() % 9000 + 1000; // Genereaza un numar intre 1000 si 9999
            std::string codRezervare = "#TKT-" + std::to_string(randomNum);
            // salvam chitanta in registru ca sa stim ce a cumparat
            rezervariActive[codRezervare] = {idFilm, bileteDorite};

            // calculam nota de plata 
            double totalPlata = filmGasit->getFinalPrice() * bileteDorite;

            // formatam pretul sa aiba fix 2 zecimale
            std::stringstream streamPret;
            streamPret << std::fixed << std::setprecision(2) << totalPlata;
            std::string pretFormatat = streamPret.str();

            std::string mesaj = "Ai rezervat " + std::to_string(bileteDorite) + cuvantBilet +
                                ". Total de plata: " + pretFormatat + " RON. Cod intrare: " + codRezervare;

            // trimitem nota de plata clientului
            ApiResponse<std::string> apiResponse(true, "Rezervare Confirmata", mesaj);

            crow::response res(apiResponse.toJson().dump());
            res.add_header("Content-Type", "application/json");
            return res;

        } 
        // prindem erorile noastre custom si dam codul corect (400 sau 404)
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
        // daca a trimis un json invalid
        catch (const nlohmann::json::exception& e) {
            ApiResponse<std::string> errorResponse(false, "JSON Invalid", "Te rog verifica formatul JSON");
            crow::response res(errorResponse.toJson().dump());
            res.code = 400;
            res.add_header("Content-Type", "application/json");
            return res;
        }
    });

    // RUTA 3: Anularea unei rezervari (Metoda DELETE)
    // clientul trimite JSON cu cate bilete vrea sa returneze: {"bilete_anulate": 2}
    CROW_ROUTE(app, "/cancel").methods(crow::HTTPMethod::Delete)([&repo, &rezervariActive](const crow::request& req) {
        try {
            auto body = nlohmann::json::parse(req.body);
            if (!body.contains("cod_intrare")) {
                throw InvalidDataException("Trebuie sa introduci codul de rezervare!");
            }
            std::string cod = body["cod_intrare"];

            // verificam daca exista chitanta in registru
            if (rezervariActive.find(cod) == rezervariActive.end()) {
                throw EventNotFoundException("Cod invalid sau rezervarea a fost deja anulata!");
            }

            // citim datele de pe chitanta
            int idFilm = rezervariActive[cod].first;
            int bileteDeAnulat = rezervariActive[cod].second;

            // cautam filmul si dam locurile inapoi
            Event* filmGasit = repo.getEventById(idFilm);
            int locuriCurente = filmGasit->getAvailableSeats();
            filmGasit->setAvailableSeats(locuriCurente + bileteDeAnulat);
            repo.updateEvent(filmGasit); // salvam in SQLite

            // distrugem chitanta
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


    std::cout << ">>> Serverul a pornit! Deschide un browser si acceseaza: http://localhost:8080/movies <<<" << std::endl;
    
    // pornim efectiv serverul pe portul 8080
    // functia multithreaded() permite procesarea cererilor HTTP in paralel
    app.port(8080).multithreaded().run();

    return 0;
}