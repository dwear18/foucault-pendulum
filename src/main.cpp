#include <SFML/Graphics.hpp>
#include <optional>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <iostream>

#include "simulation.h"
#include "renderer.h"
#include "controller.h"

// ============================================================
// Логический размер окна (в «дизайн-пикселях»)
// На Retina реальных пикселей в 2 раза больше, но мы запрашиваем
// именно этот размер — SFML сам сделает 2x буфер.
// Все координаты задаём в этих единицах.
// ============================================================
static const unsigned LOGI_W = 1400;
static const unsigned LOGI_H = 900;

// ============================================================
// Размеры панелей (в логических пикселях)
// ============================================================
static const float W = (float)LOGI_W;
static const float H = (float)LOGI_H;
static const float TOP = 52.f;
static const float BOT = 34.f;
static const float LEFT = 440.f;
static const float PAD = 6.f;
static const float CTRL = 38.f; // высота строки кнопок

// Кнопки — снизу вверх от статус-бара
static const float BTN_Y = H - BOT - PAD - CTRL;
static const float BTN_W = (LEFT - PAD * 2.f) / 4.f - 2.f;

// Траектория — занимает всё между шапкой и кнопками
static const float TRAJ_X = PAD;
static const float TRAJ_Y = TOP + PAD;
static const float TRAJ_W = LEFT - PAD * 2.f;
static const float TRAJ_H = BTN_Y - TRAJ_Y - PAD;

// Три графика справа
static const float GX = LEFT + PAD;
static const float GW = W - GX - PAD;
static const float GY0 = TOP + PAD;
static const float GH = (H - TOP - BOT - PAD * 4.f) / 3.f;

// ============================================================
// Примитивы
// ============================================================

static void box(sf::RenderWindow &w, float x, float y,
                float wd, float ht, sf::Color fill,
                sf::Color border = sf::Color::Transparent,
                float thick = 0.f)
{
    sf::RectangleShape r(sf::Vector2f(wd, ht));
    r.setPosition(sf::Vector2f(x, y));
    r.setFillColor(fill);
    if (thick > 0.f)
    {
        r.setOutlineThickness(thick);
        r.setOutlineColor(border);
    }
    w.draw(r);
}

static void hline(sf::RenderWindow &w, float x, float y, float len, sf::Color c)
{
    box(w, x, y, len, 1.f, c);
}

static void vline(sf::RenderWindow &w, float x, float y, float len, sf::Color c)
{
    box(w, x, y, 1.f, len, c);
}

static std::string fmt(double v, int p = 1)
{
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(p) << v;
    return ss.str();
}

// ============================================================
// Кнопка
// ============================================================
static sf::FloatRect drawBtn(sf::RenderWindow &w, Renderer &r,
                             const std::string &label,
                             float x, float y, float wd, float ht,
                             sf::Color bg, sf::Color fg)
{
    sf::FloatRect rc(sf::Vector2f(x, y), sf::Vector2f(wd, ht));
    // Конвертируем позицию мыши в логические координаты
    sf::Vector2i mp = sf::Mouse::getPosition(w);
    sf::Vector2f mf = w.mapPixelToCoords(mp);
    if (rc.contains(mf))
    {
        bg.r = (uint8_t)std::min(255, (int)bg.r + 25);
        bg.g = (uint8_t)std::min(255, (int)bg.g + 25);
        bg.b = (uint8_t)std::min(255, (int)bg.b + 25);
    }
    box(w, x, y, wd, ht, bg, sf::Color(255, 255, 255, 30), 1.f);
    r.drawText(w, label, x + 8.f, y + (ht - 16.f) / 2.f, 14, fg);
    return rc;
}

