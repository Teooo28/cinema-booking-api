#pragma once

#include "Event.h"

class Movie3D : public Event {
private:
    double specialFee;

public:
    Movie3D(int id, const std::string& title, int duration, double basePrice, double fee);

    nlohmann::json toJson() const override;
    std::string getType() const override;
};