#pragma once

#include <vector>
#include "pendulum.h"
#include "physics_engine.h"
#include "runge_kutta.h"

// Ядро симуляции - объединяет маятник, физику и решатель
class Simulation
{
public:
    //  Основные объекты
    Pendulum pendulum;       // маятник (основной, широта A)
    PhysicsEngine physics;   // физический движок (широта A)
    RungeKuttaSolver solver; // решатель РК4

    //  Для сравнения широт
    Pendulum pendulum2;       // маятник для широты B
    PhysicsEngine physics2;   // физика для широты B
    RungeKuttaSolver solver2; // решатель для широты B
    bool showSecond = true;   // показывать второй маятник?

    //  Параметры времени 
    double time = 0.0;
    double dt = 0.01;       // шаг интегрирования (с)
    double T_max = 200.0;   // максимальное время симуляции (с)
    double timeScale = 1.0; // множитель скорости

    //  Начальные условия 
    double x0 = 0.10;  // начальное смещение x (м)
    double y0 = 0.00;  // начальное смещение y (м)
    double vx0 = 0.00; // начальная скорость Vx (м/с)
    double vy0 = 0.00; // начальная скорость Vy (м/с)

    //  Широты для сравнения 
    double lat1 = 45.0; // широта A (градусы)
    double lat2 = 60.0; // широта B (градусы)

    //  История траектории (для графиков) 
    std::vector<double> historyX;     // x(t) - широта A
    std::vector<double> historyY;     // y(t) - широта A
    std::vector<double> historyAlpha; // α(t) - угол прецессии

    std::vector<double> historyX2; // x(t) - широта B
    std::vector<double> historyY2; // y(t) - широта B

    //  Методы 
    void start();                // инициализировать симуляцию
    void update(double real_dt); // обновить на один кадр
    void stop();                 // остановить (сброс = start)

    // Получить текущий угол прецессии (рад)
    double getCurrentAlpha() const;

    // Получить историю траектории
    const std::vector<double> &getTrajectoryX() const { return historyX; }
    const std::vector<double> &getTrajectoryY() const { return historyY; }

    // Получить историю угла
    const std::vector<double> &getAngleHistory() const { return historyAlpha; }
};