// ============================================================
// Индикатор прецессии (п.10 ТЗ)
// ============================================================
static void drawPrecession(sf::RenderWindow &w, Renderer &r,
                           float cx, float cy, float rad, double alpha)
{
    sf::CircleShape bg(rad);
    bg.setOrigin(sf::Vector2f(rad, rad));
    bg.setPosition(sf::Vector2f(cx, cy));
    bg.setFillColor(sf::Color(18, 22, 34));
    bg.setOutlineThickness(1.f);
    bg.setOutlineColor(sf::Color(50, 70, 110));
    w.draw(bg);

    for (int i = 0; i < 12; i++)
    {
        float a = i * (float)M_PI / 6.f;
        sf::VertexArray t(sf::PrimitiveType::LineStrip, 2);
        t[0].position = {cx + (rad - 5) * std::cos(a), cy + (rad - 5) * std::sin(a)};
        t[0].color = sf::Color(50, 65, 100);
        t[1].position = {cx + (rad - 1) * std::cos(a), cy + (rad - 1) * std::sin(a)};
        t[1].color = sf::Color(50, 65, 100);
        w.draw(t);
    }

    // базовая линия
    sf::VertexArray base(sf::PrimitiveType::LineStrip, 2);
    base[0].position = {cx, cy};
    base[0].color = sf::Color(50, 65, 100);
    base[1].position = {cx + rad * 0.82f, cy};
    base[1].color = sf::Color(50, 65, 100);
    w.draw(base);

    // дуга
    if (std::abs(alpha) > 0.001)
    {
        sf::VertexArray arc(sf::PrimitiveType::LineStrip);
        for (int i = 0; i <= 32; i++)
        {
            float a = (float)alpha * i / 32.f;
            sf::Vertex v;
            v.position = {cx + rad * 0.6f * std::cos(a), cy + rad * 0.6f * std::sin(a)};
            v.color = sf::Color(235, 195, 50, 110);
            arc.append(v);
        }
        w.draw(arc);
    }

    // стрелка
    sf::VertexArray arr(sf::PrimitiveType::LineStrip, 2);
    arr[0].position = {cx, cy};
    arr[0].color = sf::Color(235, 200, 55);
    arr[1].position = {cx + rad * 0.78f * (float)std::cos(alpha),
                       cy + rad * 0.78f * (float)std::sin(alpha)};
    arr[1].color = sf::Color(235, 200, 55, 150);
    w.draw(arr);

    r.drawText(w, "Precession", cx - rad, cy - rad - 14, 12, sf::Color(160, 148, 55));
    double deg = alpha * 180.0 / M_PI;
    r.drawText(w, fmt(deg, 2) + " deg", cx - rad, cy + rad + 3, 13, sf::Color(235, 200, 55));
}

