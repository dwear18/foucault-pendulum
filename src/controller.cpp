#include "controller.h"
#include "renderer.h"
#include <sstream>
#include <iomanip>
#include <cmath>

// Вспомогательная функция: нарисовать прямоугольник
static void fillRect(sf::RenderWindow &w, float x, float y,
                     float wd, float ht, sf::Color c)
{
    sf::RectangleShape r(sf::Vector2f(wd, ht));
    r.setPosition(sf::Vector2f(x, y));
    r.setFillColor(c);
    w.draw(r);
}

// Попробовать преобразовать строку в число
static bool toDouble(const std::string &s, double &out)
{
    try
    {
        size_t pos;
        out = std::stod(s, &pos);
        return pos == s.size(); // вся строка должна быть числом
    }
    catch (...)
    {
        return false;
    }
}

// Число в строку с нужной точностью
static std::string dtos(double v, int prec = 4)
{
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(prec) << v;
    return ss.str();
}

// ============================================================
// Конструктор - задать поля ввода (по ТЗ Приложение 1)
// ============================================================
Controller::Controller()
{
    fields = {
        {"Latitude A (phi)", "deg", "45.0000", -90, 90},
        {"Latitude B (phi)", "deg", "60.0000", -90, 90},
        {"Length L", "m", "2.5000", 0.01, 1000},
        {"Mass m", "kg", "1.0000", 0.001, 1e6},
        {"Gravity g", "m/s2", "9.8067", 0.01, 100},
        {"x0", "m", "0.1000", -10, 10},
        {"y0", "m", "0.0000", -10, 10},
        {"Vx0", "m/s", "0.0000", -50, 50},
        {"Vy0", "m/s", "0.0000", -50, 50},
        {"dt", "s", "0.0100", 1e-4, 1.0},
        {"T_max", "s", "200.0", 1, 1e6},
    };
}

// ============================================================
// По UML: getInput - вызывается каждый кадр из main
// ============================================================
void Controller::getInput(Renderer &renderer,
                          sf::RenderWindow &window,
                          Simulation &sim)
{
    // Отрисовка панели если открыта
    if (panelOpen)
        draw(window, renderer, 430, 80, 530);
}

// По UML: handleStart
void Controller::handleStart(Simulation &sim)
{
    sim.start();
}

// По UML: handleStop
void Controller::handleStop(Simulation &sim)
{
    sim.stop();
}

// По UML: handleLatitudeChange
void Controller::handleLatitudeChange(Simulation &sim, double newLat)
{
    sim.lat1 = newLat;
    sim.physics.setLatitude(newLat);
}

// ============================================================
// Клавиатура (вызывается из main при событии KeyPressed)
// ============================================================
void Controller::handleKey(sf::Keyboard::Key key,
                           Simulation &sim, bool &paused)
{
    if (panelOpen)
    {
        // В панели: Tab переключает поле, Enter применяет
        if (key == sf::Keyboard::Key::Tab)
        {
            if (focusedField >= 0)
            {
                focusedField = (focusedField + 1) % (int)fields.size();
                for (auto &f : fields)
                    f.error = false;
            }
        }
        if (key == sf::Keyboard::Key::Enter)
            applyNow = true;
        return;
    }

    // Вне панели: управление симуляцией
    switch (key)
    {
    case sf::Keyboard::Key::Space:
        paused = !paused;
        break;
    case sf::Keyboard::Key::R:
        handleStop(sim); // сброс = stop = start
        break;
    case sf::Keyboard::Key::Add:
    case sf::Keyboard::Key::Equal:
        sim.timeScale = std::min(20.0, sim.timeScale * 1.25);
        break;
    case sf::Keyboard::Key::Subtract:
    case sf::Keyboard::Key::Hyphen:
        sim.timeScale = std::max(0.1, sim.timeScale / 1.25);
        break;
    case sf::Keyboard::Key::Num1:
        sim.timeScale = 1.0;
        break;
    case sf::Keyboard::Key::Num2:
        sim.showSecond = !sim.showSecond;
        break;
    case sf::Keyboard::Key::P:
        panelOpen = !panelOpen;
        if (panelOpen)
            syncFrom(sim);
        break;
    default:
        break;
    }
}

