#include "simulation.h"
#include <cmath>

void Simulation::start()
{
    historyX.clear();
    historyY.clear();
    historyAlpha.clear();

    time = 0;
}

void Simulation::update(double real_dt)
{
    real_dt *= timeScale;

    int steps = static_cast<int>(real_dt / dt);

    for (int i = 0; i < steps; i++)
    {
        solver.step(pendulum, physics, dt);
        time += dt;

        historyX.push_back(pendulum.x);
        historyY.push_back(pendulum.y);
        historyAlpha.push_back(getCurrentAlpha());

        if (historyX.size() > 5000)
        {
            historyX.erase(historyX.begin());
            historyY.erase(historyY.begin());
            historyAlpha.erase(historyAlpha.begin());
        }
    }
}

double Simulation::getCurrentAlpha() const
{
    double omegaZ = physics.getOmegaZ();
    return -omegaZ * time;
}