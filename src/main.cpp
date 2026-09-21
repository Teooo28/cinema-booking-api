#include <iostream>
#include <unordered_map>
#include <mutex>
#include "crow.h" 
#include "CinemaRepository.h"
#include "EventFactory.h"
#include "AuthController.h"
#include "MovieController.h"

int main() {
    crow::SimpleApp app;
    CinemaRepository repo;

    // In-memory registry: Reservation Code -> {Movie_ID, Ticket_Count}
    std::unordered_map<std::string, std::pair<int, int>> activeReservations;
    std::mutex reservationsMutex;

    // Delegate routing logic to Controllers (MVC Pattern)
    AuthController::registerRoutes(app, repo);
    MovieController::registerRoutes(app, repo, activeReservations, reservationsMutex);

    std::cout << ">>> Server is running on port 8080 <<<" << std::endl;
    app.port(8080).multithreaded().run();

    return 0;
}