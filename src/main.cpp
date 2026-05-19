#include <SFML/Graphics.hpp>
#include <optional> // для window.pollEvent()
#include <cmath>    // M_PI, std::cos и т.д.
#include <sstream>  // ostringstream для форматирования чисел
#include <iomanip>  // std::fixed, std::setprecision
#include <iostream> // std::cerr для ошибок

// Наши классы — по UML-диаграмме
#include "simulation.h"
#include "renderer.h"
#include "controller.h"

// Структура Layout — хранит все координаты интерфейса.
// Пересчитывается каждый кадр из реального размера окна.
// Это позволяет менять размер окна и всё масштабируется.
struct Layout
{
    float W, H; // размер окна в пикселях

    float top;  // высота шапки
    float bot;  // высота статус-бара снизу
    float left; // ширина левой панели

    float pad; // базовый отступ

    // Левая панель: траектория маятника
    float trajX, trajY, trajW, trajH;

    // Кнопки управления (под траекторией)
    float btnY, btnH, btnW;

    // Правая панель: три графика
    float gx, gy0, gw, gh;

    // Базовый размер шрифта (масштабируется с окном)
    float fontSize;
};

// Вычислить Layout из текущего размера окна
static Layout calcLayout(sf::Vector2u winSize)
{
    Layout L;
    L.W = (float)winSize.x;
    L.H = (float)winSize.y;

    // Все размеры — доли от размера окна
    L.pad = std::max(4.f, L.W * 0.004f);
    L.top = std::max(44.f, L.H * 0.062f);
    L.bot = std::max(28.f, L.H * 0.040f);
    L.left = std::max(280.f, L.W * 0.31f);
    L.fontSize = std::max(11.f, std::min(17.f, L.W * 0.011f));

    // Кнопки: считаем СНИЗУ ВВЕРХ, чтобы они всегда были видны
    L.btnH = std::max(30.f, L.H * 0.044f);
    L.btnY = L.H - L.bot - L.pad - L.btnH;
    L.btnW = (L.left - L.pad * 2.f) / 4.f - 2.f;

    // Траектория: от шапки до кнопок
    L.trajX = L.pad;
    L.trajY = L.top + L.pad;
    L.trajW = L.left - L.pad * 2.f;
    L.trajH = L.btnY - L.trajY - L.pad;

    // Три графика справа: занимают всю правую часть
    L.gx = L.left + L.pad;
    L.gw = L.W - L.gx - L.pad;
    L.gy0 = L.top + L.pad;
    L.gh = (L.H - L.top - L.bot - L.pad * 4.f) / 3.f;

    return L;
}

// Вспомогательные функции рисования примитивов

// Закрашенный прямоугольник
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

// Горизонтальная линия (высота 1px)
static void hline(sf::RenderWindow &w, float x, float y,
                  float len, sf::Color c)
{
    box(w, x, y, len, 1.f, c);
}

// Вертикальная линия (ширина 1px)
static void vline(sf::RenderWindow &w, float x, float y,
                  float len, sf::Color c)
{
    box(w, x, y, 1.f, len, c);
}

// Число в строку с нужной точностью
static std::string fmt(double v, int p = 1)
{
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(p) << v;
    return ss.str();
}

// Кнопка управления
// Возвращает свой прямоугольник — используется для проверки клика
static sf::FloatRect drawBtn(sf::RenderWindow &w, Renderer &r,
                             const std::string &label,
                             float x, float y, float wd, float ht,
                             sf::Color bg, sf::Color fg)
{
    sf::FloatRect rc(sf::Vector2f(x, y), sf::Vector2f(wd, ht));

    // Конвертируем позицию мыши в логические координаты
    // (важно для Retina-дисплеев macOS)
    sf::Vector2f mouse = w.mapPixelToCoords(sf::Mouse::getPosition(w));

    // Подсветить кнопку при наведении
    if (rc.contains(mouse))
    {
        bg.r = (uint8_t)std::min(255, (int)bg.r + 25);
        bg.g = (uint8_t)std::min(255, (int)bg.g + 25);
        bg.b = (uint8_t)std::min(255, (int)bg.b + 25);
    }

    // Фон кнопки с тонкой рамкой
    box(w, x, y, wd, ht, bg, sf::Color(255, 255, 255, 30), 1.f);

    // Текст: размер зависит от высоты кнопки
    unsigned int fs = (unsigned int)std::max(11.f, std::min(16.f, ht * 0.40f));
    r.drawText(w, label, x + 8.f, y + (ht - (float)fs) / 2.f, fs, fg);

    return rc;
}

