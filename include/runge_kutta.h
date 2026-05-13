#ifndef RUNGE_KUTTA_H
#define RUNGE_KUTTA_H

#include "pendulum.h"
#include "physics_engine.h"

class RungeKuttaSolver
{
public:
    void step(Pendulum &p,
              const PhysicsEngine &engine,
              double dt);
};

#endif