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

    std::shared_ptr<DiscountStrategy> discountStrategy;

public:
    static const int MAX_TICKETS_PER_TRANSACTION = 10;

    Event(int id, const std::string& title, int durationMinutes, double basePrice, int availableSeats);
    virtual ~Event();

    void setDiscountStrategy(std::shared_ptr<DiscountStrategy> strategy);

    void bookSeats(int count);

    static int getTotalEventsCreated();

    virtual double getFinalPrice() const;
    int getAvailableSeats() const;

    int getId() const;
    std::string getTitle() const;
    double getBasePrice() const;

    virtual nlohmann::json toJson() const = 0; 
    virtual std::string getType() const = 0;

    void setAvailableSeats(int seats); // ca sa poata baza de date sa suprascrie locurile
};
