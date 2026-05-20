#pragma once

#include "pendulum.h"

// Физический движок - вычисляет ускорения с учётом силы Кориолиса
class PhysicsEngine
{
private:
    double omega;    // угловая скорость вращения Земли (рад/с)
    double latitude; // географическая широта (рад)
    double g;        // ускорение свободного падения (м/с²)

public:
    PhysicsEngine();

    // Установить широту в градусах
    void setLatitude(double lat_deg);

    // Установить ускорение свободного падения
    void setG(double newG);

    // Вычислить ускорение маятника (включая силу Кориолиса)
    void computeAcceleration(const Pendulum &p,
                             double &ax,
                             double &ay) const;

    // Получить вертикальную составляющую угловой скорости Земли
    double getOmegaZ() const;
};