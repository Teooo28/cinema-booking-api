#pragma once

#include <string>
#include <nlohmann/json.hpp>

class Event {
protected:
    int id;
    std::string title;
    int durationMinutes;
    double basePrice;

public:
    Event(int id, const std::string& title, int durationMinutes, double basePrice);
    virtual ~Event();

    int getId() const;
    std::string getTitle() const;
    double getBasePrice() const;

    virtual nlohmann::json toJson() const = 0; 
    virtual std::string getType() const = 0;
};
