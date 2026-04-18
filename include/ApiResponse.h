#pragma once

#include <string>
#include <iostream>
#include <nlohmann/json.hpp>

template <typename T>
class ApiResponse {
private:
    bool success;
    std::string message;
    T data;

public:
    ApiResponse(bool success, const std::string& message, T data)
        : success(success), message(message), data(data) {}

    nlohmann::json toJson() const {
        return {
            {"success", success},
            {"message", message},
            {"data", data} 
        };
    }
    
    void print() const {
        std::cout << toJson().dump(4) << std::endl;
    }
};