// Индикатор угла прецессии
// Рисует стрелку на круговой шкале, показывающую текущий угол.
// cx, cy — центр круга; rad — радиус; alpha — угол в радианах
static void drawPrecession(sf::RenderWindow &w, Renderer &r,
                           float cx, float cy, float rad,
                           double alpha, float fontSize)
{
    // Фон круга
    sf::CircleShape bg(rad);
    bg.setOrigin(sf::Vector2f(rad, rad));
    bg.setPosition(sf::Vector2f(cx, cy));
    bg.setFillColor(sf::Color(18, 22, 34));
    bg.setOutlineThickness(1.f);
    bg.setOutlineColor(sf::Color(50, 70, 110));
    w.draw(bg);

    // Засечки — 12 штук как на часах
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

    // Начальное направление (горизонталь = 0 радиан)
    sf::VertexArray base(sf::PrimitiveType::LineStrip, 2);
    base[0].position = {cx, cy};
    base[0].color = sf::Color(50, 65, 100);
    base[1].position = {cx + rad * 0.82f, cy};
    base[1].color = sf::Color(50, 65, 100);
    w.draw(base);

    // Дуга — показывает сколько повернулась плоскость колебаний
    if (std::abs(alpha) > 0.001)
    {
        sf::VertexArray arc(sf::PrimitiveType::LineStrip);
        for (int i = 0; i <= 32; i++)
        {
            float a = (float)alpha * i / 32.f;
            sf::Vertex v;
            v.position = {cx + rad * 0.6f * std::cos(a),
                          cy + rad * 0.6f * std::sin(a)};
            v.color = sf::Color(235, 195, 50, 110);
            arc.append(v);
        }
        w.draw(arc);
    }

    // Стрелка текущего направления
    sf::VertexArray arr(sf::PrimitiveType::LineStrip, 2);
    arr[0].position = {cx, cy};
    arr[0].color = sf::Color(235, 200, 55);
    arr[1].position = {cx + rad * 0.78f * (float)std::cos(alpha),
                       cy + rad * 0.78f * (float)std::sin(alpha)};
    arr[1].color = sf::Color(235, 200, 55, 150);
    w.draw(arr);

    // Подпись и числовое значение угла
    unsigned int fs = (unsigned int)std::max(10.f, fontSize * 0.78f);
    r.drawText(w, "Precession",
               cx - rad, cy - rad - (float)fs - 2,
               fs, sf::Color(160, 148, 55));

    double deg = alpha * 180.0 / M_PI;
    r.drawText(w, fmt(deg, 2) + " deg",
               cx - rad, cy + rad + 3,
               fs, sf::Color(235, 200, 55));
}

