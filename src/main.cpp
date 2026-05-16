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
// Размеры окна
// ============================================================
static const float WIN_W = 1400.f;
static const float WIN_H = 900.f;
static const float TOP_H = 55.f;   // верхняя панель (заголовок)
static const float BOT_H = 40.f;   // нижняя панель (статус)
static const float LEFT_W = 460.f; // левая панель (маятник)
static const float BTN_H = 38.f;   // высота кнопок управления

// Рабочая область (между шапкой и низом)
static const float WORK_Y = TOP_H + 2.f;
// Кнопки управления идут снизу левой панели — 4 кнопки
static const float CTRL_Y = WIN_H - BOT_H - BTN_H - 4.f;
// Траектория занимает пространство выше кнопок
static const float TRAJ_H = CTRL_Y - WORK_Y - 2.f;

// Правая панель — три графика
static const float GX = LEFT_W + 2.f;
static const float GW = WIN_W - GX - 2.f;
static const float WORK_H = WIN_H - TOP_H - BOT_H - 4.f;
static const float GH = (WORK_H - 4.f) / 3.f - 2.f;

// ============================================================
// Вспомогательные функции рисования
// ============================================================

static void fillRect(sf::RenderWindow &w, float x, float y,
                     float wd, float ht, sf::Color c)
{
    sf::RectangleShape r(sf::Vector2f(wd, ht));
    r.setPosition(sf::Vector2f(x, y));
    r.setFillColor(c);
    w.draw(r);
}

static void hline(sf::RenderWindow &w, float x, float y,
                  float len, sf::Color c)
{
    fillRect(w, x, y, len, 1.f, c);
}

static void vline(sf::RenderWindow &w, float x, float y,
                  float len, sf::Color c)
{
    fillRect(w, x, y, 1.f, len, c);
}

static void drawBorder(sf::RenderWindow &w, float x, float y,
                       float wd, float ht, sf::Color c, float t = 1.f)
{
    sf::RectangleShape r(sf::Vector2f(wd, ht));
    r.setPosition(sf::Vector2f(x, y));
    r.setFillColor(sf::Color::Transparent);
    r.setOutlineThickness(t);
    r.setOutlineColor(c);
    w.draw(r);
}

// Число в строку
static std::string fmt(double v, int p = 1)
{
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(p) << v;
    return ss.str();
}

// ============================================================
// Индикатор угла прецессии (п.10 ТЗ)
// ============================================================
static void drawAngleIndicator(sf::RenderWindow &w, Renderer &r,
                               float cx, float cy, float rad,
                               double alpha)
{
    // Фон
    sf::CircleShape bg(rad);
    bg.setOrigin(sf::Vector2f(rad, rad));
    bg.setPosition(sf::Vector2f(cx, cy));
    bg.setFillColor(sf::Color(16, 20, 32));
    bg.setOutlineThickness(1.f);
    bg.setOutlineColor(sf::Color(50, 68, 115));
    w.draw(bg);

    // Засечки (12 штук)
    for (int i = 0; i < 12; i++)
    {
        float a = (float)i * (float)M_PI / 6.f;
        float r0 = rad - 5, r1 = rad - 1;
        sf::VertexArray t(sf::PrimitiveType::LineStrip, 2);
        t[0].position = {cx + r0 * std::cos(a), cy + r0 * std::sin(a)};
        t[0].color = sf::Color(45, 60, 95);
        t[1].position = {cx + r1 * std::cos(a), cy + r1 * std::sin(a)};
        t[1].color = sf::Color(45, 60, 95);
        w.draw(t);
    }

    // Начальное направление
    sf::VertexArray ref(sf::PrimitiveType::LineStrip, 2);
    ref[0].position = {cx, cy};
    ref[0].color = sf::Color(48, 62, 100);
    ref[1].position = {cx + rad * 0.85f, cy};
    ref[1].color = sf::Color(48, 62, 100);
    w.draw(ref);

    // Дуга пройденного угла
    if (std::abs(alpha) > 0.001)
    {
        sf::VertexArray arc(sf::PrimitiveType::LineStrip);
        for (int i = 0; i <= 32; i++)
        {
            float a = (float)alpha * i / 32.f;
            sf::Vertex v;
            v.position = {cx + rad * 0.62f * std::cos(a),
                          cy + rad * 0.62f * std::sin(a)};
            v.color = sf::Color(235, 195, 48, 130);
            arc.append(v);
        }
        w.draw(arc);
    }

    // Стрелка
    float ax = cx + rad * 0.80f * (float)std::cos(alpha);
    float ay = cy + rad * 0.80f * (float)std::sin(alpha);
    sf::VertexArray arr(sf::PrimitiveType::LineStrip, 2);
    arr[0].position = {cx, cy};
    arr[0].color = sf::Color(235, 200, 50);
    arr[1].position = {ax, ay};
    arr[1].color = sf::Color(235, 200, 50, 160);
    w.draw(arr);

    // Подпись и значение угла (только ASCII)
    r.drawText(w, "Precession", cx - rad, cy - rad - 16, 13,
               sf::Color(165, 150, 55));
    double deg = alpha * 180.0 / M_PI;
    r.drawText(w, fmt(deg, 2) + " deg", cx - rad, cy + rad + 4, 13,
               sf::Color(235, 200, 50));
}

