#include <iostream>
#include <vector>
#include <memory> 
#include "Movie2D.h"
#include "Movie3D.h"

int main() {
    // Creăm o listă de "Evenimente" (folosind clasa de bază)
    // Deși lista e de Event, ea va conține obiecte de tip Movie2D și Movie3D
    std::vector<std::unique_ptr<Event>> schedule;

    schedule.push_back(std::make_unique<Movie2D>(1, "Avatar 2D", 180, 30.0));
    schedule.push_back(std::make_unique<Movie3D>(2, "Avatar 3D", 180, 30.0, 10.0));

    for (const auto& event : schedule) {
        // AICI e polimorfismul: deși apelăm toJson() pe un Event*, 
        // se va executa funcția corectă (cea din 2D sau 3D)
        std::cout << event->toJson().dump(4) << std::endl;
        std::cout << "----------------------" << std::endl;
    }

    return 0;
}