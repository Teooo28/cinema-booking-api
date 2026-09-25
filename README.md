# Cinema Booking Web API

## Executive Summary
A robust, multithreaded RESTful API built in modern C++ for managing cinema reservations. This project transitions traditional OOP concepts into a fully functional web backend, featuring stateless JWT authentication, Role-Based Access Control (RBAC), thread-safe database transactions, and dynamic pricing models.

Designed with a focus on clean architecture and scalability, the API handles mixed-cart bookings, automatic inventory synchronization, and secure ticket cancellations.

## Architecture & Design Patterns
The system is built upon strict software engineering principles to ensure maintainability and separation of concerns:

* **MVC (Model-View-Controller):** Routing logic is delegated to specific controllers (`AuthController`, `MovieController`), keeping the entry point (`main.cpp`) clean and acting merely as a dispatcher.
* **Strategy Pattern:** Implemented via the `DiscountStrategy` interface (`NoDiscount`, `StudentDiscount`). It powers a polymorphic pricing engine capable of calculating nested, mixed-cart payloads at runtime without complex `if/else` chains.
* **Factory Pattern:** The `EventFactory` encapsulates the instantiation logic for polymorphic entities (`Movie2D`, `Movie3D`), essential for the dynamic, admin-only movie creation endpoint.
* **Repository Pattern:** `CinemaRepository` abstracts all SQLite persistence logic, utilizing *Upsert* mechanisms to seamlessly sync in-memory states with disk storage upon server restarts.

## Key Features

* **Advanced Authentication & RBAC:** Secures sensitive endpoints using JSON Web Tokens (HS256 signature). Enforces strict Role-Based Access Control (Admin vs. Client privileges).
* **Modern C++ Memory Management:** Zero memory leaks. Utilizes smart pointers (`std::unique_ptr`, `std::shared_ptr`) for resource lifecycle management (RAII).
* **Thread-Safe Transactions:** Employs `std::lock_guard` in critical sections (e.g., ticket purchasing, cancellations) to prevent race conditions in a multithreaded server environment.
* **Standardized API Responses:** Wraps all outgoing HTTP responses in a generic `ApiResponse` template for consistent, predictable JSON structures across the frontend.
* **Defensive Programming:** Comprehensive exception handling (`InvalidDataException`, `UnauthorizedException`) mapped to proper HTTP status codes (400, 401, 404, 500).

## Tech Stack
* **Language:** C++20
* **Web Framework:** [Crow](https://crowcpp.org/) (Multithreaded C++ Microframework)
* **Database:** SQLite3 (Embedded Relational DB)
* **Serialization:** [nlohmann/json](https://github.com/nlohmann/json) (JSON for Modern C++)
* **Security:** `jwt-cpp` & `picosha2` (Token generation, validation, and payload decoding)
* **Build System:** CMake

## API Endpoints Documentation

### 1. Authentication
| Method | Endpoint | Description | Access |
| :--- | :--- | :--- | :--- |
| `POST` | `/register` | Register a new user (default role: `client`). | Public |
| `POST` | `/login` | Authenticate user and receive JWT. | Public |

### 2. Catalog Management
| Method | Endpoint | Description | Access |
| :--- | :--- | :--- | :--- |
| `GET` | `/movies` | Fetch all available movies, dynamic prices, and seats. | Public |
| `POST` | `/movies` | Add a new movie to the database (2D/3D). | **Admin Only** |

### 3. Booking & Operations
| Method | Endpoint | Description | Access |
| :--- | :--- | :--- | :--- |
| `POST` | `/movies/{id}/book` | Book tickets. Supports nested JSON for mixed carts (e.g., Adults + Students). | Authenticated |
| `DELETE` | `/cancel` | Cancel a reservation via ticket code and refund seats to the database. | Authenticated |

## Payload Examples

**Mixed-Cart Booking (`POST /movies/{id}/book`):**
```json
{
    "tickets": {
        "adult": 2,
        "student": 1
    }
}
```

*Server calculates the dynamic polymorphic price and returns:*

```json
{
    "success": true,
    "message": "Booking Confirmed",
    "data": "You have successfully booked 3 tickets. Total payment: 102.50 RON. Entry code: #TKT-1041"
}
```

## Build & Run Instructions

**Prerequisites:**

* A modern C++ compiler (MSVC, GCC, or Clang)
* CMake (3.15+)

**Build Steps:**

```bash
# 1. Clone the repository
git clone https://github.com/Teooo28/cinema-booking-api.git
cd cinema-booking-api

# 2. Generate build files
cmake -S . -B build

# 3. Build the executable
cmake --build build

# 4. Run the server
./build/oop.exe   # Windows
# or
./build/oop       # Linux/macOS
```