#include "pendulum.h"

Pendulum::Pendulum()
{
    L = 98.0; // Исаакиевский собор, Санкт-Петербург
    m = 54.0; // масса груза (кг)

    x = 0.1;
    y = 0.0;

    vx = 0.0;
    vy = 0.0;
}

void Pendulum::setState(double x_,
                        double y_,
                        double vx_,
                        double vy_)
{
    x = x_;
    y = y_;
    vx = vx_;
    vy = vy_;
}

void Pendulum::getState(double &x_,
                        double &y_,
                        double &vx_,
                        double &vy_) const
{
    x_ = x;
    y_ = y;
    vx_ = vx;
    vy_ = vy;
}