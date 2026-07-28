#include "CinemaRepository.h"
#include <iostream>

CinemaRepository::CinemaRepository() {
    // Initializare conexiune cu baza de date locala
    int rc = sqlite3_open("cinema.db", &db); 
    if (rc) {
        std::cerr << "[DB ERROR] Failed to open database: " << sqlite3_errmsg(db) << "\n";
    } else {
        initDatabase();
    }
}

CinemaRepository::~CinemaRepository() {
    // Asiguram eliberarea resurselor la inchiderea aplicatiei (RAII concept)
    sqlite3_close(db);
}

void CinemaRepository::initDatabase() {
    // Schema bazei de date: stocam doar starea volatila (locurile ramase)
    const char* sql = "CREATE TABLE IF NOT EXISTS seats_inventory ("
                      "event_id INTEGER PRIMARY KEY, "
                      "available_seats INTEGER);";
    
    char* errMsg = nullptr;
    sqlite3_exec(db, sql, nullptr, nullptr, &errMsg); 
    
    if (errMsg) {
        std::cerr << "[DB ERROR] Schema initialization failed: " << errMsg << "\n";
        sqlite3_free(errMsg);   
    }
}

void CinemaRepository::addEvent(std::unique_ptr<Event> event) {
    int currentId = event->getId();
    bool foundInDb = false;
    
    // Verificam persistenta datelor pentru a sincroniza memoria RAM cu discul
    const char* sqlCheck = "SELECT available_seats FROM seats_inventory WHERE event_id = ?;";
    sqlite3_stmt* stmtCheck; 
    
    if (sqlite3_prepare_v2(db, sqlCheck, -1, &stmtCheck, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmtCheck, 1, currentId);
        
        if (sqlite3_step(stmtCheck) == SQLITE_ROW) {  
            int savedSeats = sqlite3_column_int(stmtCheck, 0);
            event->setAvailableSeats(savedSeats);
            foundInDb = true;
        }
    }
    sqlite3_finalize(stmtCheck); 

    // Inregistram evenimentul in baza de date daca este la prima rulare
    if (!foundInDb) {
        const char* sqlInsert = "INSERT INTO seats_inventory (event_id, available_seats) VALUES (?, ?);";
        sqlite3_stmt* stmtInsert;
        
        if (sqlite3_prepare_v2(db, sqlInsert, -1, &stmtInsert, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(stmtInsert, 1, currentId);
            sqlite3_bind_int(stmtInsert, 2, event->getAvailableSeats());
            sqlite3_step(stmtInsert);
        }
        sqlite3_finalize(stmtInsert);
    }

    schedule.push_back(std::move(event));
}

void CinemaRepository::updateEvent(Event* event) {
    // Tranzactie securizata impotriva SQL Injection (Prepared Statement)
    const char* sqlUpdate = "UPDATE seats_inventory SET available_seats = ? WHERE event_id = ?;";
    sqlite3_stmt* stmt;

    int rc = sqlite3_prepare_v2(db, sqlUpdate, -1, &stmt, nullptr);

    if (rc == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, event->getAvailableSeats());
        sqlite3_bind_int(stmt, 2, event->getId());
        
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            std::cerr << "[DB ERROR] Failed to execute update statement for Event ID: " << event->getId() << "\n";
        }
    } else {
        std::cerr << "[DB ERROR] Failed to compile update statement.\n";
    }

    sqlite3_finalize(stmt);
}

const std::vector<std::unique_ptr<Event>>& CinemaRepository::getAllEvents() const {
    return schedule;
}

Event* CinemaRepository::getEventById(int id) {
    for (const auto& event : schedule) {
        if (event->getId() == id) {
            return event.get();
        }
    }
    return nullptr;
}