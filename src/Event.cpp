#include "Event.h"
#include "Exceptions.h"

int Event::totalEventsCreated = 0;

Event::Event(int id, const std::string& title, int duration, double basePrice, int availableSeats)
    : id(id), title(title), durationMinutes(duration), basePrice(basePrice), availableSeats(availableSeats) {
    
    // setam o strategie default pentru a evita erori de tip null pointer
    this->discountStrategy = std::make_shared<NoDiscount>();
    // crestem contorul de fiecare data cand se fabrica un film nou
    totalEventsCreated++;
}

Event::~Event() {
}

void Event::setDiscountStrategy(std::shared_ptr<DiscountStrategy> strategy) {

    // verificam sa nu primim o strategie goala/invalida
    if (strategy) {
        this->discountStrategy = strategy;
    }
}

void Event::bookSeats(int count) {
    if (count <= 0) {
        throw InvalidDataException("Trebuie sa rezervi cel putin un bilat!");
    }
    if (count > MAX_TICKETS_PER_TRANSACTION) {
            throw InvalidDataException("Poti cumpara maxim 10 bilete odata!");
    }
    if (count > availableSeats) {
        throw InvalidDataException("Nu sunt destule locuri disponibile in sala!");
    }
    // scadem locurile din sala
    availableSeats -= count;
}

int Event::getTotalEventsCreated() {
    return totalEventsCreated;
}

int Event::getAvailableSeats() const {
    return availableSeats;
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