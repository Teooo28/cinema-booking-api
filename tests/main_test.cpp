#include <gtest/gtest.h>
#include "DiscountStrategy.h"
#include "Movie2D.h"
#include "Exceptions.h"

TEST(DiscountTestSuite, StudentDiscountAppliesExactlyHalfPrice) {
    // Arrange: Set up the environment and test data
    StudentDiscount strategy;
    double basePrice = 50.0;
    
    // Act: Execute the business logic
    double finalPrice = strategy.calculatePrice(basePrice);
    
    // Assert: Verify the outcome matches expectations
    EXPECT_DOUBLE_EQ(finalPrice, 25.0);
}

TEST(DiscountTestSuite, NoDiscountAppliesFullPrice) {
    // Arrange
    NoDiscount strategy;
    double basePrice = 50.0;
    
    // Act
    double finalPrice = strategy.calculatePrice(basePrice);
    
    // Assert
    EXPECT_DOUBLE_EQ(finalPrice, 50.0);
}

TEST(EventTestSuite, BookSeatsThrowsExceptionWhenExceedingLimit) {
    // Arrange: Create a valid movie object with plenty of seats
    Movie2D testMovie(1, "Test Movie", 120, 20.0, 100);

    // Act & Assert: Attempt to book 11 tickets. 
    // The business logic dictates a maximum of 10 tickets per transaction.
    EXPECT_THROW(testMovie.bookSeats(11), InvalidDataException);
}