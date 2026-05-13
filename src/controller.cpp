#include "controller.h"

#include <iostream>

void Controller::getInput(Simulation &sim)
{
    double lat;

    std::cout << "Широта: ";
    std::cin >> lat;

    if (lat < -90 || lat > 90)
    {
        std::cout << "Ошибка широты\n";
        exit(1);
    }

    sim.physics.setLatitude(lat);

    std::cout << "Длина L: ";
    std::cin >> sim.pendulum.L;

    if (sim.pendulum.L <= 0)
    {
        std::cout << "Ошибка длины\n";
        exit(1);
    }

    std::cout << "Масса m: ";
    std::cin >> sim.pendulum.m;

    std::cout << "x0: ";
    std::cin >> sim.pendulum.x;

    std::cout << "y0: ";
    std::cin >> sim.pendulum.y;

    std::cout << "dt: ";
    std::cin >> sim.dt;

    if (sim.dt > 0.1)
    {
        std::cout << "Предупреждение: dt слишком большой\n";
    }

    std::cout << "T_max: ";
    std::cin >> sim.T_max;

    std::cout << "Масштаб времени: ";
    std::cin >> sim.timeScale;
}