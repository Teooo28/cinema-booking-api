#include <iostream>
#include <vector>
#include "EventFactory.h"
#include "DiscountStrategy.h"

int main() {
    std::vector<std::unique_ptr<Event>> schedule;

    // Creăm filmele prin Factory
    auto movie1 = EventFactory::createEvent("2D", 1, "Interstellar", 169, 30.0);
    auto movie2 = EventFactory::createEvent("3D", 2, "Avatar", 192, 30.0, 10.0);

    // APLICĂM STRATEGIA! (Facem Avatar-ul să aibă reducere de student)
    movie2->setDiscountStrategy(std::make_shared<StudentDiscount>());

    schedule.push_back(std::move(movie1));
    schedule.push_back(std::move(movie2));

    for (const auto& event : schedule) {
        std::cout << event->toJson().dump(4) << std::endl;
        std::cout << "----------------------" << std::endl;
    }

    return 0;
}