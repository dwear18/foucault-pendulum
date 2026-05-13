#pragma once

#include <vector>
#include "pendulum.h"
#include "physics_engine.h"
#include "runge_kutta.h"

class Simulation
{
public:
    Pendulum pendulum;
    PhysicsEngine physics;
    RungeKuttaSolver solver;

    double time = 0;
    double dt = 0.01;
    double timeScale = 1.0;
    double T_max = 100.0;

    std::vector<double> historyX;
    std::vector<double> historyY;
    std::vector<double> historyAlpha;

    void start();
    void update(double real_dt);

    double getCurrentAlpha() const;
};