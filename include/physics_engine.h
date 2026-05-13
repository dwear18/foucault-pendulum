#ifndef PHYSICS_ENGINE_H
#define PHYSICS_ENGINE_H

#include "pendulum.h"

class PhysicsEngine
{
private:
    double omega;
    double latitude;
    double g;

public:
    PhysicsEngine();

    void setLatitude(double lat_deg);

    void setG(double newG);

    void computeAcceleration(const Pendulum &p,
                             double &ax,
                             double &ay) const;

    double getOmegaZ() const;
};



#endif