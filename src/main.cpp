#include <iostream>
#include <vector>
#include "EventFactory.h"

int main() {
    std::vector<std::unique_ptr<Event>> schedule;

    schedule.push_back(EventFactory::createEvent("2D", 1, "Interstellar", 169, 25.0));
    schedule.push_back(EventFactory::createEvent("3D", 2, "Avatar: The Way of Water", 192, 35.0, 15.0));

    for (const auto& event : schedule) {
        std::cout << event->toJson().dump(4) << std::endl;
        std::cout << "----------------------" << std::endl;
    }

    return 0;
}