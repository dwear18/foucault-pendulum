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
// Размеры окна и области
// ============================================================
static const float WIN_W = 1400.f;
static const float WIN_H = 900.f;
static const float TOP_H = 60.f;   // высота верхней панели
static const float BOT_H = 44.f;   // высота нижней панели
static const float LEFT_W = 460.f; // ширина левой панели (траектория)

// Рабочая область (между верхней и нижней панелями)
static const float WORK_Y = TOP_H + 2.f;
static const float WORK_H = WIN_H - TOP_H - BOT_H - 4.f;

// Правая часть - три графика
static const float GX = LEFT_W + 2.f;
static const float GW = WIN_W - GX - 2.f;
static const float GH = (WORK_H - 4.f) / 3.f - 2.f;

// ============================================================
// Вспомогательные функции
// ============================================================

// Нарисовать горизонтальную линию
static void hline(sf::RenderWindow &w, float x, float y,
                  float len, sf::Color c)
{
    sf::RectangleShape r(sf::Vector2f(len, 1));
    r.setPosition(sf::Vector2f(x, y));
    r.setFillColor(c);
    w.draw(r);
}

// Нарисовать вертикальную линию
static void vline(sf::RenderWindow &w, float x, float y,
                  float len, sf::Color c)
{
    sf::RectangleShape r(sf::Vector2f(1, len));
    r.setPosition(sf::Vector2f(x, y));
    r.setFillColor(c);
    w.draw(r);
}

// Нарисовать прямоугольник
static void fillRect(sf::RenderWindow &w, float x, float y,
                     float wd, float ht, sf::Color c)
{
    sf::RectangleShape r(sf::Vector2f(wd, ht));
    r.setPosition(sf::Vector2f(x, y));
    r.setFillColor(c);
    w.draw(r);
}

// Число в строку
static std::string fmt(double v, int p = 1)
{
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(p) << v;
    return ss.str();
}

// Нарисовать рамку с цветной обводкой
static void drawBorder(sf::RenderWindow &w, float x, float y,
                       float wd, float ht, sf::Color c)
{
    sf::RectangleShape b(sf::Vector2f(wd, ht));
    b.setPosition(sf::Vector2f(x, y));
    b.setFillColor(sf::Color::Transparent);
    b.setOutlineThickness(1.f);
    b.setOutlineColor(c);
    w.draw(b);
}

// Нарисовать индикатор угла прецессии (п.10 ТЗ)
// cx,cy - центр круга, r - радиус, alpha - угол в радианах
static void drawAngleIndicator(sf::RenderWindow &w, Renderer &renderer,
                               float cx, float cy, float r, double alpha)
{
    // Фон круга
    sf::CircleShape bg(r);
    bg.setOrigin(sf::Vector2f(r, r));
    bg.setPosition(sf::Vector2f(cx, cy));
    bg.setFillColor(sf::Color(18, 22, 34));
    bg.setOutlineThickness(1.f);
    bg.setOutlineColor(sf::Color(55, 72, 120));
    w.draw(bg);

    // Засечки по кругу (12 штук как на часах)
    for (int i = 0; i < 12; i++)
    {
        float a = (float)i * (float)M_PI / 6.f;
        float r0 = r - 5, r1 = r - 1;
        sf::VertexArray t(sf::PrimitiveType::LineStrip, 2);
        t[0].position = {cx + r0 * std::cos(a), cy + r0 * std::sin(a)};
        t[0].color = sf::Color(48, 62, 100);
        t[1].position = {cx + r1 * std::cos(a), cy + r1 * std::sin(a)};
        t[1].color = sf::Color(48, 62, 100);
        w.draw(t);
    }

    // Начальное направление (горизонталь)
    sf::VertexArray ref(sf::PrimitiveType::LineStrip, 2);
    ref[0].position = {cx, cy};
    ref[0].color = sf::Color(50, 65, 105);
    ref[1].position = {cx + r * 0.85f, cy};
    ref[1].color = sf::Color(50, 65, 105);
    w.draw(ref);

    // Дуга пройденного угла
    if (std::abs(alpha) > 0.001)
    {
        sf::VertexArray arc(sf::PrimitiveType::LineStrip);
        int segs = 32;
        for (int i = 0; i <= segs; i++)
        {
            float a = (float)alpha * i / segs;
            sf::Vertex v;
            v.position = {cx + r * 0.65f * std::cos(a),
                          cy + r * 0.65f * std::sin(a)};
            v.color = sf::Color(240, 200, 50, 130);
            arc.append(v);
        }
        w.draw(arc);
    }

    // Стрелка текущего направления
    float ax = cx + r * 0.82f * (float)std::cos(alpha);
    float ay = cy + r * 0.82f * (float)std::sin(alpha);
    sf::VertexArray arr(sf::PrimitiveType::LineStrip, 2);
    arr[0].position = {cx, cy};
    arr[0].color = sf::Color(240, 205, 55);
    arr[1].position = {ax, ay};
    arr[1].color = sf::Color(240, 205, 55, 160);
    w.draw(arr);

    // Подпись и значение
    renderer.drawText(w, "PRECESSION", cx - r, cy - r - 14,
                      1.4f, sf::Color(170, 155, 55));
    double deg = alpha * 180.0 / M_PI;
    renderer.drawText(w, fmt(deg, 2) + " deg",
                      cx - r, cy + r + 5,
                      1.4f, sf::Color(240, 205, 55));
}

