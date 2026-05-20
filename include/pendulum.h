#pragma once

// Класс маятника - хранит все физические параметры и состояние
class Pendulum
{
public:
    double L; // длина маятника (м)
    double m; // масса груза (кг)

    double x; // координата x (м)
    double y; // координата y (м)

    double vx; // скорость по x (м/с)
    double vy; // скорость по y (м/с)

    Pendulum();

    // Установить состояние маятника
    void setState(double x, double y, double vx, double vy);

    // Получить текущее состояние
    void getState(double &x, double &y, double &vx, double &vy) const;
};