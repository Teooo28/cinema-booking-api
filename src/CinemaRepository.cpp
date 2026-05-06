#include "CinemaRepository.h"
#include <iostream>

CinemaRepository::CinemaRepository() {
    // deschidem fisierul bazei de date (sau il cream daca nu exista)
    int rc = sqlite3_open("cinema.db", &db); // rc = return code (0 = SQLITE_OK)
    if (rc) {
        std::cerr << "Eroare la deschiderea DB: " << sqlite3_errmsg(db) << "\n";
    } else {
        initDatabase();
    }
}

CinemaRepository::~CinemaRepository() {
    // cand oprim serverul inchidem fisierul
    sqlite3_close(db);
}

void CinemaRepository::initDatabase() {
    // cream tabelul care retine id-ul filmului si locurile ramase
    const char* sql = "CREATE TABLE IF NOT EXISTS seats_inventory ("
                      "event_id INTEGER PRIMARY KEY, "
                      "available_seats INTEGER);";
    
    char* errMsg = nullptr;
    sqlite3_exec(db, sql, nullptr, nullptr, &errMsg); // trimitem comanda SQL catre hard disk
    if (errMsg) {
        std::cerr << "Eroare SQL: " << errMsg << "\n";
        sqlite3_free(errMsg);   // eliberam memoria mesajului de eroare
    }
}

void CinemaRepository::addEvent(std::unique_ptr<Event> event) {
    int currentId = event->getId();
    
    // pregatim interogarea SELECT pentru a vedea daca filmul e deja pe disc
    std::string sqlCheck = "SELECT available_seats FROM seats_inventory WHERE event_id = " + std::to_string(currentId) + ";";
    sqlite3_stmt* stmt; // stmt = return statement
    
    // compilam textul SQL in cod executabil pentru SQLite
    int rc = sqlite3_prepare_v2(db, sqlCheck.c_str(), -1, &stmt, nullptr);
    
    bool foundInDb = false;
    if (rc == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW) {  // SQLITE_ROW indica faptul ca a gasit un rand cu date
        
        // citim valoarea tip intreg de pe prima coloana (index 0)
        int savedSeats = sqlite3_column_int(stmt, 0);

        // suprascriem capacitatea maxima cu locurile gasite pe hard disk
        event->setAvailableSeats(savedSeats);
        foundInDb = true;
    }
    sqlite3_finalize(stmt); // curatam memoria interogarii

    // daca filmul nu exista il inseram in tabel
    if (!foundInDb) {
        std::string sqlInsert = "INSERT INTO seats_inventory (event_id, available_seats) VALUES (" + 
                                std::to_string(currentId) + ", " + 
                                std::to_string(event->getAvailableSeats()) + ");";
        sqlite3_exec(db, sqlInsert.c_str(), nullptr, nullptr, nullptr);
    }

    schedule.push_back(std::move(event));
}

void CinemaRepository::updateEvent(Event* event) {
    // comanda de UPDATE pentru a salva numarul nou de locuri
    std::string sqlUpdate = "UPDATE seats_inventory SET available_seats = " + 
                            std::to_string(event->getAvailableSeats()) + 
                            " WHERE event_id = " + std::to_string(event->getId()) + ";";
    
    char* errMsg = nullptr;

    // executam modificarea pe disc fara a astepta date in retur
    sqlite3_exec(db, sqlUpdate.c_str(), nullptr, nullptr, &errMsg);
    
    if (errMsg) {
        std::cerr << "Eroare Update DB: " << errMsg << "\n";
        sqlite3_free(errMsg);
    }
}

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