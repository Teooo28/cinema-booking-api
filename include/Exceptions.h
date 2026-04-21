#pragma once

#include <stdexcept>
#include <string>

// exceptie pentru date invalide (ex: pret negativ, durata imposibila)
class InvalidDataException : public std::runtime_error {
public:
    InvalidDataException(const std::string& message) 
        : std::runtime_error(message) {}
};

// exceptie pentru cautari esuate (ex: cautam filmul cu id-ul 99 si nu exista)
class EventNotFoundException : public std::runtime_error {
public:
    EventNotFoundException(const std::string& message) 
        : std::runtime_error(message) {}
};