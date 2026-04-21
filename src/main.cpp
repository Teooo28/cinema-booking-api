#include <iostream>
#include <vector>
#include <memory>
#include "crow.h" 
#include "EventFactory.h"
#include "DiscountStrategy.h"
#include "ApiResponse.h"
#include "Exceptions.h"

int main() {
    // initializam aplicatia web (serverul)
    crow::SimpleApp app;

    // definim ruta
    CROW_ROUTE(app, "/movies")([]() {
        
        try {
            std::vector<std::unique_ptr<Event>> schedule;

            // FORTAM O EROARE
            int durataGresita = -10;
            
            if (durataGresita < 0) {
                throw InvalidEventException("Durata filmului nu poate fi negativa!");
            }

            auto movie1 = EventFactory::createEvent("2D", 1, "Interstellar", durataGresita, 30.0);
            schedule.push_back(std::move(movie1));

            nlohmann::json moviesArray = nlohmann::json::array();
            for (const auto& event : schedule) {
                moviesArray.push_back(event->toJson());
            }

            ApiResponse<nlohmann::json> apiResponse(true, "Filme gasite", moviesArray);
            crow::response res(apiResponse.toJson().dump());
            res.add_header("Content-Type", "application/json");
            return res;
        } 
        catch (const InvalidEventException& e) {
            
            ApiResponse<std::string> errorResponse(false, "Eroare de Validare", e.what());
            
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