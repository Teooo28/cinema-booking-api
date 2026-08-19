#include "MovieController.h"

void MovieController::registerRoutes(
    crow::SimpleApp& app, 
    CinemaRepository& repo, 
    std::unordered_map<std::string, std::pair<int, int>>& activeReservations, 
    std::mutex& reservationsMutex) 
{
    // GET /movies - Fetch all events in schedule
    CROW_ROUTE(app, "/movies").methods(crow::HTTPMethod::GET)([&repo]() {
        try {
            nlohmann::json moviesArray = nlohmann::json::array();
            
            // Inject a default standard strategy for public catalog viewing
            auto defaultStrategy = std::make_shared<NoDiscount>();

            for (const auto& event : repo.getAllEvents()) {
                moviesArray.push_back(event->toJson(defaultStrategy));
            }

            ApiResponse<nlohmann::json> apiResponse(true, "Movies retrieved successfully", moviesArray);
            crow::response res(apiResponse.toJson().dump());
            res.add_header("Content-Type", "application/json");
            return res;
        } 
        catch (const std::exception& e) {
            ApiResponse<std::string> errorResponse(false, "Internal Server Error", e.what());
            crow::response res(errorResponse.toJson().dump());
            res.code = 500; 
            res.add_header("Content-Type", "application/json");
            return res;
        }
    });
    

    // POST /movies/<id>/book - Process ticket reservation
    CROW_ROUTE(app, "/movies/<int>/book").methods(crow::HTTPMethod::POST)([&repo, &activeReservations, &reservationsMutex](const crow::request& req, int movieId) {
        try {
            // JWT Authentication Middleware
            std::string authHeader = req.get_header_value("Authorization");
            if (authHeader.empty() || authHeader.substr(0, 7) != "Bearer ") {
                throw UnauthorizedException("Missing or invalid Authorization header!");
            }

            std::string tokenString = authHeader.substr(7);
            try {
                auto decodedToken = jwt::decode(tokenString);
                auto verifier = jwt::verify()
                    .allow_algorithm(jwt::algorithm::hs256{"SUPER_SECRET_KEY_123"})
                    .with_issuer("cinema_api");
                
                verifier.verify(decodedToken);
            } 
            catch (const std::exception& e) {
                throw UnauthorizedException("Invalid or expired token!");
            }

            auto body = nlohmann::json::parse(req.body);

            if (!body.contains("tickets")) {
                throw InvalidDataException("You must provide the number of tickets!");
            }
            int requestedTickets = body["tickets"];

            Event* targetMovie = repo.getEventById(movieId);
            if (!targetMovie) {
                throw EventNotFoundException("Movie with the provided ID does not exist!");
            }

            // Determine the business logic strategy dynamically based on user status
            std::shared_ptr<DiscountStrategy> appliedStrategy;
            if (body.contains("status") && body["status"] == "student") {
                appliedStrategy = std::make_shared<StudentDiscount>();
            } else {
                appliedStrategy = std::make_shared<NoDiscount>(); 
            }

            // Critical section: lock concurrent access to guarantee transaction consistency (RAM + DB + History)
            std::lock_guard<std::mutex> lock(reservationsMutex);
            
            targetMovie->bookSeats(requestedTickets);
            repo.updateEvent(targetMovie); 

            int randomNum = rand() % 9000 + 1000;
            std::string reservationCode = "#TKT-" + std::to_string(randomNum);
            activeReservations[reservationCode] = {movieId, requestedTickets};

            // Final calculation applying the Strategy Design Pattern dynamically
            double totalPayment = targetMovie->getFinalPrice(appliedStrategy) * requestedTickets;

            std::stringstream priceStream;
            priceStream << std::fixed << std::setprecision(2) << totalPayment;
            std::string ticketWord = (requestedTickets == 1) ? " ticket" : " tickets";
            std::string message = "You have successfully booked " + std::to_string(requestedTickets) + ticketWord +
                                  ". Total payment: " + priceStream.str() + " RON. Entry code: " + reservationCode;

            ApiResponse<std::string> apiResponse(true, "Booking Confirmed", message);
            crow::response res(apiResponse.toJson().dump());
            res.add_header("Content-Type", "application/json");
            return res;

        } 
        catch (const UnauthorizedException& e) {
            ApiResponse<std::string> errorResponse(false, "Authentication Failed", e.what());
            crow::response res(errorResponse.toJson().dump());
            res.code = 401; 
            res.add_header("Content-Type", "application/json");
            return res;
        }
        catch (const InvalidDataException& e) {
            ApiResponse<std::string> errorResponse(false, "Data Error", e.what());
            crow::response res(errorResponse.toJson().dump());
            res.code = 400; 
            res.add_header("Content-Type", "application/json");
            return res;
        } 
        catch (const EventNotFoundException& e) {
            ApiResponse<std::string> errorResponse(false, "Search Error", e.what());
            crow::response res(errorResponse.toJson().dump());
            res.code = 404; 
            res.add_header("Content-Type", "application/json");
            return res;
        }
        catch (const nlohmann::json::exception& e) {
            ApiResponse<std::string> errorResponse(false, "Invalid JSON", "Please verify the JSON format.");
            crow::response res(errorResponse.toJson().dump());
            res.code = 400;
            res.add_header("Content-Type", "application/json");
            return res;
        }
    });


    // DELETE /cancel - Cancel reservation and refund seats
    CROW_ROUTE(app, "/cancel").methods(crow::HTTPMethod::Delete)([&repo, &activeReservations, &reservationsMutex](const crow::request& req) {
        try {
            // JWT Authentication Middleware
            std::string authHeader = req.get_header_value("Authorization");
            if (authHeader.empty() || authHeader.substr(0, 7) != "Bearer ") {
                throw UnauthorizedException("Missing or invalid Authorization header!");
            }

            std::string tokenString = authHeader.substr(7);
            try {
                auto decodedToken = jwt::decode(tokenString);
                auto verifier = jwt::verify()
                    .allow_algorithm(jwt::algorithm::hs256{"SUPER_SECRET_KEY_123"})
                    .with_issuer("cinema_api");
                
                verifier.verify(decodedToken);
            } 
            catch (const std::exception& e) {
                throw UnauthorizedException("Invalid or expired token!");
            }

            auto body = nlohmann::json::parse(req.body);
            if (!body.contains("reservation_code")) {
                throw InvalidDataException("You must provide the reservation code!");
            }
            std::string code = body["reservation_code"];

            // Critical section: synchronize receipt lookup and seat refunding
            std::lock_guard<std::mutex> lock(reservationsMutex);
            
            if (activeReservations.find(code) == activeReservations.end()) {
                throw EventNotFoundException("Invalid code or reservation already cancelled!");
            }

            int movieId = activeReservations[code].first;
            int ticketsToCancel = activeReservations[code].second;

            Event* targetMovie = repo.getEventById(movieId);
            
            // Refund seats in memory and database
            targetMovie->setAvailableSeats(targetMovie->getAvailableSeats() + ticketsToCancel);
            repo.updateEvent(targetMovie); 

            activeReservations.erase(code);

            ApiResponse<std::string> apiResponse(true, "Cancellation successful", 
                "Successfully refunded " + std::to_string(ticketsToCancel) + " seats for code " + code);
            
            crow::response res(apiResponse.toJson().dump());
            res.add_header("Content-Type", "application/json");
            return res;

        } 
        catch (const UnauthorizedException& e) {
            ApiResponse<std::string> errorResponse(false, "Authentication Failed", e.what());
            crow::response res(errorResponse.toJson().dump());
            res.code = 401; 
            res.add_header("Content-Type", "application/json");
            return res;
        }
        catch (const EventNotFoundException& e) {
            ApiResponse<std::string> errorResponse(false, "Cancellation Error", e.what());
            crow::response res(errorResponse.toJson().dump());
            res.code = 404; 
            res.add_header("Content-Type", "application/json");
            return res;
        }
        catch (const std::exception& e) {
            ApiResponse<std::string> errorResponse(false, "Internal Error", e.what());
            crow::response res(errorResponse.toJson().dump());
            res.code = 400; 
            res.add_header("Content-Type", "application/json");
            return res;
        }
    });
}