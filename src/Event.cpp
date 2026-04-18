#include "Event.h"

Event::Event(int id, const std::string& title, int duration, double basePrice)
    : id(id), title(title), durationMinutes(duration), basePrice(basePrice) {
    
    this->discountStrategy = std::make_shared<NoDiscount>();
}

Event::~Event() {
}

void Event::setDiscountStrategy(std::shared_ptr<DiscountStrategy> strategy) {
    if (strategy) {
        this->discountStrategy = strategy;
    }
}

int Event::getId() const {
    return id;
}

std::string Event::getTitle() const {
    return title;
}

double Event::getBasePrice() const {
    return basePrice;
}