#include "DiscountStrategy.h"

double NoDiscount::calculatePrice(double basePrice) const {
    return basePrice;
}

std::string NoDiscount::getStrategyName() const {
    return "Standard";
}

double StudentDiscount::calculatePrice(double basePrice) const {
    return basePrice * 0.5; // 50% reducere
}

std::string StudentDiscount::getStrategyName() const {
    return "Student (-50%)";
}