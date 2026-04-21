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

    std::shared_ptr<DiscountStrategy> discountStrategy;

public:
    Event(int id, const std::string& title, int durationMinutes, double basePrice);
    virtual ~Event();

    void setDiscountStrategy(std::shared_ptr<DiscountStrategy> strategy);

    double getFinalPrice() const;

    int getId() const;
    std::string getTitle() const;
    double getBasePrice() const;

    virtual nlohmann::json toJson() const = 0; 
    virtual std::string getType() const = 0;
};
