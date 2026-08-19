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
#include "picosha2.h"
#include "User.h"
#include <jwt-cpp/jwt.h>

int main() {
    crow::SimpleApp app;
    CinemaRepository repo;

    // In-memory registry: Reservation Code -> {Movie_ID, Ticket_Count}
    std::unordered_map<std::string, std::pair<int, int>> activeReservations;
    std::mutex reservationsMutex;
    
    // Populate the database with initial test events
    repo.addEvent(EventFactory::createEvent("2D", 1, "Interstellar", 169, 30.0, 100, 0.0));
    repo.addEvent(EventFactory::createEvent("3D", 2, "Avatar", 192, 40.0, 50, 15.0));

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

    // POST /register - Create a new client account
    CROW_ROUTE(app, "/register").methods(crow::HTTPMethod::POST)([&repo](const crow::request& req) {
        try {
            auto body = nlohmann::json::parse(req.body);

            if (!body.contains("username")) {
                throw InvalidDataException("You must provide your username!");
            }
            if (!body.contains("password")) {
                throw InvalidDataException("You must provide your password!");
            }

            std::string username = body["username"];
            std::string rawPassword = body["password"];

            // Uniqueness Check
            if (repo.getUserByUsername(username) != nullptr) {
                throw InvalidDataException("Username already exists!");
            }

            // Password Hashing (Security)
            std::string hashedPassword = picosha2::hash256_hex_string(rawPassword);
            
            User newUser(username, hashedPassword, "client");
            repo.addUser(newUser);

            ApiResponse<std::string> apiResponse(true, "Registration complete", "You have successfully registered!");
            crow::response res(apiResponse.toJson().dump());
            res.code = 201;
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
        catch (const nlohmann::json::exception& e) {
            ApiResponse<std::string> errorResponse(false, "Invalid JSON", "Please verify the JSON format.");
            crow::response res(errorResponse.toJson().dump());
            res.code = 400;
            res.add_header("Content-Type", "application/json");
            return res;
        }
    });

    // POST /login - Authenticate user and issue JWT
    CROW_ROUTE(app, "/login").methods(crow::HTTPMethod::POST)([&repo](const crow::request& req) {
        try {
            auto body = nlohmann::json::parse(req.body);

            if (!body.contains("username") || !body.contains("password")) {
                throw InvalidDataException("You must provide both username and password!");
            }

            std::string username = body["username"];
            std::string rawPasswordFromLogin = body["password"];

            std::unique_ptr<User> foundUser = repo.getUserByUsername(username);

            // Security Check (Generic error for both missing user and bad password)
            if (!foundUser || picosha2::hash256_hex_string(rawPasswordFromLogin) != foundUser->getPasswordHash()) {
                throw UnauthorizedException("Invalid username or password!");
            }
            
            // Generate JWT
            auto token = jwt::create()
                .set_issuer("cinema_api")
                .set_type("JWS")
                .set_payload_claim("id", jwt::claim(std::to_string(foundUser->getId())))
                .set_payload_claim("username", jwt::claim(foundUser->getUsername()))
                .set_payload_claim("role", jwt::claim(foundUser->getRole()))
                .set_expires_at(std::chrono::system_clock::now() + std::chrono::hours(24))
                .sign(jwt::algorithm::hs256("SUPER_SECRET_KEY_123")); // Cryptographic signature

            // Return Token to Client
            nlohmann::json responseBody;
            responseBody["message"] = "Login successful";
            responseBody["token"] = token;

            crow::response res(responseBody.dump());
            res.code = 200;
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
        catch (const UnauthorizedException& e) {
            ApiResponse<std::string> authError(false, "Authentication Failed", e.what());
            crow::response res(authError.toJson().dump());
            res.code = 401; 
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

        } catch (const std::exception& e) {
            ApiResponse<std::string> errorResponse(false, "Cancellation Error", e.what());
            crow::response res(errorResponse.toJson().dump());
            res.code = 400; 
            res.add_header("Content-Type", "application/json");
            return res;
        }
    });

    std::cout << ">>> Server is running on port 8080 <<<" << std::endl;
    app.port(8080).multithreaded().run();

    return 0;
}