#include "Movie3D.h"

Movie3D::Movie3D(int id, const std::string& title, int duration, double basePrice, int availableSeats, double fee)
    : Event(id, title, duration, basePrice, availableSeats), specialFee(fee) {}

std::string Movie3D::getType() const {
    return "Movie3D";
}
// polimorfism - suprascriere functie virtuala din clasa de baza
double Movie3D::getFinalPrice() const {
    return Event::getFinalPrice() + specialFee; 
}

nlohmann::json Movie3D::toJson() const {
    double finalPrice = this->getFinalPrice();

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