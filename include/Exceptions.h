#pragma once
#include <stdexcept>
#include <string>

// Exception thrown when incoming data (JSON payload) fails validation
class InvalidDataException : public std::runtime_error {
public:
    explicit InvalidDataException(const std::string& message)
        : std::runtime_error(message) {}
};

// Exception thrown when a requested entity cannot be found in the database or memory
class EventNotFoundException : public std::runtime_error {
public:
    explicit EventNotFoundException(const std::string& message)
        : std::runtime_error(message) {}
};

// Exception thrown when authentication fails or a user lacks required permissions
class UnauthorizedException : public std::runtime_error {
public:
    explicit UnauthorizedException(const std::string& message)
        : std::runtime_error(message) {}
};