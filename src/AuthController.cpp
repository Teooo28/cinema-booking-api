#include "AuthController.h"

void AuthController::registerRoutes(crow::SimpleApp& app, CinemaRepository& repo) {
    
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

}