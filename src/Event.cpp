#include "Event.h"
#include "Exceptions.h"

int Event::totalEventsCreated = 0;

Event::Event(int id, const std::string& title, int duration, double basePrice, int availableSeats)
    : id(id), title(title), durationMinutes(duration), basePrice(basePrice), availableSeats(availableSeats) {
    
    // Track global instances for potential analytics integration
    totalEventsCreated++;
}

Event::~Event() {}

void Event::bookSeats(int count) {
    if (count <= 0) {
        throw InvalidDataException("You must book at least one ticket!");
    }
    if (count > MAX_TICKETS_PER_TRANSACTION) {
        throw InvalidDataException("You can book a maximum of 10 tickets per transaction!");
    }
    if (count > availableSeats) {
        throw InvalidDataException("Capacity exceeded: Not enough available seats!");
    }
    
    // In-memory update (requires external synchronization like Mutex in concurrent environments)
    availableSeats -= count;
}

int Event::getTotalEventsCreated() {
    return totalEventsCreated;
}

int Event::getAvailableSeats() const {
    return availableSeats;
}

double Event::getFinalPrice(std::shared_ptr<DiscountStrategy> strategy) const {
    // Price calculation using Dependency Injection for the discount strategy
    return strategy->calculatePrice(basePrice);
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

void Event::setAvailableSeats(int seats) {
    this->availableSeats = seats;
}