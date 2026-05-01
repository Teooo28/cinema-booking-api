#pragma once

#include "Event.h"
#include <vector>
#include <memory>

class CinemaRepository {
private:
    std::vector<std::unique_ptr<Event>> schedule;

public:
    void addEvent(std::unique_ptr<Event> event);
    const std::vector<std::unique_ptr<Event>>& getAllEvents() const;
    Event* getEventById(int id);
};