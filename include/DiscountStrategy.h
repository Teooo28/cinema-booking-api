#pragma once

#include <string>

class DiscountStrategy {
public:
    virtual ~DiscountStrategy() = default;
    
    virtual double calculatePrice(double basePrice) const = 0;
    virtual std::string getStrategyName() const = 0;
};

class NoDiscount : public DiscountStrategy {
public:
    double calculatePrice(double basePrice) const override;
    std::string getStrategyName() const override;
};

class StudentDiscount : public DiscountStrategy {
public:
    double calculatePrice(double basePrice) const override;
    std::string getStrategyName() const override;
};
