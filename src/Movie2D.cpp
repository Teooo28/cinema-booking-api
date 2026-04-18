#include "Movie2D.h"

Movie2D::Movie2D(int id, const std::string& title, int duration, double basePrice)
    : Event(id, title, duration, basePrice) {}

std::string Movie2D::getType() const {
    return "Movie2D";
}

nlohmann::json Movie2D::toJson() const {
    return {
        {"id", id},
        {"title", title},
        {"duration", durationMinutes},
        {"basePrice", basePrice},
        {"type", getType()}
    };
}