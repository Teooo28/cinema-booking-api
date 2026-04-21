#include "Event.h"

Event::Event(int id, const std::string& title, int duration, double basePrice)
    : id(id), title(title), durationMinutes(duration), basePrice(basePrice) {
    
    // setam o strategie default pentru a evita erori de tip null pointer
    this->discountStrategy = std::make_shared<NoDiscount>();
}

Event::~Event() {
}

void Event::setDiscountStrategy(std::shared_ptr<DiscountStrategy> strategy) {

    // verificam sa nu primim o strategie goala/invalida
    if (strategy) {
        this->discountStrategy = strategy;
    }
}

// calculam pretul aici o singura data ca sa nu repetam codul in toate clasele copil
double Event::getFinalPrice() const {
    return discountStrategy->calculatePrice(basePrice);
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