#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include <memory>
#include "DiscountStrategy.h"

class Event {
protected:
    int id;
    std::string title;
    int durationMinutes;
    double basePrice;
    int availableSeats;

    static int totalEventsCreated;

public:
    static const int MAX_TICKETS_PER_TRANSACTION = 10;

    Event(int id, const std::string& title, int durationMinutes, double basePrice, int availableSeats);
    virtual ~Event();

    void bookSeats(int count);

    static int getTotalEventsCreated();

    virtual double getFinalPrice(std::shared_ptr<DiscountStrategy> strategy) const;
    int getAvailableSeats() const;

    int getId() const;
    std::string getTitle() const;
    double getBasePrice() const;

    virtual nlohmann::json toJson(std::shared_ptr<DiscountStrategy> strategy) const = 0; 
    virtual std::string getType() const = 0;

    // Synchronize in-memory state with the persistence layer (e.g., SQLite)
    void setAvailableSeats(int seats); 
};