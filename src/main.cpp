#include <iostream>
#include <vector>
#include <memory>
#include "crow.h" 
#include "EventFactory.h"
#include "DiscountStrategy.h"
#include "ApiResponse.h"
#include "Exceptions.h"
#include "CinemaRepository.h"

int main() {
    // initializam aplicatia web (serverul)
    crow::SimpleApp app;

    // instantiem clasa de depozitare
    CinemaRepository repo;
    
    // bagam cateva filme de test
    repo.addEvent(EventFactory::createEvent("2D", 1, "Interstellar", 169, 30.0, 100, 0.0));
    repo.addEvent(EventFactory::createEvent("3D", 2, "Avatar", 192, 40.0, 50, 15.0));

    // RUTA 1: citeste filmele (GET)
    // punem [&schedule] ca sa aiba voie sa foloseasca lista de mai sus
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
            
            res.code = 500; // 500 = internal server error
            res.add_header("Content-Type", "application/json");
            return res;
        }
    });

    // RUTA 2: cumpara bilete (POST)
    // ruta primeste req (datele din internet) si idFilm (numarul din link)
    CROW_ROUTE(app, "/movies/<int>/book").methods(crow::HTTPMethod::POST)([&repo](const crow::request& req, int idFilm) {
        try {
            // citim json-ul trimis de client ca sa vedem cate bilete vrea
            auto body = nlohmann::json::parse(req.body);
            if (!body.contains("bilete")) {
                throw InvalidDataException("Trebuie sa trimiti numarul de bilete!");
            }
            int bileteDorite = body["bilete"];

            // cautam filmul in lista
            Event* filmGasit = repo.getEventById(idFilm);

            // daca nu l-am gasit, dam eroare 404
            if (!filmGasit) {
                throw EventNotFoundException("Filmul cu acest ID nu exista!");
            }

            // facem actiunea (aici pica in eroare 400 daca nu sunt destule locuri)
            filmGasit->bookSeats(bileteDorite);

            // confirmam succesul pe net
            ApiResponse<std::string> apiResponse(true, "Succes", "Ai rezervat biletele cu succes!");
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

    std::cout << ">>> Serverul a pornit! Deschide un browser si acceseaza: http://localhost:8080/movies <<<" << std::endl;
    
    // pornim efectiv serverul pe portul 8080
    app.port(8080).multithreaded().run();

    return 0;
}