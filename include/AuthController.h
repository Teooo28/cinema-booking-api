#pragma once
#include "crow.h"
#include "CinemaRepository.h"
#include "Exceptions.h"
#include "ApiResponse.h"
#include "picosha2.h"
#include <jwt-cpp/jwt.h>

class AuthController {
public:
    static void registerRoutes(crow::SimpleApp& app, CinemaRepository& repo);
};