// ============================================================
// MAIN
// ============================================================
int main()
{
    // Создаём окно запрашивая логический размер.
    // SFML на Retina сам выделит framebuffer 2800x1800,
    // но getSize() вернёт 2800x1800.
    // Мы устанавливаем View = логический размер,
    // тогда ВСЕ координаты (рисование + клики) одинаковые.
    sf::RenderWindow window(
        sf::VideoMode(sf::Vector2u(LOGI_W, LOGI_H)),
        "Foucault Pendulum Simulation");
    window.setFramerateLimit(60);

    // Ключевой fix для Retina:
    // setView с логическим размером гарантирует что
    // mapPixelToCoords вернёт логические координаты для кликов,
    // а шрифт рендерится в реальных пикселях (чётко).
    sf::View view(sf::FloatRect(sf::Vector2f(0, 0), sf::Vector2f(W, H)));
    window.setView(view);

    Simulation sim;
    Renderer renderer;
    Controller controller;

    // --- Шрифт ---
    std::vector<std::string> fontPaths = {
        "DejaVuSans.ttf",
        "font.ttf",
        "arial.ttf",
        "/System/Library/Fonts/Supplemental/Arial Unicode.ttf",
        "/Library/Fonts/Arial Unicode.ttf",
        "/usr/local/share/fonts/DejaVuSans.ttf",
        "/opt/homebrew/share/fonts/dejavu-fonts/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "C:/Windows/Fonts/arial.ttf",
    };
    bool fontOk = false;
    for (const auto &p : fontPaths)
        if (renderer.loadFont(p))
        {
            fontOk = true;
            break;
        }
    if (!fontOk)
    {
        std::cerr << "Font not found. Place DejaVuSans.ttf next to the exe.\n";
        return 1;
    }

    sim.start();
    sf::Clock clock;
    bool paused = false;

    // Прямоугольники кнопок — заполняются при рисовании
    sf::FloatRect btnPause, btnReset, btnSpeedDn, btnSpeedUp;

    while (window.isOpen())
    {
        // --- События ---
        while (const std::optional ev = window.pollEvent())
        {
            if (ev->is<sf::Event::Closed>())
                window.close();

            if (const auto *te = ev->getIf<sf::Event::TextEntered>())
                controller.handleTextEntered(te->unicode);

            if (const auto *k = ev->getIf<sf::Event::KeyPressed>())
                controller.handleKey(k->code, sim, paused);

            if (const auto *mc = ev->getIf<sf::Event::MouseButtonPressed>())
            {
                // mapPixelToCoords конвертирует реальные пиксели мыши
                // в логические координаты View — именно это решает
                // проблему с кнопками на Retina
                sf::Vector2f pos = window.mapPixelToCoords(mc->position);

                if (btnPause.contains(pos))
                    paused = !paused;
                if (btnReset.contains(pos))
                {
                    controller.handleStop(sim);
                    paused = false;
                }
                if (btnSpeedDn.contains(pos))
                    sim.timeScale = std::max(0.1, sim.timeScale / 1.25);
                if (btnSpeedUp.contains(pos))
                    sim.timeScale = std::min(20.0, sim.timeScale * 1.25);

                // Клик по [P] Params в правом углу шапки
                if (pos.y < TOP && pos.x > W * 0.85f)
                {
                    controller.panelOpen = !controller.panelOpen;
                    if (controller.panelOpen)
                        controller.syncFrom(sim);
                }

                controller.handleClick(pos, sim);
            }
        }

        if (controller.applyNow)
        {
            controller.applyNow = false;
            if (controller.applyTo(sim))
            {
                controller.handleStart(sim);
                controller.panelOpen = false;
                paused = false;
            }
        }

        if (!paused)
        {
            double dt = clock.restart().asSeconds();
            if (dt > 0.1)
                dt = 0.1;
            sim.update(dt);
        }
        else
            clock.restart();

        // ============================================================
        // РИСОВАНИЕ
        // ============================================================
        window.clear(sf::Color(14, 16, 24));

        // ── ШАПКА ───────────────────────────────────────────────────
        box(window, 0, 0, W, TOP, sf::Color(20, 23, 36));
        hline(window, 0, TOP - 1, W, sf::Color(40, 55, 90));

        renderer.drawText(window, "Foucault Pendulum", PAD, 8, 20, sf::Color(88, 152, 238));
        renderer.drawText(window, "Simulation  |  RK4  |  Coriolis", PAD, 32, 12, sf::Color(48, 68, 110));

        // Блоки данных: каждый занимает ~100px, фиксированный X
        // label на y=10, value на y=30 — не перекрываются
        struct Block
        {
            float x;
            const char *label;
            std::string val;
            sf::Color vc;
        };
        Block blocks[] = {
            {290, "Speed", fmt(sim.timeScale) + "x", sf::Color(215, 205, 72)},
            {400, "Time", fmt(sim.time) + " s", sf::Color(128, 198, 255)},
            {510, "Lat.A", fmt(sim.lat1) + "d", sf::Color(78, 218, 108)},
            {620, "Lat.B", sim.showSecond ? fmt(sim.lat2) + "d" : "off",
             sim.showSecond ? sf::Color(172, 128, 250) : sf::Color(55, 55, 75)},
            {730, "Status", paused ? "|| Pause" : "> Running",
             paused ? sf::Color(228, 78, 78) : sf::Color(62, 208, 108)},
        };
        for (auto &b : blocks)
        {
            renderer.drawText(window, b.label, b.x, 10, 12, sf::Color(95, 105, 130));
            renderer.drawText(window, b.val, b.x, 28, 15, b.vc);
        }
        renderer.drawText(window, "[P] Params", W - 105, 18, 13, sf::Color(70, 100, 160));

        // ── ЛЕВАЯ ПАНЕЛЬ ────────────────────────────────────────────
        float lx = TRAJ_X, ly = TRAJ_Y, lw = TRAJ_W, lh = TRAJ_H;

        box(window, lx, ly, lw, lh, sf::Color(16, 19, 30), sf::Color(40, 60, 100), 1.f);
        renderer.drawText(window, "Trajectory", lx + 8, ly + 6, 13, sf::Color(65, 125, 210));

        // Сетка
        for (int i = 1; i < 5; i++)
        {
            sf::Color gc(24, 30, 48);
            vline(window, lx + i * lw / 5.f, ly + 1, lh - 2, gc);
            hline(window, lx + 1, ly + i * lh / 5.f, lw - 2, gc);
        }

        // Оси
        float axCx = lx + lw * 0.5f, axCy = ly + lh * 0.44f;
        hline(window, lx + 4, axCy, lw - 8, sf::Color(36, 48, 78));
        vline(window, axCx, ly + 4, lh - 8, sf::Color(36, 48, 78));
        renderer.drawText(window, "x", lx + lw - 14, axCy + 3, 12, sf::Color(48, 65, 105));
        renderer.drawText(window, "y", axCx + 3, ly + 5, 12, sf::Color(48, 65, 105));

        float scale = 820.f;

        renderer.drawTrajectory(window,
                                sim.getTrajectoryX(), sim.getTrajectoryY(),
                                sf::Color(46, 212, 142), axCx, axCy, scale);
        if (sim.showSecond)
            renderer.drawTrajectory(window,
                                    sim.historyX2, sim.historyY2,
                                    sf::Color(172, 98, 248), axCx, axCy, scale);

        renderer.drawPendulum(window, sim.pendulum, axCx, axCy, scale);

        // Координаты груза — одна строка, не вылезает за панель
        renderer.drawText(window,
                          "x=" + fmt(sim.pendulum.x, 3) + "  y=" + fmt(sim.pendulum.y, 3),
                          lx + 6, ly + lh - 16, 12, sf::Color(80, 130, 105, 200));

        // Легенда широт — правый нижний угол, внутри панели
        if (sim.showSecond)
        {
            float legy = ly + lh - 34;
            float legx = lx + lw - 72;
            box(window, legx, legy + 5, 10, 3, sf::Color(46, 212, 142));
            renderer.drawText(window, fmt(sim.lat1, 0) + "d", legx + 14, legy, 12, sf::Color(46, 212, 142));
            box(window, legx, legy + 21, 10, 3, sf::Color(172, 98, 248));
            renderer.drawText(window, fmt(sim.lat2, 0) + "d", legx + 14, legy + 16, 12, sf::Color(172, 98, 248));
        }

        // Индикатор прецессии — левый нижний угол, радиус 36
        // Размещаем так чтобы всё помещалось:
        // "Precession" (12px,~70px шир) = cx-36..cx+~34
        // значение "-0.10 deg" = cx-36..cx+~70
        // cx=lx+50 => "Precession" от x=lx+14 — внутри панели
        {
            float rad = 36.f;
            float icx = lx + 50, icy = ly + lh - 50;
            drawPrecession(window, renderer, icx, icy, rad, sim.getCurrentAlpha());
        }

        // ── КНОПКИ УПРАВЛЕНИЯ ───────────────────────────────────────
        {
            float bx = TRAJ_X, by = BTN_Y, bw = BTN_W;
            sf::Color pauseC = paused ? sf::Color(35, 115, 35) : sf::Color(115, 35, 35);
            btnPause = drawBtn(window, renderer, paused ? "> Resume" : "|| Pause",
                               bx, by, bw, CTRL - 2, pauseC, sf::Color(220, 240, 220));
            btnReset = drawBtn(window, renderer, "R  Reset",
                               bx + bw + 2, by, bw, CTRL - 2, sf::Color(38, 42, 88), sf::Color(190, 205, 255));
            btnSpeedDn = drawBtn(window, renderer, "-  Speed",
                                 bx + 2 * (bw + 2), by, bw, CTRL - 2, sf::Color(50, 46, 18), sf::Color(225, 218, 130));
            btnSpeedUp = drawBtn(window, renderer, "+  Speed",
                                 bx + 3 * (bw + 2), by, bw, CTRL - 2, sf::Color(50, 46, 18), sf::Color(225, 218, 130));
        }

        // ── ПРАВАЯ ПАНЕЛЬ: ГРАФИКИ ───────────────────────────────────
        struct Graph
        {
            const std::vector<double> *dataA, *dataB;
            sf::Color color;
            const char *title, *desc;
            void (Renderer::*fn)(sf::RenderWindow &,
                                 const std::vector<double> &, sf::Vector2f, sf::Vector2f, sf::Color);
        };
        Graph graphs[3] = {
            {&sim.historyX, &sim.historyX2, sf::Color(242, 88, 88),
             "x(t)", "Displacement X, m", &Renderer::drawGraphXvsTime},
            {&sim.historyY, &sim.historyY2, sf::Color(68, 215, 105),
             "y(t)", "Displacement Y, m", &Renderer::drawGraphYvsTime},
            {&sim.historyAlpha, nullptr, sf::Color(242, 212, 48),
             "a(t)", "Precession angle, rad", &Renderer::drawGraphAlphaVsTime},
        };
        for (int i = 0; i < 3; i++)
        {
            float gy = GY0 + i * (GH + PAD);
            box(window, GX, gy, GW, GH, sf::Color(14, 17, 26),
                sf::Color(graphs[i].color.r / 3, graphs[i].color.g / 3, graphs[i].color.b / 3, 200), 1.f);
            renderer.drawText(window, graphs[i].title, GX + 6, gy + 5, 14, graphs[i].color);
            renderer.drawText(window, graphs[i].desc, GX + 60, gy + 6, 12,
                              sf::Color(graphs[i].color.r, graphs[i].color.g, graphs[i].color.b, 110));

            sf::Vector2f gPos(GX + 3, gy + 22), gSz(GW - 6, GH - 26);
            (renderer.*graphs[i].fn)(window, *graphs[i].dataA, gPos, gSz, graphs[i].color);

            if (sim.showSecond && graphs[i].dataB && !graphs[i].dataB->empty())
            {
                sf::Color c2 = graphs[i].color;
                c2.a = 80;
                (renderer.*graphs[i].fn)(window, *graphs[i].dataB, gPos, gSz, c2);
            }
        }

        // ── СТАТУС-БАР ──────────────────────────────────────────────
        float sy = H - BOT;
        box(window, 0, sy, W, BOT, sf::Color(14, 17, 26));
        hline(window, 0, sy, W, sf::Color(32, 44, 72));
        float ty = sy + (BOT - 14.f) / 2.f;

        renderer.drawText(window,
                          "Foucault Pendulum  |  Runge-Kutta 4  |  KubSU FTF 2026",
                          PAD, ty, 12, sf::Color(40, 54, 85));
        {
            std::ostringstream ss;
            ss << "OmegaZ = " << std::scientific << std::setprecision(3)
               << sim.physics.getOmegaZ() << " rad/s";
            renderer.drawText(window, ss.str(), 440, ty, 12, sf::Color(44, 62, 102));
        }
        renderer.drawText(window,
                          "Pts: " + std::to_string(sim.historyX.size()),
                          800, ty, 12, sf::Color(40, 54, 85));
        renderer.drawText(window,
                          "Space=pause  R=reset  +/-=speed  2=ch.B  P=params",
                          900, ty, 11, sf::Color(36, 48, 76));

        // ── РАЗДЕЛИТЕЛЬ ─────────────────────────────────────────────
        vline(window, LEFT, TOP, H - TOP, sf::Color(32, 48, 82));

        // ── ПАНЕЛЬ ПАРАМЕТРОВ (поверх всего) ────────────────────────
        controller.getInput(renderer, window, sim);

        window.display();
    }
    return 0;
}