// ============================================================
// Ввод символов (вызывается из main при событии TextEntered)
// ============================================================
void Controller::handleTextEntered(uint32_t unicode)
{
    if (!panelOpen || focusedField < 0)
        return;
    auto &f = fields[focusedField];

    if (unicode == 8)
    {
        // Backspace - удалить последний символ
        if (!f.value.empty())
            f.value.pop_back();
    }
    else if (unicode >= 32 && unicode < 127)
    {
        char c = (char)unicode;
        // Разрешаем: цифры, точку, минус (только первый), e/E
        if (std::isdigit(c) || c == '.' || c == 'e' || c == 'E' || (c == '-' && f.value.empty()))
        {
            f.value += c;
        }
    }
    f.error = false;
}

// ============================================================
// Клик мышью
// ============================================================
bool Controller::handleClick(sf::Vector2f pos, Simulation &sim)
{
    if (!panelOpen)
        return false;

    // Кнопка "Apply"
    if (applyBtn.contains(pos))
    {
        applyNow = true;
        return true;
    }
    // Кнопка "Close"
    if (closeBtn.contains(pos))
    {
        panelOpen = false;
        return true;
    }
    return true; // поглощаем клик внутри панели
}

// ============================================================
// Синхронизировать поля с текущими параметрами симуляции
// ============================================================
void Controller::syncFrom(const Simulation &sim)
{
    fields[0].value = dtos(sim.lat1);
    fields[1].value = dtos(sim.lat2);
    fields[2].value = dtos(sim.pendulum.L);
    fields[3].value = dtos(sim.pendulum.m);
    fields[4].value = dtos(9.80665);
    fields[5].value = dtos(sim.x0);
    fields[6].value = dtos(sim.y0);
    fields[7].value = dtos(sim.vx0);
    fields[8].value = dtos(sim.vy0);
    fields[9].value = dtos(sim.dt, 4);
    fields[10].value = dtos(sim.T_max, 1);
}

// ============================================================
// Применить поля к симуляции (п. 4.2 ТЗ: валидация входных данных)
// ============================================================
bool Controller::applyTo(Simulation &sim)
{
    bool ok = true;
    double v;

    // Проверить одно поле
    auto check = [&](int i) -> bool
    {
        fields[i].error = false;
        fields[i].errorMsg = "";
        if (!toDouble(fields[i].value, v))
        {
            fields[i].error = true;
            fields[i].errorMsg = "not a number";
            ok = false;
            return false;
        }
        if (v < fields[i].minVal || v > fields[i].maxVal)
        {
            fields[i].error = true;
            std::ostringstream ss;
            ss << "[" << fields[i].minVal
               << ".." << fields[i].maxVal << "]";
            fields[i].errorMsg = ss.str();
            ok = false;
            return false;
        }
        return true;
    };

    // Применить каждый параметр
    if (check(0))
        sim.lat1 = v;
    if (check(1))
        sim.lat2 = v;
    if (check(2))
        sim.pendulum.L = v;
    if (check(3))
        sim.pendulum.m = v;
    double g = 9.80665;
    if (check(4))
        g = v;
    if (check(5))
        sim.x0 = v;
    if (check(6))
        sim.y0 = v;
    if (check(7))
        sim.vx0 = v;
    if (check(8))
        sim.vy0 = v;
    if (check(9))
        sim.dt = v;
    if (check(10))
        sim.T_max = v;

    if (ok)
    {
        // Применить гравитацию к обоим каналам
        sim.physics.setG(g);
        sim.physics2.setG(g);
    }

    return ok;
}