// ============================================================
// Нарисовать одну кнопку управления
// Возвращает true если на неё кликнули
// ============================================================
static sf::FloatRect drawCtrlButton(sf::RenderWindow &w, Renderer &r,
                                    const std::string &label,
                                    float x, float y, float wd, float ht,
                                    sf::Color bg, sf::Color textCol)
{
    sf::Vector2f mouse(sf::Mouse::getPosition(w));
    sf::FloatRect rect(sf::Vector2f(x, y), sf::Vector2f(wd, ht));

    // Подсветка при наведении
    if (rect.contains(mouse))
    {
        bg.r = (uint8_t)std::min(255, (int)bg.r + 25);
        bg.g = (uint8_t)std::min(255, (int)bg.g + 25);
        bg.b = (uint8_t)std::min(255, (int)bg.b + 25);
    }

    fillRect(w, x, y, wd, ht, bg);
    drawBorder(w, x, y, wd, ht,
               sf::Color(bg.r / 2 + 70, bg.g / 2 + 70, bg.b / 2 + 70));

    // Текст по центру кнопки
    // (sf::Text не даёт точный размер без лишних зависимостей,
    //  поэтому центрируем приближённо)
    r.drawText(w, label,
               x + 6, y + (ht - 16) / 2.f,
               15, textCol);

    return rect;
}

