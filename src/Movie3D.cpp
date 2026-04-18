#include "Movie3D.h"

Movie3D::Movie3D(int id, const std::string& title, int duration, double basePrice, double fee)
    : Event(id, title, duration, basePrice), specialFee(fee) {}

std::string Movie3D::getType() const {
    return "Movie3D";
}

nlohmann::json Movie3D::toJson() const {
    return {
        {"id", id},
        {"title", title},
        {"duration", durationMinutes},
        {"basePrice", basePrice},
        {"specialFee", specialFee},
        {"totalPrice", basePrice + specialFee},
        {"type", getType()}
    };
}