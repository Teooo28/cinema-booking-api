#pragma once
#include <string>

class User {
private:
    int id;
    std::string username;
    std::string passwordHash;
    std::string role;

public:
    // Constructor used when retrieving an existing user from the database
    User(int id, const std::string& username, const std::string& passwordHash, const std::string& role) 
        : id(id), username(username), passwordHash(passwordHash), role(role) {}

    // Constructor used when creating a new user (ID is generated later by SQLite)
    User(const std::string& username, const std::string& passwordHash, const std::string& role) 
        : id(0), username(username), passwordHash(passwordHash), role(role) {}

    int getId() const { return id; }
    std::string getUsername() const { return username; }
    std::string getPasswordHash() const { return passwordHash; }
    std::string getRole() const { return role; }
};