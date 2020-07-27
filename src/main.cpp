#include "app.h"
#include <cstdlib> // For std::rand() and std::srand()
#include <ctime> // For std::time();

int main()
{
    // Seed randomness
    std::srand(std::time(nullptr));
    App app{};
    return app.exec();
}