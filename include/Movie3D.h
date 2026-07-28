#pragma once

#include "Event.h"

class Movie3D : public Event {
private:
    double specialFee;

public:
    Movie3D(int id, const std::string& title, int duration, double basePrice, int availableSeats, double fee);

    nlohmann::json toJson(std::shared_ptr<DiscountStrategy> strategy) const override;
    std::string getType() const override;

    double getFinalPrice(std::shared_ptr<DiscountStrategy> strategy) const override;
};