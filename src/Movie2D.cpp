#include "Movie2D.h"

Movie2D::Movie2D(int id, const std::string& title, int duration, double basePrice, int availableSeats)
    : Event(id, title, duration, basePrice, availableSeats) {}

std::string Movie2D::getType() const {
    return "Movie2D";
}

nlohmann::json Movie2D::toJson(std::shared_ptr<DiscountStrategy> strategy) const {
    double finalPrice = this->getFinalPrice(strategy);

    return {
        {"id", id},
        {"title", title},
        {"duration", durationMinutes},
        {"basePrice", basePrice},
        {"finalPrice", finalPrice},
        {"discount", strategy->getStrategyName()},
        {"type", getType()},
        {"availableSeats", this->getAvailableSeats()}
    };
}