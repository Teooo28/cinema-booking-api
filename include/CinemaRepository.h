#pragma once

#include "Event.h"
#include <vector>
#include <memory>
#include "sqlite3.h"

class CinemaRepository {
private:
    std::vector<std::unique_ptr<Event>> schedule;

    sqlite3* db;    // pointer care tine legatura cu hard disk ul

    // functie privata ascunsa care creeaza tabelul la prima pornire
    void initDatabase();

public:
    CinemaRepository();  // deschide baza de date
    ~CinemaRepository(); // inchide baza de date la oprirea serverului

    void addEvent(std::unique_ptr<Event> event);
    const std::vector<std::unique_ptr<Event>>& getAllEvents() const;
    Event* getEventById(int id);

    void updateEvent(Event* event); // salveaza modificarea locurilor pe hard disk
};