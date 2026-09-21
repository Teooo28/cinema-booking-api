#include "MovieController.h"
#include "EventFactory.h"

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
    
    // POST /movies - Add a new movie to the catalog (ADMIN ONLY)
    CROW_ROUTE(app, "/movies").methods(crow::HTTPMethod::POST)([&repo](const crow::request& req) {
        try {
            // JWT Middleware & Authentication
            std::string authHeader = req.get_header_value("Authorization");
            if (authHeader.empty() || authHeader.substr(0, 7) != "Bearer ") {
                throw UnauthorizedException("Missing or invalid Authorization header!");
            }

            std::string tokenString = authHeader.substr(7);
            auto decodedToken = jwt::decode(tokenString);

            try {
                auto verifier = jwt::verify()
                    .allow_algorithm(jwt::algorithm::hs256{"SUPER_SECRET_KEY_123"})
                    .with_issuer("cinema_api");
                verifier.verify(decodedToken);
            } catch (const std::exception& e) {
                throw UnauthorizedException("Invalid or expired token!");
            }

            // Role-Based Access Control (RBAC) Verification
            std::string userRole = decodedToken.get_payload_claim("role").as_string();
            if (userRole != "admin") {
                throw UnauthorizedException("Access denied! Administrator privileges required.");
            }

            // Request Payload Parsing & Validation
            auto body = nlohmann::json::parse(req.body);
            
            if (!body.contains("type") || !body.contains("id") || !body.contains("title") || 
                !body.contains("duration") || !body.contains("basePrice") || !body.contains("availableSeats")) {
                throw InvalidDataException("Incomplete data. Required fields: type, id, title, duration, basePrice, availableSeats.");
            }

            std::string type = body["type"];
            int id = body["id"];
            std::string title = body["title"];
            int duration = body["duration"];
            double basePrice = body["basePrice"];
            int availableSeats = body["availableSeats"];
            
            double glassesPrice = 0.0;
            if (type == "3D" && body.contains("glassesPrice")) {
                glassesPrice = body["glassesPrice"];
            }

            repo.addEvent(EventFactory::createEvent(type, id, title, duration, basePrice, availableSeats, glassesPrice));
            
            ApiResponse<std::string> apiResponse(true, "Resource Created", "Successfully added '" + title + "' to the catalog.");
            crow::response res(apiResponse.toJson().dump());
            res.code = 201;
            res.add_header("Content-Type", "application/json");
            return res;

        } 
        catch (const UnauthorizedException& e) {
            ApiResponse<std::string> errorResponse(false, "Authorization Failed", e.what());
            crow::response res(errorResponse.toJson().dump());
            res.code = 401; 
            res.add_header("Content-Type", "application/json");
            return res;
        }
        catch (const InvalidDataException& e) {
            ApiResponse<std::string> errorResponse(false, "Validation Error", e.what());
            crow::response res(errorResponse.toJson().dump());
            res.code = 400; 
            res.add_header("Content-Type", "application/json");
            return res;
        } 
        catch (const nlohmann::json::exception& e) {
            ApiResponse<std::string> errorResponse(false, "Malformed JSON", "Please ensure the request body is valid JSON.");
            crow::response res(errorResponse.toJson().dump());
            res.code = 400;
            res.add_header("Content-Type", "application/json");
            return res;
        }
        catch (const std::exception& e) {
            ApiResponse<std::string> errorResponse(false, "Internal Server Error", "An unexpected error occurred.");
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
            int adultTickets = 0;
            int studentTickets = 0;

            if (body["tickets"].contains("adult")) {
                adultTickets = body["tickets"]["adult"];
            }
            if (body["tickets"].contains("student")) {
                studentTickets = body["tickets"]["student"];
            }
            int totalRequestedTickets = adultTickets + studentTickets;
            if (totalRequestedTickets <= 0) {
                throw InvalidDataException("You must select at least one ticket to proceed!");
            }

            Event* targetMovie = repo.getEventById(movieId);
            if (!targetMovie) {
                throw EventNotFoundException("Movie with the provided ID does not exist!");
            }

            auto adultStrategy = std::make_shared<NoDiscount>();
            auto studentStrategy = std::make_shared<StudentDiscount>();

            double adultPriceTotal = targetMovie->getFinalPrice(adultStrategy) * adultTickets;
            double studentPriceTotal = targetMovie->getFinalPrice(studentStrategy) * studentTickets;

            double totalPayment = adultPriceTotal + studentPriceTotal;

            // Critical section: lock concurrent access to guarantee transaction consistency (RAM + DB + History)
            std::lock_guard<std::mutex> lock(reservationsMutex);
            
            targetMovie->bookSeats(totalRequestedTickets);
            repo.updateEvent(targetMovie); 

            int randomNum = rand() % 9000 + 1000;
            std::string reservationCode = "#TKT-" + std::to_string(randomNum);
            activeReservations[reservationCode] = {movieId, totalRequestedTickets};

            std::stringstream priceStream;
            priceStream << std::fixed << std::setprecision(2) << totalPayment;
            std::string ticketWord = (totalRequestedTickets == 1) ? " ticket" : " tickets";
            std::string message = "You have successfully booked " + std::to_string(totalRequestedTickets) + ticketWord +
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