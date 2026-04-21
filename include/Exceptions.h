#pragma once

#include <exception>
#include <string>

class InvalidEventException : public std::exception {
private:
    std::string message;

public:
    InvalidEventException(const std::string& message) : message(message) {}

    const char* what() const noexcept override {
        return message.c_str();
    }
};