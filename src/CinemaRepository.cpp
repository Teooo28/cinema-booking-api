#include "CinemaRepository.h"

void CinemaRepository::addEvent(std::unique_ptr<Event> event) {
    schedule.push_back(std::move(event));
}

// returnam o referinta ca sa nu copiem toata lista in memorie
const std::vector<std::unique_ptr<Event>>& CinemaRepository::getAllEvents() const {
    return schedule;
}

// cautam filmul si returnam pointer la el
Event* CinemaRepository::getEventById(int id) {
    for (const auto& event : schedule) {
        if (event->getId() == id) {
            return event.get();
        }
    }
    // daca nu gasim nimic returnam null
    return nullptr;
}