// ============================================================
// Отрисовка одного поля ввода
// ============================================================
void Controller::drawField(sf::RenderWindow &w, Renderer &r,
                           Field &f, float x, float y,
                           float width, int idx)
{
    bool focused = (idx == focusedField);

    // Подпись + единица
    sf::Color labelCol = f.error ? sf::Color(255, 100, 80)
                                 : sf::Color(150, 160, 180);
    r.drawText(w, f.label, x, y + 5, 1.5f, labelCol);

    // Поле ввода
    float bx = x + width * 0.55f;
    float bw = width * 0.30f;

    sf::Color borderCol = f.error   ? sf::Color(220, 70, 50)
                          : focused ? sf::Color(90, 170, 255)
                                    : sf::Color(55, 70, 100);

    fillRect(w, bx, y, bw, 22, sf::Color(20, 24, 36));
    {
        sf::RectangleShape b(sf::Vector2f(bw, 22));
        b.setPosition(sf::Vector2f(bx, y));
        b.setFillColor(sf::Color::Transparent);
        b.setOutlineThickness(1.f);
        b.setOutlineColor(borderCol);
        w.draw(b);
    }

    // Значение + курсор
    std::string display = f.value + (focused ? "_" : "");
    r.drawText(w, display, bx + 4, y + 5, 1.5f,
               focused ? sf::Color(200, 230, 255)
                       : sf::Color(150, 200, 220));

    // Единица измерения
    r.drawText(w, f.unit, bx + bw + 4, y + 5, 1.5f,
               sf::Color(80, 100, 130));

    // Сообщение об ошибке
    if (f.error)
        r.drawText(w, "ERR: " + f.errorMsg, bx, y + 24,
                   1.2f, sf::Color(255, 90, 70));
}

// ============================================================
// Отрисовка всей панели параметров
// ============================================================
void Controller::draw(sf::RenderWindow &w, Renderer &r,
                      float px, float py, float width)
{
    int n = (int)fields.size();
    float fieldH = 30.f;
    float padding = 12.f;
    float panelH = padding + 18 + n * fieldH + 40 + padding;

    // Фон панели (полупрозрачный)
    fillRect(w, px, py, width, panelH, sf::Color(16, 20, 32, 245));
    {
        sf::RectangleShape b(sf::Vector2f(width, panelH));
        b.setPosition(sf::Vector2f(px, py));
        b.setFillColor(sf::Color::Transparent);
        b.setOutlineThickness(1.5f);
        b.setOutlineColor(sf::Color(70, 110, 200));
        w.draw(b);
    }

    float cx = px + padding;
    float cy = py + padding;

    // Заголовок
    r.drawText(w, "SIMULATION PARAMETERS", cx, cy, 2.0f,
               sf::Color(90, 155, 235));
    cy += 20.f;

    // Поля
    sf::Vector2f mouse(sf::Mouse::getPosition(w));
    for (int i = 0; i < n; i++)
    {
        float fy = cy + i * fieldH;
        // Клик по полю - активировать его
        sf::FloatRect fr(sf::Vector2f(cx, fy),
                         sf::Vector2f(width - 2 * padding, fieldH));
        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) && fr.contains(mouse))
        {
            for (auto &f : fields)
                f.error = false;
            focusedField = i;
            fields[i].error = false;
        }
        drawField(w, r, fields[i], cx, fy, width - 2 * padding, i);
    }

    cy += n * fieldH + 8;

    // Кнопки Apply / Close
    float btnW = (width - 2 * padding - 8) / 2.f;

    applyBtn = sf::FloatRect(sf::Vector2f(cx, cy),
                             sf::Vector2f(btnW, 28));
    bool ah = applyBtn.contains(mouse);
    fillRect(w, cx, cy, btnW, 28,
             ah ? sf::Color(55, 125, 220) : sf::Color(38, 85, 175));
    r.drawText(w, "APPLY", cx + 10, cy + 8, 1.8f,
               sf::Color(220, 235, 255));

    float bx2 = cx + btnW + 8;
    closeBtn = sf::FloatRect(sf::Vector2f(bx2, cy),
                             sf::Vector2f(btnW, 28));
    bool ch = closeBtn.contains(mouse);
    fillRect(w, bx2, cy, btnW, 28,
             ch ? sf::Color(130, 45, 45) : sf::Color(88, 32, 32));
    r.drawText(w, "CLOSE", bx2 + 10, cy + 8, 1.8f,
               sf::Color(255, 175, 175));

    cy += 34;
    r.drawText(w, "Tab:next  Enter:apply", cx, cy, 1.3f,
               sf::Color(55, 75, 110));
}