// ============================================================
// MAIN
// ============================================================
int main()
{
    std::cout << "Starting Foucault Pendulum..." << std::endl;

    // --- Создать окно ---
    sf::RenderWindow window(
        sf::VideoMode(sf::Vector2u((unsigned)WIN_W, (unsigned)WIN_H)),
        "Foucault Pendulum Simulation");
    window.setFramerateLimit(60);

    // --- Создать объекты по UML ---
    Simulation sim;
    Renderer renderer;
    Controller controller;

    // --- Запустить симуляцию с начальными параметрами ---
    sim.start();

    sf::Clock clock;
    bool paused = false;

    // ============================================================
    // Главный цикл (соответствует диаграмме активности UML)
    // ============================================================
    while (window.isOpen())
    {
        // --- Обработка событий ---
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
                window.close();

            // Ввод символов (для панели параметров)
            if (const auto *te = event->getIf<sf::Event::TextEntered>())
                controller.handleTextEntered(te->unicode);

            // Нажатие клавиш
            if (const auto *k = event->getIf<sf::Event::KeyPressed>())
                controller.handleKey(k->code, sim, paused);

            // Клик мышью
            if (const auto *mc = event->getIf<sf::Event::MouseButtonPressed>())
            {
                sf::Vector2f pos((float)mc->position.x,
                                 (float)mc->position.y);

                // Клик по кнопке [P] в топ-баре
                if (pos.x > 625 && pos.x < 720 &&
                    pos.y > 8 && pos.y < 52)
                {
                    controller.panelOpen = !controller.panelOpen;
                    if (controller.panelOpen)
                        controller.syncFrom(sim);
                }

                controller.handleClick(pos, sim);
            }
        }

        // --- Применить параметры если нажали Apply ---
        if (controller.applyNow)
        {
            controller.applyNow = false;
            if (controller.applyTo(sim))
            {
                controller.handleStart(sim); // перезапустить симуляцию
                controller.panelOpen = false;
            }
        }

        // --- Обновить симуляцию (если не на паузе) ---
        if (!paused)
        {
            double dt = clock.restart().asSeconds();
            if (dt > 0.1)
                dt = 0.1; // защита от слишком большого шага
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

        // ── ВЕРХНЯЯ ПАНЕЛЬ ──────────────────────────────────────────
        // Все метки на y=10, все значения на y=28 (единая сетка)
        fillRect(window, 0, 0, WIN_W, TOP_H, sf::Color(17, 20, 33));
        hline(window, 0, TOP_H - 1, WIN_W, sf::Color(48, 68, 115));

        // Название программы
        renderer.drawText(window, "FOUCAULT PENDULUM",
                          12, 10, 2.5f, sf::Color(95, 160, 240));
        renderer.drawText(window, "Simulation | RK4 | Coriolis",
                          12, 38, 1.4f, sf::Color(50, 75, 125));

        // Статус (RUNNING / PAUSED)
        {
            sf::CircleShape dot(5.f);
            dot.setOrigin(sf::Vector2f(5.f, 5.f));
            dot.setPosition(sf::Vector2f(298.f, 29.f));
            dot.setFillColor(paused ? sf::Color(235, 65, 65)
                                    : sf::Color(65, 215, 115));
            window.draw(dot);
        }
        renderer.drawText(window, paused ? "PAUSED" : "RUNNING",
                          312, 22, 1.6f,
                          paused ? sf::Color(235, 90, 90)
                                 : sf::Color(65, 215, 115));

        // Скорость симуляции
        renderer.drawText(window, "SPEED", 430, 10, 1.4f,
                          sf::Color(105, 115, 140));
        renderer.drawText(window, fmt(sim.timeScale) + "x",
                          430, 28, 2.2f, sf::Color(215, 205, 75));
        fillRect(window, 498, 31, 80, 6, sf::Color(27, 31, 50));
        fillRect(window, 498, 31,
                 std::min(80.f, (float)sim.timeScale / 20.f * 80.f), 6,
                 sf::Color(195, 190, 68, 200));

        // Кнопка параметров [P]
        fillRect(window, 625, 8, 92, 44, sf::Color(27, 42, 80));
        drawBorder(window, 625, 8, 92, 44, sf::Color(62, 108, 195));
        renderer.drawText(window, "[P]", 633, 10, 2.0f,
                          sf::Color(95, 155, 235));
        renderer.drawText(window, "PARAMS", 633, 33, 1.3f,
                          sf::Color(75, 115, 185));

        // Время симуляции
        renderer.drawText(window, "TIME", 730, 10, 1.4f,
                          sf::Color(105, 115, 140));
        renderer.drawText(window, fmt(sim.time) + "s",
                          730, 28, 2.2f, sf::Color(135, 205, 255));

        // Широта A
        renderer.drawText(window, "LAT.A", 875, 10, 1.4f,
                          sf::Color(75, 170, 95));
        renderer.drawText(window, fmt(sim.lat1) + "d",
                          875, 28, 2.2f, sf::Color(85, 225, 115));

        // Широта B
        renderer.drawText(window, "LAT.B", 980, 10, 1.4f,
                          sim.showSecond ? sf::Color(150, 100, 235)
                                         : sf::Color(52, 52, 72));
        renderer.drawText(window,
                          sim.showSecond ? fmt(sim.lat2) + "d" : "OFF",
                          980, 28, 2.2f,
                          sim.showSecond ? sf::Color(180, 135, 250)
                                         : sf::Color(52, 52, 72));

        // Подсказка по клавишам
        renderer.drawText(window,
                          "SPC:pause  R:reset  +/-:speed  2:ch.B",
                          1095, 22, 1.3f, sf::Color(44, 58, 90));

        // ── ЛЕВАЯ ПАНЕЛЬ: ТРАЕКТОРИЯ ────────────────────────────────
        float lx = 2.f, ly = WORK_Y;
        float lw = LEFT_W - 4.f, lh = WORK_H;

        fillRect(window, lx, ly, lw, lh, sf::Color(14, 17, 26));
        drawBorder(window, lx, ly, lw, lh, sf::Color(47, 90, 168));

        renderer.drawText(window, "TRAJECTORY",
                          lx + 8, ly + 8, 2.0f, sf::Color(70, 135, 220));

        // Сетка в области траектории
        for (int i = 1; i < 6; i++)
        {
            sf::Color gc(24, 30, 48);
            vline(window, lx + i * lw / 6, ly + 1, lh - 2, gc);
            hline(window, lx + 1, ly + i * lh / 6, lw - 2, gc);
        }

        // Оси через центр подвеса
        float axCx = lx + lw * 0.5f;  // центр X
        float axCy = ly + lh * 0.46f; // центр Y (чуть выше середины)
        hline(window, lx + 6, axCy, lw - 12, sf::Color(38, 50, 80));
        vline(window, axCx, ly + 6, lh - 12, sf::Color(38, 50, 80));

        // Метки осей
        renderer.drawText(window, "X", lx + lw - 14, axCy + 4,
                          1.4f, sf::Color(52, 70, 110));
        renderer.drawText(window, "Y", axCx + 4, ly + 6,
                          1.4f, sf::Color(52, 70, 110));

        // Масштаб: 1 метр = scale пикселей
        // Подобран так чтобы x0=0.1 было хорошо видно
        float scale = 850.f;

        // Траектория широты A (зелёная)
        renderer.drawTrajectory(window,
                                sim.getTrajectoryX(),
                                sim.getTrajectoryY(),
                                sf::Color(50, 220, 150),
                                axCx, axCy, scale);

        // Траектория широты B (фиолетовая) если включена
        if (sim.showSecond)
            renderer.drawTrajectory(window,
                                    sim.historyX2,
                                    sim.historyY2,
                                    sf::Color(180, 105, 255),
                                    axCx, axCy, scale);

        // Маятник (основной, широта A)
        renderer.drawPendulum(window, sim.pendulum,
                              axCx, axCy, scale);

        // Индикатор угла прецессии (п.10 ТЗ) - левый нижний угол
        {
            float icx = lx + 55.f;
            float icy = ly + lh - 75.f;
            double alpha = sim.getCurrentAlpha();
            drawAngleIndicator(window, renderer, icx, icy, 42.f, alpha);
        }

        // Текущие координаты (мелко, внизу)
        renderer.drawText(window,
                          "x=" + fmt(sim.pendulum.x, 3) +
                              "  y=" + fmt(sim.pendulum.y, 3),
                          lx + 115, ly + lh - 12, 1.4f,
                          sf::Color(90, 145, 115, 180));

        // Легенда широт (только если B включён)
        if (sim.showSecond)
        {
            fillRect(window, lx + 115, ly + lh - 30, 12, 3,
                     sf::Color(50, 220, 150));
            renderer.drawText(window, fmt(sim.lat1, 0) + "d",
                              lx + 131, ly + lh - 34, 1.4f,
                              sf::Color(50, 220, 150));
            fillRect(window, lx + 175, ly + lh - 30, 12, 3,
                     sf::Color(180, 105, 255));
            renderer.drawText(window, fmt(sim.lat2, 0) + "d",
                              lx + 191, ly + lh - 34, 1.4f,
                              sf::Color(180, 105, 255));
        }

        // ── ПРАВАЯ ПАНЕЛЬ: ТРИ ГРАФИКА ──────────────────────────────
        // Данные и настройки для каждого графика
        struct GraphInfo
        {
            const std::vector<double> *data;
            sf::Color color;
            const char *title;
        };

        GraphInfo graphs[3] = {
            {&sim.historyX, sf::Color(250, 95, 95), "X(t)"},
            {&sim.historyY, sf::Color(75, 225, 115), "Y(t)"},
            {&sim.historyAlpha, sf::Color(250, 220, 50), "a(t)"},
        };

        // Три метода рисования графиков (по UML)
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

            // Фон и рамка графика
            fillRect(window, GX, gy, GW, GH, sf::Color(13, 16, 26));
            drawBorder(window, GX, gy, GW, GH,
                       sf::Color(graphs[i].color.r / 2,
                                 graphs[i].color.g / 2,
                                 graphs[i].color.b / 2, 200));

            // Заголовок (в отдельном badge над рамкой)
            float tw = strlen(graphs[i].title) * 12.f + 8;
            fillRect(window, GX + 8, gy - 8, tw, 10,
                     sf::Color(13, 16, 26));
            renderer.drawText(window, graphs[i].title,
                              GX + 10, gy - 7, 2.0f, graphs[i].color);

            // Область графика
            sf::Vector2f gPos(GX + 4, gy + 14);
            sf::Vector2f gSz(GW - 8, GH - 18);

            // Нарисовать основной график (широта A)
            (renderer.*drawFns[i])(window, *graphs[i].data,
                                   gPos, gSz, graphs[i].color);

            // Наложить второй канал если активен (п.8 ТЗ)
            if (sim.showSecond && i < 2)
            {
                const std::vector<double> *data2 =
                    (i == 0) ? &sim.historyX2 : &sim.historyY2;
                if (!data2->empty())
                {
                    sf::Color c2 = graphs[i].color;
                    c2.a = 100;
                    (renderer.*drawFns[i])(window, *data2,
                                           gPos, gSz, c2);
                }
            }
        }

        // ── НИЖНЯЯ ПАНЕЛЬ ───────────────────────────────────────────
        float by = WIN_H - BOT_H;
        fillRect(window, 0, by, WIN_W, BOT_H, sf::Color(12, 15, 24));
        hline(window, 0, by, WIN_W, sf::Color(36, 50, 82));

        // Информация в одну строку по центру высоты
        float ty = by + (BOT_H - 10.f) / 2.f;

        renderer.drawText(window,
                          "Foucault Pendulum  |  RK4  |  Coriolis",
                          12, ty, 1.4f, sf::Color(44, 58, 92));

        {
            std::ostringstream ss;
            ss << "OmegaZ=" << std::scientific << std::setprecision(3)
               << sim.physics.getOmegaZ() << " rad/s";
            renderer.drawText(window, ss.str(),
                              400, ty, 1.4f, sf::Color(48, 68, 108));
        }

        renderer.drawText(window,
                          "pts:" + std::to_string(sim.historyX.size()),
                          880, ty, 1.4f, sf::Color(44, 58, 92));

        // ── РАЗДЕЛИТЕЛИ ─────────────────────────────────────────────
        vline(window, LEFT_W, WORK_Y, WORK_H, sf::Color(36, 56, 92));

        // ── ПАНЕЛЬ ПАРАМЕТРОВ (поверх всего) ────────────────────────
        // По UML: Controller.getInput() вызывается каждый кадр
        controller.getInput(renderer, window, sim);

        window.display();
    }

    std::cout << "Done." << std::endl;
    return 0;
}