#pragma once

#include "Event.h"
#include "User.h"
#include <vector>
#include <memory>
#include "sqlite3.h"

class CinemaRepository {
private:
    std::vector<std::unique_ptr<Event>> schedule;
    
    // Pointer to the active SQLite database connection
    sqlite3* db;    

    // Initializes database schema (tables) if they do not exist at startup
    void initDatabase();

public:
    CinemaRepository();
    ~CinemaRepository();

    // Event management methods
    void addEvent(std::unique_ptr<Event> event);
    const std::vector<std::unique_ptr<Event>>& getAllEvents() const;
    Event* getEventById(int id);
    void updateEvent(Event* event); 

    // User management methods
    void addUser(const User& user);
    std::unique_ptr<User> getUserByUsername(const std::string& username);
};