#include "EventFactory.h"
#include <stdexcept>

std::unique_ptr<Event> EventFactory::createEvent(
    const std::string& type, int id, const std::string& title, int duration, double basePrice, double specialFee) {
    
    if (type == "2D") {
        return std::make_unique<Movie2D>(id, title, duration, basePrice);
    } 
    else if (type == "3D") {
        return std::make_unique<Movie3D>(id, title, duration, basePrice, specialFee);
    } 
    else {
        throw std::invalid_argument("Eroare: Tip de eveniment necunoscut!");
    }
}