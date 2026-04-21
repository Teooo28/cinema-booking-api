#include <iostream>
#include <vector>
#include <memory>
#include "crow.h" 
#include "EventFactory.h"
#include "DiscountStrategy.h"
#include "ApiResponse.h"

int main() {
    // initializam aplicatie web (serverul)
    crow::SimpleApp app;

    // definim ruta
    CROW_ROUTE(app, "/movies")([]() {
        
        std::vector<std::unique_ptr<Event>> schedule;

        // generam filme
        auto movie1 = EventFactory::createEvent("2D", 1, "Interstellar", 169, 30.0);
        auto movie2 = EventFactory::createEvent("3D", 2, "Avatar", 192, 30.0, 10.0);
        movie2->setDiscountStrategy(std::make_shared<StudentDiscount>());

        schedule.push_back(std::move(movie1));
        schedule.push_back(std::move(movie2));

        // punem datele filmelor intr-o lista JSON
        nlohmann::json moviesArray = nlohmann::json::array();
        for (const auto& event : schedule) {
            moviesArray.push_back(event->toJson());
        }

        ApiResponse<nlohmann::json> apiResponse(true, "Filmele au fost preluate cu succes", moviesArray);

        // transformam raspunsul in text si ii spunem browserului ca e un JSON
        crow::response res(apiResponse.toJson().dump());
        res.add_header("Content-Type", "application/json");
        return res;
    });

    std::cout << ">>> Serverul a pornit! Deschide un browser si acceseaza: http://localhost:8080/movies <<<" << std::endl;
    
    // pornim efectiv serverul pe portul 8080
    app.port(8080).multithreaded().run();

    return 0;
}