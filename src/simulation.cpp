#include "simulation.h"
#include <cmath>

// Максимальное количество точек в истории
static const size_t MAX_PTS = 5000;

// Инициализировать один канал симуляции
static void initChannel(Pendulum &p, PhysicsEngine &ph,
                        double lat, double L, double m, double g,
                        double x0, double y0, double vx0, double vy0)
{
    ph.setLatitude(lat);
    ph.setG(g);
    p.L = L;
    p.m = m;
    p.setState(x0, y0, vx0, vy0);
}

// Запустить (или перезапустить) симуляцию
void Simulation::start()
{
    // Сбросить время
    time = 0.0;

    // Очистить историю
    historyX.clear();
    historyY.clear();
    historyAlpha.clear();
    historyX2.clear();
    historyY2.clear();

    // Инициализировать оба маятника с теми же начальными условиями
    initChannel(pendulum, physics, lat1,
                pendulum.L, pendulum.m, 9.80665,
                x0, y0, vx0, vy0);

    initChannel(pendulum2, physics2, lat2,
                pendulum.L, pendulum.m, 9.80665,
                x0, y0, vx0, vy0);
}

// Выполнить один шаг симуляции и сохранить данные
static void stepAndSave(Pendulum &p, PhysicsEngine &ph,
                        RungeKuttaSolver &sol,
                        std::vector<double> &hx,
                        std::vector<double> &hy,
                        double dt)
{
    // Шаг Рунге-Кутты
    sol.step(p, ph, dt);

    // Сохранить в историю
    hx.push_back(p.x);
    hy.push_back(p.y);

    // Не хранить больше MAX_PTS точек - удаляем самые старые
    if (hx.size() > MAX_PTS)
    {
        hx.erase(hx.begin());
        hy.erase(hy.begin());
    }
}

static void appendAlpha(std::vector<double> &history, double alpha)
{
    history.push_back(alpha);
    if (history.size() > MAX_PTS)
        history.erase(history.begin());
}

// Быстро перейти на будущее состояние без анимации
void Simulation::jumpToTime(double seconds, double step)
{
    if (seconds <= 0.0)
        return;

    // Используем малый интеграционный шаг для длинных прогонах,
    // чтобы не терять энергию из-за численной диссипации.
    double dtEstimate = std::min(step, this->dt);
    int steps = static_cast<int>(std::ceil(seconds / dtEstimate));
    int sampleRate = std::max(1, steps / 2500);
    double left = seconds;

    for (int i = 0; i < steps && left > 1e-12; ++i)
    {
        double dtStep = std::min(dtEstimate, left);
        solver.step(pendulum, physics, dtStep);
        if (showSecond)
            solver2.step(pendulum2, physics2, dtStep);

        time += dtStep;

        if ((i % sampleRate) == 0 || dtStep == left)
        {
            historyX.push_back(pendulum.x);
            historyY.push_back(pendulum.y);
            if (showSecond)
            {
                historyX2.push_back(pendulum2.x);
                historyY2.push_back(pendulum2.y);
            }
            appendAlpha(historyAlpha, getCurrentAlpha());
        }

        // Ограничим максимальный размер историй сразу, чтобы при больших прыжках
        // не разрастаться слишком сильно.
        if (historyX.size() > MAX_PTS)
        {
            historyX.erase(historyX.begin());
            historyY.erase(historyY.begin());
        }
        if (showSecond && historyX2.size() > MAX_PTS)
        {
            historyX2.erase(historyX2.begin());
            historyY2.erase(historyY2.begin());
        }

        left -= dtStep;
    }
}

// Обновить симуляцию на один кадр
void Simulation::update(double real_dt)
{
    // Масштабировать реальное время
    real_dt *= timeScale;

    // Количество шагов за кадр
    int steps = static_cast<int>(real_dt / dt);

    for (int i = 0; i < steps; i++)
    {
        // Шаг для маятника A (широта lat1)
        stepAndSave(pendulum, physics, solver,
                    historyX, historyY, dt);

        // Шаг для маятника B (широта lat2) если нужно
        if (showSecond)
        {
            stepAndSave(pendulum2, physics2, solver2,
                        historyX2, historyY2, dt);
        }

        // Сохранить угол прецессии α = -Ωz * t
        double alpha = getCurrentAlpha();
        historyAlpha.push_back(alpha);
        if (historyAlpha.size() > MAX_PTS)
            historyAlpha.erase(historyAlpha.begin());
    }

    // Добавить реальное прошедшее время (независимо от количества шагов)
    time += real_dt;
}

// Остановить симуляцию
void Simulation::stop()
{
    start();
}

// Текущий угол прецессии плоскости колебаний (рад)
double Simulation::getCurrentAlpha() const
{
    // α(t) = -Ωz * t, где Ωz = ω * sin(φ)
    return -physics.getOmegaZ() * time;
}