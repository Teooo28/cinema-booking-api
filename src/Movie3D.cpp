#include "Movie3D.h"

Movie3D::Movie3D(int id, const std::string& title, int duration, double basePrice, int availableSeats, double fee)
    : Event(id, title, duration, basePrice, availableSeats), specialFee(fee) {}

std::string Movie3D::getType() const {
    return "Movie3D";
}

nlohmann::json Movie3D::toJson() const {
    // luam pretul cu reducere din parinte si adaugam taxa 3d
    double finalPrice = this->getFinalPrice() + specialFee;

    return {
        {"id", id},
        {"title", title},
        {"duration", durationMinutes},
        {"basePrice", basePrice},
        {"specialFee", specialFee},
        {"finalPrice", finalPrice},
        {"discount", discountStrategy->getStrategyName()},
        {"type", getType()},
        {"availableSeats", this->getAvailableSeats()}
    };
}