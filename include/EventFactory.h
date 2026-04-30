#pragma once

#include <memory>
#include <string>
#include "Event.h"
#include "Movie2D.h"
#include "Movie3D.h"

class EventFactory {
public:
    static std::unique_ptr<Event> createEvent(
        const std::string& type, 
        int id, 
        const std::string& title, 
        int duration, 
        double basePrice, 
        int availableSeats,
        double specialFee = 0.0
    );
};