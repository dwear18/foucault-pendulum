#include "physics_engine.h"

#include <cmath>

PhysicsEngine::PhysicsEngine()
{
    omega = 7.2921e-5;
    latitude = 0.0;
    g = 9.80665;
}

void PhysicsEngine::setLatitude(double lat_deg)
{
    latitude = lat_deg * M_PI / 180.0;
}

void PhysicsEngine::setG(double newG)
{
    g = newG;
}

double PhysicsEngine::getOmegaZ() const
{
    return omega * sin(latitude);
}

void PhysicsEngine::computeAcceleration(
    const Pendulum &p,
    double &ax,
    double &ay) const
{
    double omega0 = sqrt(g / p.L);

    double omegaZ = getOmegaZ();

    ax = -omega0 * omega0 * p.x + 2.0 * omegaZ * p.vy;

    ay = -omega0 * omega0 * p.y - 2.0 * omegaZ * p.vx;
}