// ============================================================
// MAIN
// ============================================================
int main()
{
    std::cout << "Starting Foucault Pendulum simulation..." << std::endl;

    // --- Создать окно ---
    std::string titleStr = "Foucault Pendulum Simulation";
    sf::RenderWindow window(
        sf::VideoMode(sf::Vector2u((unsigned)WIN_W, (unsigned)WIN_H)),
        titleStr);
    window.setFramerateLimit(60);

    // Фиксируем логическое разрешение = физическому.
    // На Retina macOS окно может быть 2x, поэтому явно задаём View
    // чтобы координаты соответствовали пикселям и текст не размывался.
    window.setView(sf::View(sf::FloatRect(
        sf::Vector2f(0.f, 0.f),
        sf::Vector2f(WIN_W, WIN_H))));

    // --- Создать объекты ---
    Simulation sim;
    Renderer renderer;
    Controller controller;

    // --- Загрузить шрифт ---
    // Ищем шрифт с поддержкой кириллицы.
    // Лучший вариант: положить DejaVuSans.ttf рядом с exe
    // (скачать с https://dejavu-fonts.github.io/)
    bool fontOk = false;
    std::vector<std::string> fontPaths = {
        // Рядом с exe (рекомендуется - поддерживает кириллицу)
        "DejaVuSans.ttf",
        "font.ttf",
        "arial.ttf",
        // macOS - шрифты с поддержкой кириллицы
        "/System/Library/Fonts/Supplemental/Arial Unicode.ttf",
        "/Library/Fonts/Arial Unicode.ttf",
        "/System/Library/Fonts/Supplemental/Verdana.ttf",
        "/Library/Fonts/Microsoft/Arial.ttf",
        // macOS Homebrew / пользовательские шрифты
        "/usr/local/share/fonts/DejaVuSans.ttf",
        "/opt/homebrew/share/fonts/dejavu-fonts/DejaVuSans.ttf",
        // Linux
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        // Windows
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/segoeui.ttf",
    };
    for (const auto &path : fontPaths)
    {
        if (renderer.loadFont(path))
        {
            fontOk = true;
            std::cout << "Font loaded: " << path << std::endl;
            break;
        }
    }
    if (!fontOk)
    {
        std::cerr << "Font not found!" << std::endl;
        std::cerr << "Place DejaVuSans.ttf next to the FoucaultPendulum executable." << std::endl;
        std::cerr << "Download at: https://dejavu-fonts.github.io/" << std::endl;
        return 1;
    }

    // --- Запустить симуляцию ---
    sim.start();

    sf::Clock clock;
    bool paused = false;

    // Прямоугольники кнопок управления (заполняются при рисовании)
    sf::FloatRect btnPause, btnReset, btnSpeedUp, btnSpeedDn, btnParams;

    // ============================================================
    // Главный цикл (по диаграмме активности UML)
    // ============================================================
    while (window.isOpen())
    {
        // --- Обработка событий ---
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
                window.close();

            if (const auto *te = event->getIf<sf::Event::TextEntered>())
                controller.handleTextEntered(te->unicode);

            if (const auto *k = event->getIf<sf::Event::KeyPressed>())
                controller.handleKey(k->code, sim, paused);

            if (const auto *mc = event->getIf<sf::Event::MouseButtonPressed>())
            {
                sf::Vector2f pos((float)mc->position.x,
                                 (float)mc->position.y);

                // Кнопки управления в интерфейсе
                if (btnPause.contains(pos))
                    paused = !paused;
                if (btnReset.contains(pos))
                {
                    controller.handleStop(sim);
                    paused = false;
                }
                if (btnSpeedUp.contains(pos))
                    sim.timeScale = std::min(20.0, sim.timeScale * 1.25);
                if (btnSpeedDn.contains(pos))
                    sim.timeScale = std::max(0.1, sim.timeScale / 1.25);
                if (btnParams.contains(pos))
                {
                    controller.panelOpen = !controller.panelOpen;
                    if (controller.panelOpen)
                        controller.syncFrom(sim);
                }

                controller.handleClick(pos, sim);
            }
        }

        // --- Применить параметры если нажали "Применить" ---
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

        // --- Обновить симуляцию ---
        if (!paused)
        {
            double dt = clock.restart().asSeconds();
            if (dt > 0.1)
                dt = 0.1;
            sim.update(dt);
        }
        else
        {
            clock.restart();
        }

        // ============================================================
        // ОТРИСОВКА
        // ============================================================
        window.clear(sf::Color(13, 15, 22));

        // ── ВЕРХНЯЯ ПАНЕЛЬ (шапка) ──────────────────────────────────
        fillRect(window, 0, 0, WIN_W, TOP_H, sf::Color(17, 20, 33));
        hline(window, 0, TOP_H - 1, WIN_W, sf::Color(46, 66, 110));

        // Название
        renderer.drawText(window, "Foucault Pendulum",
                          12, 8, 22, sf::Color(90, 155, 240));
        renderer.drawText(window, "Simulation | RK4 | Coriolis force",
                          12, 34, 13, sf::Color(48, 72, 120));

        // Скорость
        renderer.drawText(window, "Speed:", 300, 10, 13,
                          sf::Color(110, 120, 145));
        renderer.drawText(window, fmt(sim.timeScale) + "x",
                          300, 28, 16, sf::Color(215, 205, 75));

        // Время симуляции
        renderer.drawText(window, "Time:", 415, 10, 13,
                          sf::Color(110, 120, 145));
        renderer.drawText(window, fmt(sim.time) + " s",
                          415, 28, 16, sf::Color(130, 200, 255));

        // Широты
        renderer.drawText(window, "Lat. A:", 545, 10, 13,
                          sf::Color(70, 165, 90));
        renderer.drawText(window, fmt(sim.lat1) + "d",
                          545, 28, 16, sf::Color(80, 220, 110));

        renderer.drawText(window, "Lat. B:", 650, 10, 13,
                          sim.showSecond ? sf::Color(145, 95, 235)
                                         : sf::Color(50, 50, 70));
        renderer.drawText(window,
                          sim.showSecond ? fmt(sim.lat2) + "d" : "off",
                          650, 28, 16,
                          sim.showSecond ? sf::Color(175, 130, 250)
                                         : sf::Color(50, 50, 70));

        // Статус
        renderer.drawText(window, paused ? "|| PAUSED" : "> RUNNING",
                          780, 20, 16,
                          paused ? sf::Color(230, 80, 80)
                                 : sf::Color(60, 210, 110));

        // ── ЛЕВАЯ ПАНЕЛЬ: ТРАЕКТОРИЯ ────────────────────────────────
        float lx = 2.f, ly = WORK_Y;
        float lw = LEFT_W - 4.f;

        fillRect(window, lx, ly, lw, TRAJ_H, sf::Color(13, 16, 25));
        drawBorder(window, lx, ly, lw, TRAJ_H, sf::Color(45, 88, 165));

        renderer.drawText(window, "Trajectory",
                          lx + 8, ly + 6, 15, sf::Color(68, 130, 215));

        // Сетка
        for (int i = 1; i < 6; i++)
        {
            sf::Color gc(22, 28, 46);
            vline(window, lx + i * lw / 6, ly + 1, TRAJ_H - 2, gc);
            hline(window, lx + 1, ly + i * TRAJ_H / 6, lw - 2, gc);
        }

        // Оси
        float axCx = lx + lw * 0.5f;
        float axCy = ly + TRAJ_H * 0.44f;
        hline(window, lx + 6, axCy, lw - 12, sf::Color(36, 48, 76));
        vline(window, axCx, ly + 6, TRAJ_H - 12, sf::Color(36, 48, 76));
        renderer.drawText(window, "X", lx + lw - 16, axCy + 4, 13,
                          sf::Color(50, 68, 108));
        renderer.drawText(window, "Y", axCx + 4, ly + 6, 13,
                          sf::Color(50, 68, 108));

        float scale = 850.f; // пикселей на метр

        // Траектории
        renderer.drawTrajectory(window,
                                sim.getTrajectoryX(),
                                sim.getTrajectoryY(),
                                sf::Color(48, 215, 145),
                                axCx, axCy, scale);
        if (sim.showSecond)
            renderer.drawTrajectory(window,
                                    sim.historyX2,
                                    sim.historyY2,
                                    sf::Color(175, 100, 250),
                                    axCx, axCy, scale);

        // Маятник (по UML: renderer.drawPendulum)
        renderer.drawPendulum(window, sim.pendulum, axCx, axCy, scale);

        // Индикатор угла прецессии (п.10 ТЗ)
        {
            float icx = lx + 52.f;
            float icy = ly + TRAJ_H - 68.f;
            drawAngleIndicator(window, renderer, icx, icy, 42.f,
                               sim.getCurrentAlpha());
        }

        // Координаты и легенда
        renderer.drawText(window,
                          "x=" + fmt(sim.pendulum.x, 3) +
                              "  y=" + fmt(sim.pendulum.y, 3),
                          lx + 108, ly + TRAJ_H - 14, 13,
                          sf::Color(88, 140, 112, 190));
        if (sim.showSecond)
        {
            fillRect(window, lx + 108, ly + TRAJ_H - 32, 14, 3,
                     sf::Color(48, 215, 145));
            renderer.drawText(window, fmt(sim.lat1, 0) + "d",
                              lx + 126, ly + TRAJ_H - 36, 13,
                              sf::Color(48, 215, 145));
            fillRect(window, lx + 170, ly + TRAJ_H - 32, 14, 3,
                     sf::Color(175, 100, 250));
            renderer.drawText(window, fmt(sim.lat2, 0) + "d",
                              lx + 188, ly + TRAJ_H - 36, 13,
                              sf::Color(175, 100, 250));
        }

        // ── КНОПКИ УПРАВЛЕНИЯ (под траекторией) ─────────────────────
        // 4 кнопки в ряд: ПАУЗА | СБРОС | СКОРОСТЬ- | СКОРОСТЬ+
        {
            float bw = (lw - 3.f) / 4.f; // ширина одной кнопки
            float by = CTRL_Y;

            // ПАУЗА / ПРОДОЛЖИТЬ
            sf::Color pauseCol = paused ? sf::Color(38, 120, 38)
                                        : sf::Color(130, 50, 38);
            btnPause = drawCtrlButton(window, renderer,
                                      paused ? "> Resume" : "|| Pause",
                                      lx, by, bw, BTN_H,
                                      pauseCol, sf::Color(220, 240, 220));

            // RESET
            btnReset = drawCtrlButton(window, renderer,
                                      "R Reset",
                                      lx + bw + 1, by, bw, BTN_H,
                                      sf::Color(50, 50, 110), sf::Color(200, 210, 255));

            // SPEED -
            btnSpeedDn = drawCtrlButton(window, renderer,
                                        "- Speed",
                                        lx + 2 * (bw + 1), by, bw, BTN_H,
                                        sf::Color(60, 55, 25), sf::Color(230, 220, 140));

            // SPEED +
            btnSpeedUp = drawCtrlButton(window, renderer,
                                        "+ Speed",
                                        lx + 3 * (bw + 1), by, bw, BTN_H,
                                        sf::Color(60, 55, 25), sf::Color(230, 220, 140));
        }

        // ── КНОПКА ПАРАМЕТРОВ (отдельно, между левой и правой) ─────
        {
            float by = WORK_Y;
            btnParams = drawCtrlButton(window, renderer,
                                       "[P] Parameters",
                                       lx, by, lw, 28,
                                       sf::Color(28, 40, 78), sf::Color(160, 185, 240));
        }
        // Сдвигаем траекторию вниз — уже учтено через WORK_Y + 30 ниже
        // (оставляем кнопку поверх рамки, она отдельная полоска)

        // ── ПРАВАЯ ПАНЕЛЬ: ТРИ ГРАФИКА ──────────────────────────────
        struct GraphInfo
        {
            const std::vector<double> *data;
            sf::Color color;
            const char *title;
        };

        GraphInfo graphs[3] = {
            {&sim.historyX, sf::Color(245, 92, 92), "x(t)"},
            {&sim.historyY, sf::Color(72, 220, 110), "y(t)"},
            {&sim.historyAlpha, sf::Color(245, 215, 48), "a(t)"},
        };

        using DrawFn = void (Renderer::*)(sf::RenderWindow &,
                                          const std::vector<double> &,
                                          sf::Vector2f, sf::Vector2f, sf::Color);
        DrawFn drawFns[3] = {
            &Renderer::drawGraphXvsTime,
            &Renderer::drawGraphYvsTime,
            &Renderer::drawGraphAlphaVsTime,
        };

        for (int i = 0; i < 3; i++)
        {
            float gy = WORK_Y + i * (GH + 3.f);

            fillRect(window, GX, gy, GW, GH, sf::Color(12, 15, 25));
            drawBorder(window, GX, gy, GW, GH,
                       sf::Color(graphs[i].color.r / 2,
                                 graphs[i].color.g / 2,
                                 graphs[i].color.b / 2, 190));

            // Заголовок (badge над рамкой)
            float tw = strlen(graphs[i].title) * 10.f + 10;
            fillRect(window, GX + 8, gy - 9, tw, 13, sf::Color(12, 15, 25));
            renderer.drawText(window, graphs[i].title,
                              GX + 10, gy - 8, 14, graphs[i].color);

            // Подпись
            const char *desc[3] = {
                "Displacement X, m",
                "Displacement Y, m",
                "Precession angle a, rad"};
            renderer.drawText(window, desc[i],
                              GX + 10, gy + 4, 13,
                              sf::Color(graphs[i].color.r,
                                        graphs[i].color.g,
                                        graphs[i].color.b, 110));

            sf::Vector2f gPos(GX + 4, gy + 18);
            sf::Vector2f gSz(GW - 8, GH - 22);

            (renderer.*drawFns[i])(window, *graphs[i].data,
                                   gPos, gSz, graphs[i].color);

            // Второй канал (п.8 ТЗ)
            if (sim.showSecond && i < 2)
            {
                const std::vector<double> *d2 =
                    (i == 0) ? &sim.historyX2 : &sim.historyY2;
                if (!d2->empty())
                {
                    sf::Color c2 = graphs[i].color;
                    c2.a = 95;
                    (renderer.*drawFns[i])(window, *d2,
                                           gPos, gSz, c2);
                }
            }
        }

        // ── НИЖНЯЯ ПАНЕЛЬ ───────────────────────────────────────────
        float by = WIN_H - BOT_H;
        fillRect(window, 0, by, WIN_W, BOT_H, sf::Color(11, 14, 23));
        hline(window, 0, by, WIN_W, sf::Color(34, 48, 80));

        float ty = by + (BOT_H - 14.f) / 2.f;

        renderer.drawText(window,
                          "Foucault Pendulum  |  Runge-Kutta 4  |  KubSU FTF 2026",
                          12, ty, 13, sf::Color(42, 56, 88));

        {
            std::ostringstream ss;
            ss << "OmegaZ = " << std::scientific << std::setprecision(3)
               << sim.physics.getOmegaZ() << " rad/s";
            renderer.drawText(window, ss.str(),
                              440, ty, 13, sf::Color(46, 65, 105));
        }

        renderer.drawText(window,
                          "Pts: " + std::to_string(sim.historyX.size()),
                          860, ty, 13, sf::Color(42, 56, 88));

        renderer.drawText(window,
                          "Keys: Space=pause  R=reset  +/-=speed  2=ch.B  P=params",
                          950, ty, 13, sf::Color(38, 50, 76));

        // ── Разделитель ─────────────────────────────────────────────
        vline(window, LEFT_W, WORK_Y, WORK_H, sf::Color(34, 54, 90));

        // ── Панель параметров (поверх всего) ────────────────────────
        // По UML: Controller.getInput() вызывается каждый кадр
        controller.getInput(renderer, window, sim);

        window.display();
    }

    std::cout << "Done." << std::endl;
    return 0;
}