// Главная функция — точка входа программы
int main()
{
    //  1. Создать окно SFML 
    sf::RenderWindow window(
        sf::VideoMode(sf::Vector2u(1400u, 900u)),
        "Foucault Pendulum Simulation",
        sf::State::Windowed);
    window.setFramerateLimit(60);

    //  2. Создать объекты по UML-диаграмме классов 
    Simulation sim;        // ядро симуляции
    Renderer renderer;     // отрисовщик
    Controller controller; // контроллер ввода

    //  3. Загрузить шрифт 
    // Ищем шрифт последовательно в разных местах.
    // DejaVuSans.ttf поддерживает все нужные символы.
    // Скачать: https://dejavu-fonts.github.io/
    std::vector<std::string> fontPaths = {
        "DejaVuSans.ttf", // рядом с exe (рекомендуется)
        "font.ttf",
        "arial.ttf",
        "/System/Library/Fonts/Supplemental/Arial Unicode.ttf", // macOS
        "/Library/Fonts/Arial Unicode.ttf",
        "/usr/local/share/fonts/DejaVuSans.ttf", // Linux
        "/opt/homebrew/share/fonts/dejavu-fonts/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "C:/Windows/Fonts/arial.ttf", // Windows
    };

    bool fontOk = false;
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
        std::cerr << "Font not found!\n"
                  << "Place DejaVuSans.ttf next to FoucaultPendulum.\n"
                  << "Download: https://dejavu-fonts.github.io/\n";
        return 1;
    }

    //  4. Запустить симуляцию
    sim.start();

    sf::Clock clock;     // таймер для вычисления dt между кадрами
    bool paused = false; // пауза симуляции

    // Прямоугольники кнопок — нужны для обработки кликов.
    // Заполняются при рисовании каждый кадр.
    sf::FloatRect btnPause, btnReset, btnSpeedDn, btnSpeedUp;

    // Главный цикл программы
    while (window.isOpen())
    {
        // Шаг 1: обработать события
        while (const std::optional ev = window.pollEvent())
        {
            // Закрытие окна
            if (ev->is<sf::Event::Closed>())
                window.close();

            // Изменение размера окна — обновляем View
            // Это гарантирует что координаты не сломаются при resize
            if (const auto *rs = ev->getIf<sf::Event::Resized>())
            {
                sf::View view(sf::FloatRect(
                    sf::Vector2f(0, 0),
                    sf::Vector2f((float)rs->size.x, (float)rs->size.y)));
                window.setView(view);
            }

            // Ввод текста (для панели параметров)
            if (const auto *te = ev->getIf<sf::Event::TextEntered>())
                controller.handleTextEntered(te->unicode);

            // Клавиши (по UML: Controller.handleKey)
            if (const auto *k = ev->getIf<sf::Event::KeyPressed>())
                controller.handleKey(k->code, sim, paused);

            // Клик мышью
            if (const auto *mc = ev->getIf<sf::Event::MouseButtonPressed>())
            {
                // mapPixelToCoords — конвертирует реальные пиксели мыши
                // в логические координаты View.
                // Без этого на Retina-дисплеях клик будет в 2 раза ниже/правее.
                sf::Vector2f pos = window.mapPixelToCoords(mc->position);

                // Кнопки управления
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

                // Кнопка [P] Params в правом углу шапки
                Layout Lc = calcLayout(window.getSize());
                sf::FloatRect paramsArea(
                    sf::Vector2f(Lc.W - 115.f, Lc.top * 0.1f),
                    sf::Vector2f(112.f, Lc.top * 0.8f));
                if (paramsArea.contains(pos))
                {
                    controller.panelOpen = !controller.panelOpen;
                    if (controller.panelOpen)
                        controller.syncFrom(sim); // заполнить поля
                }

                // Клики внутри панели параметров
                controller.handleClick(pos, sim);
            }
        }

        // Применить параметры если пользователь нажал Apply
        if (controller.applyNow)
        {
            controller.applyNow = false;
            if (controller.applyTo(sim))
            {
                // По UML: Controller.handleStart
                controller.handleStart(sim);
                controller.panelOpen = false;
                paused = false;
            }
        }

        // Шаг 2: обновить симуляцию
        if (!paused)
        {
            double dt = clock.restart().asSeconds();
            if (dt > 0.1)
                dt = 0.1; // защита от слишком большого шага
            sim.update(dt);
        }
        else
        {
            clock.restart(); // не накапливать время пока на паузе
        }

        // Шаг 3: нарисовать кадр

        // Пересчитываем Layout под текущий размер окна
        Layout L = calcLayout(window.getSize());
        float fs = L.fontSize;                  // обычный текст
        float fsS = std::max(10.f, fs * 0.78f); // мелкий текст

        window.clear(sf::Color(14, 16, 24));

        // ШАПКА (title bar)
        box(window, 0, 0, L.W, L.top, sf::Color(20, 23, 36));
        hline(window, 0, L.top - 1, L.W, sf::Color(40, 55, 90));

        // Название программы
        renderer.drawText(window, "Foucault Pendulum",
                          L.pad, L.top * 0.10f,
                          (unsigned)std::max(14.f, fs * 1.3f),
                          sf::Color(88, 152, 238));
        renderer.drawText(window, "Simulation  |  RK4  |  Coriolis",
                          L.pad, L.top * 0.58f,
                          (unsigned)fsS, sf::Color(48, 68, 110));

        // Блоки данных в шапке — равномерно распределены
        // Каждый блок: метка (мелко) + значение (крупно)
        float bStart = std::max(200.f, L.left * 0.55f);
        float bStep = (L.W - 120.f - bStart) / 5.f;

        // Данные для каждого блока
        struct Block
        {
            const char *label;
            std::string val;
            sf::Color vc;
        };
        Block blocks[] = {
            {"Speed", fmt(sim.timeScale) + "x",
             sf::Color(215, 205, 72)},
            {"Time", fmt(sim.time) + " s",
             sf::Color(128, 198, 255)},
            {"Lat.A", fmt(sim.lat1) + "d",
             sf::Color(78, 218, 108)},
            {"Lat.B", sim.showSecond ? fmt(sim.lat2) + "d" : "off",
             sim.showSecond ? sf::Color(172, 128, 250) : sf::Color(55, 55, 75)},
            {"Status", paused ? "|| Pause" : "> Running",
             paused ? sf::Color(228, 78, 78) : sf::Color(62, 208, 108)},
        };
        for (int i = 0; i < 5; i++)
        {
            float bx = bStart + i * bStep;
            renderer.drawText(window, blocks[i].label,
                              bx, L.top * 0.10f,
                              (unsigned)fsS, sf::Color(95, 105, 130));
            renderer.drawText(window, blocks[i].val,
                              bx, L.top * 0.52f,
                              (unsigned)fs, blocks[i].vc);
        }

        // Кнопка открытия параметров
        renderer.drawText(window, "[P] Params",
                          L.W - 108.f, L.top * 0.30f,
                          (unsigned)fsS, sf::Color(70, 100, 160));

        // ЛЕВАЯ ПАНЕЛЬ: ТРАЕКТОРИЯ
        float lx = L.trajX, ly = L.trajY;
        float lw = L.trajW, lh = L.trajH;

        box(window, lx, ly, lw, lh,
            sf::Color(16, 19, 30), sf::Color(40, 60, 100), 1.f);
        renderer.drawText(window, "Trajectory",
                          lx + L.pad, ly + L.pad,
                          (unsigned)fsS, sf::Color(65, 125, 210));

        // Сетка (5×5 клеток)
        for (int i = 1; i < 5; i++)
        {
            sf::Color gc(24, 30, 48);
            vline(window, lx + i * lw / 5.f, ly + 1, lh - 2, gc);
            hline(window, lx + 1, ly + i * lh / 5.f, lw - 2, gc);
        }

        // Оси X и Y через центр подвеса маятника
        float axCx = lx + lw * 0.50f; // центр по горизонтали
        float axCy = ly + lh * 0.44f; // центр чуть выше середины
        hline(window, lx + 4, axCy, lw - 8, sf::Color(36, 48, 78));
        vline(window, axCx, ly + 4, lh - 8, sf::Color(36, 48, 78));
        renderer.drawText(window, "x", lx + lw - 14, axCy + 3,
                          (unsigned)fsS, sf::Color(48, 65, 105));
        renderer.drawText(window, "y", axCx + 3, ly + 5,
                          (unsigned)fsS, sf::Color(48, 65, 105));

        // Масштаб: сколько пикселей на 1 метр
        // При x0=0.1м и scale=lw*1.86 → 0.1*lw*1.86 ≈ 15% ширины панели
        float scale = lw * 1.86f;

        // Траектория маятника — по UML: Renderer.drawTrajectory (п.3 ТЗ)
        renderer.drawTrajectory(window,
                                sim.getTrajectoryX(),
                                sim.getTrajectoryY(),
                                sf::Color(46, 212, 142),
                                axCx, axCy, scale);

        // Второй маятник для сравнения широт
        if (sim.showSecond)
            renderer.drawTrajectory(window,
                                    sim.historyX2, sim.historyY2,
                                    sf::Color(172, 98, 248),
                                    axCx, axCy, scale);

        // Анимация маятника
        renderer.drawPendulum(window, sim.pendulum, axCx, axCy, scale);

        // Текущие координаты груза — одна строка в левом нижнем углу
        renderer.drawText(window,
                          "x=" + fmt(sim.pendulum.x, 3) +
                              "  y=" + fmt(sim.pendulum.y, 3),
                          lx + L.pad, ly + lh - fsS - 4,
                          (unsigned)fsS, sf::Color(80, 130, 105, 200));

        // Легенда широт (только если второй канал активен)
        if (sim.showSecond)
        {
            float legX = lx + lw - 68;
            float legY = ly + lh - fsS * 3.2f;
            box(window, legX, legY + fsS * 0.5f, 10, 3,
                sf::Color(46, 212, 142));
            renderer.drawText(window, fmt(sim.lat1, 0) + "d",
                              legX + 13, legY,
                              (unsigned)fsS, sf::Color(46, 212, 142));
            box(window, legX, legY + fsS * 1.8f, 10, 3,
                sf::Color(172, 98, 248));
            renderer.drawText(window, fmt(sim.lat2, 0) + "d",
                              legX + 13, legY + fsS * 1.2f,
                              (unsigned)fsS, sf::Color(172, 98, 248));
        }

        // Индикатор угла прецессии — в левом нижнем углу панели
        {
            float rad = std::min(36.f, lw * 0.092f);
            float icx = lx + rad + L.pad + 8;
            float icy = ly + lh - rad - fsS - 14;
            drawPrecession(window, renderer, icx, icy, rad,
                           sim.getCurrentAlpha(), fsS);
        }

        // КНОПКИ УПРАВЛЕНИЯ
        // Четыре кнопки в ряд: Пауза | Сброс | Скорость- | Скорость+
        {
            float bx = L.trajX;
            float by = L.btnY;
            float bw = L.btnW;
            float bh = L.btnH;

            // Цвет кнопки Пауза меняется в зависимости от состояния
            sf::Color pauseC = paused ? sf::Color(35, 115, 35)
                                      : sf::Color(115, 35, 35);

            btnPause = drawBtn(window, renderer,
                               paused ? "> Resume" : "|| Pause",
                               bx, by, bw, bh,
                               pauseC, sf::Color(220, 240, 220));

            btnReset = drawBtn(window, renderer,
                               "R  Reset",
                               bx + bw + 2, by, bw, bh,
                               sf::Color(38, 42, 88),
                               sf::Color(190, 205, 255));

            btnSpeedDn = drawBtn(window, renderer,
                                 "-  Speed",
                                 bx + 2 * (bw + 2), by, bw, bh,
                                 sf::Color(50, 46, 18),
                                 sf::Color(225, 218, 130));

            btnSpeedUp = drawBtn(window, renderer,
                                 "+  Speed",
                                 bx + 3 * (bw + 2), by, bw, bh,
                                 sf::Color(50, 46, 18),
                                 sf::Color(225, 218, 130));
        }

        // ПРАВАЯ ПАНЕЛЬ: ТРИ ГРАФИКА
        // Описание каждого графика
        struct Graph
        {
            const std::vector<double> *dataA; // данные широта A
            const std::vector<double> *dataB; // данные широта B (или nullptr)
            sf::Color color;
            const char *title; // название: x(t), y(t), a(t)
            const char *desc;  // описание
            // Указатель на метод отрисовки
            void (Renderer::*fn)(sf::RenderWindow &,
                                 const std::vector<double> &,
                                 sf::Vector2f, sf::Vector2f,
                                 sf::Color);
        };

        Graph graphs[3] = {
            {&sim.historyX, &sim.historyX2,
             sf::Color(242, 88, 88), "x(t)", "Displacement X, m",
             &Renderer::drawGraphXvsTime},
            {&sim.historyY, &sim.historyY2,
             sf::Color(68, 215, 105), "y(t)", "Displacement Y, m",
             &Renderer::drawGraphYvsTime},
            {&sim.historyAlpha, nullptr,
             sf::Color(242, 212, 48), "a(t)", "Precession angle, rad",
             &Renderer::drawGraphAlphaVsTime},
        };

        for (int i = 0; i < 3; i++)
        {
            float gy = L.gy0 + i * (L.gh + L.pad);

            // Фон и рамка графика
            box(window, L.gx, gy, L.gw, L.gh,
                sf::Color(14, 17, 26),
                sf::Color(graphs[i].color.r / 3,
                          graphs[i].color.g / 3,
                          graphs[i].color.b / 3, 200),
                1.f);

            // Заголовок (жирнее) и описание (мельче, правее)
            renderer.drawText(window, graphs[i].title,
                              L.gx + 6, gy + 5,
                              (unsigned)fs, graphs[i].color);
            renderer.drawText(window, graphs[i].desc,
                              L.gx + 55, gy + 7,
                              (unsigned)fsS,
                              sf::Color(graphs[i].color.r,
                                        graphs[i].color.g,
                                        graphs[i].color.b, 110));

            // Область графика — чуть ниже заголовка
            sf::Vector2f gPos(L.gx + 3, gy + fs + 6);
            sf::Vector2f gSz(L.gw - 6, L.gh - fs - 10);

            // Основной график (широта A)
            (renderer.*graphs[i].fn)(window, *graphs[i].dataA,
                                     gPos, gSz, graphs[i].color);

            // Второй канал (широта B) — полупрозрачный
            if (sim.showSecond && graphs[i].dataB &&
                !graphs[i].dataB->empty())
            {
                sf::Color c2 = graphs[i].color;
                c2.a = 80; // полупрозрачный
                (renderer.*graphs[i].fn)(window, *graphs[i].dataB,
                                         gPos, gSz, c2);
            }
        }

        // СТАТУС-БАР (нижняя строка)
        float sy = L.H - L.bot;
        box(window, 0, sy, L.W, L.bot, sf::Color(14, 17, 26));
        hline(window, 0, sy, L.W, sf::Color(32, 44, 72));

        // Вертикальный центр строки
        float ty = sy + (L.bot - fsS) / 2.f;

        // Три блока: описание | OmegaZ | Pts | подсказка по клавишам
        renderer.drawText(window,
                          "Foucault Pendulum  |  Runge-Kutta 4  |  KubSU FTF 2026",
                          L.pad, ty, (unsigned)fsS, sf::Color(40, 54, 85));
        {
            // OmegaZ = ω * sin(φ) — вертикальная составляющая ω Земли
            std::ostringstream ss;
            ss << "OmegaZ = " << std::scientific << std::setprecision(3)
               << sim.physics.getOmegaZ() << " rad/s";
            renderer.drawText(window, ss.str(),
                              L.W * 0.32f, ty,
                              (unsigned)fsS, sf::Color(44, 62, 102));
        }
        renderer.drawText(window,
                          "Pts: " + std::to_string(sim.historyX.size()),
                          L.W * 0.58f, ty,
                          (unsigned)fsS, sf::Color(40, 54, 85));
        renderer.drawText(window,
                          "Space=pause  R=reset  +/-=speed  2=ch.B  P=params",
                          L.W * 0.65f, ty,
                          (unsigned)(fsS * 0.88f), sf::Color(36, 48, 76));

        // Разделительная линия между левой и правой панелью
        vline(window, L.left, L.top, L.H - L.top, sf::Color(32, 48, 82));

        // Панель параметров (рисуется поверх всего)
        controller.getInput(renderer, window, sim);

        window.display();
    }

    return 0;
}