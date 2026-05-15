#ifndef RUNGE_KUTTA_H
#define RUNGE_KUTTA_H

#include "pendulum.h"
#include "physics_engine.h"

// Решатель методом Рунге-Кутты 4-го порядка
class RungeKuttaSolver
{
public:
    // Выполнить один шаг интегрирования
    void step(Pendulum &p,
              const PhysicsEngine &engine,
              double dt);
};

#endif