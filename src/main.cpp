#include <iostream>
#include <vector>
#include "EventFactory.h"
#include "DiscountStrategy.h"
#include "ApiResponse.h" 

int main() {
    // Exemplul 1: T devine 'std::string'
    ApiResponse<std::string> loginResponse(true, "Autentificare cu succes", "Token: ABC-123-XYZ");
    std::cout << "--- Exemplu String ---" << std::endl;
    loginResponse.print();

    std::cout << "\n----------------------\n" << std::endl;

    // Exemplul 2: Creăm un film folosind Factory
    auto movie = EventFactory::createEvent("3D", 2, "Avatar", 192, 30.0, 10.0);
    movie->setDiscountStrategy(std::make_shared<StudentDiscount>());

    // T devine 'nlohmann::json' (am cerut datele filmului sub formă de json)
    ApiResponse<nlohmann::json> movieResponse(true, "Filmul a fost gasit", movie->toJson());
    
    std::cout << "--- Exemplu Obiect Complex ---" << std::endl;
    movieResponse.print();

    return 0;
}