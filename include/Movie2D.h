#pragma once
#include "Event.h"

class Movie2D : public Event {
public:
    Movie2D(int id, const std::string& title, int duration, double basePrice, int availableSeats);

    nlohmann::json toJson() const override;
    std::string getType() const override;
};