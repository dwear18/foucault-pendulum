#ifndef PENDULUM_H
#define PENDULUM_H

class Pendulum
{
public:
    double L;
    double m;

    double x;
    double y;

    double vx;
    double vy;

    Pendulum();

    void setState(double x, double y,
                  double vx, double vy);

    void getState(double &x, double &y,
                  double &vx, double &vy) const;
};

#endif