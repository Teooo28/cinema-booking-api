#include "Movie3D.h"

Movie3D::Movie3D(int id, const std::string& title, int duration, double basePrice, int availableSeats, double fee)
    : Event(id, title, duration, basePrice, availableSeats), specialFee(fee) {}

std::string Movie3D::getType() const {
    return "Movie3D";
}

double Movie3D::getFinalPrice(std::shared_ptr<DiscountStrategy> strategy) const {
    // Polymorphic override: calculates base price using strategy and appends the 3D specific fee
    return Event::getFinalPrice(strategy) + specialFee; 
}

nlohmann::json Movie3D::toJson(std::shared_ptr<DiscountStrategy> strategy) const {
    double finalPrice = this->getFinalPrice(strategy);

    return {
        {"id", id},
        {"title", title},
        {"duration", durationMinutes},
        {"basePrice", basePrice},
        {"specialFee", specialFee},
        {"finalPrice", finalPrice},
        {"discount", strategy->getStrategyName()},
        {"type", getType()},
        {"availableSeats", this->getAvailableSeats()}
    };
}