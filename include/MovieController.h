#pragma once
#include "crow.h"
#include "CinemaRepository.h"
#include "Exceptions.h"
#include "ApiResponse.h"
#include "DiscountStrategy.h"
#include <jwt-cpp/jwt.h>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <sstream>
#include <iomanip>

class MovieController {
public:
    static void registerRoutes(
        crow::SimpleApp& app, 
        CinemaRepository& repo, 
        std::unordered_map<std::string, std::pair<int, int>>& activeReservations, 
        std::mutex& reservationsMutex
    );
};