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
                throw InvalidDataException("Durata filmului nu poate fi negativa!");
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
        // 1. prindem erorile de date gresite
        catch (const InvalidDataException& e) {
            ApiResponse<std::string> errorResponse(false, "Date Invalide", e.what());
            crow::response res(errorResponse.toJson().dump());
            
            res.code = 400; // 400 = bad request
            res.add_header("Content-Type", "application/json");
            return res;
        }
        // 2. prindem erorile de tip 'nu am gasit filmul'
        catch (const EventNotFoundException& e) {
            ApiResponse<std::string> errorResponse(false, "Lipsa Date", e.what());
            crow::response res(errorResponse.toJson().dump());
            
            res.code = 404; // 404 = not found
            res.add_header("Content-Type", "application/json");
            return res;
        }
        // 3. o plasa de siguranta pentru orice alta eroare neprevazuta
        catch (const std::exception& e) {
            ApiResponse<std::string> errorResponse(false, "Eroare Interna", e.what());
            crow::response res(errorResponse.toJson().dump());
            
            res.code = 500; // 500 = internal server error
            res.add_header("Content-Type", "application/json");
            return res;
        }
    });

    std::cout << ">>> Serverul a pornit! Deschide un browser si acceseaza: http://localhost:8080/movies <<<" << std::endl;
    
    // pornim efectiv serverul pe portul 8080
    app.port(8080).multithreaded().run();

    return 0;
}