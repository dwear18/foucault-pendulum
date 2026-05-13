#include "runge_kutta.h"

struct State
{
    double x;
    double vx;
    double y;
    double vy;
};

static State derivatives(
    const State &s,
    const Pendulum &pTemplate,
    const PhysicsEngine &engine)
{
    Pendulum p = pTemplate;

    p.x = s.x;
    p.y = s.y;
    p.vx = s.vx;
    p.vy = s.vy;

    double ax, ay;

    engine.computeAcceleration(p, ax, ay);

    return {
        s.vx,
        ax,
        s.vy,
        ay};
}

void RungeKuttaSolver::step(
    Pendulum &p,
    const PhysicsEngine &engine,
    double dt)
{
    State s{
        p.x,
        p.vx,
        p.y,
        p.vy};

    State k1 = derivatives(s, p, engine);

    State s2{
        s.x + 0.5 * dt * k1.x,
        s.vx + 0.5 * dt * k1.vx,
        s.y + 0.5 * dt * k1.y,
        s.vy + 0.5 * dt * k1.vy};

    State k2 = derivatives(s2, p, engine);

    State s3{
        s.x + 0.5 * dt * k2.x,
        s.vx + 0.5 * dt * k2.vx,
        s.y + 0.5 * dt * k2.y,
        s.vy + 0.5 * dt * k2.vy};

    State k3 = derivatives(s3, p, engine);

    State s4{
        s.x + dt * k3.x,
        s.vx + dt * k3.vx,
        s.y + dt * k3.y,
        s.vy + dt * k3.vy};

    State k4 = derivatives(s4, p, engine);

    p.x += dt / 6.0 *
           (k1.x + 2 * k2.x + 2 * k3.x + k4.x);

    p.vx += dt / 6.0 *
            (k1.vx + 2 * k2.vx + 2 * k3.vx + k4.vx);

    p.y += dt / 6.0 *
           (k1.y + 2 * k2.y + 2 * k3.y + k4.y);

    p.vy += dt / 6.0 *
            (k1.vy + 2 * k2.vy + 2 * k3.vy + k4.vy);
}