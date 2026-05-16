#include "controller.h"
#include "renderer.h"
#include <sstream>
#include <iomanip>
#include <cmath>

// Вспомогательные функции
static void fillRect(sf::RenderWindow &w, float x, float y,
                     float wd, float ht, sf::Color c)
{
    sf::RectangleShape r(sf::Vector2f(wd, ht));
    r.setPosition(sf::Vector2f(x, y));
    r.setFillColor(c);
    w.draw(r);
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

// Попробовать разобрать строку как число
static bool toDouble(const std::string &s, double &out)
{
    try
    {
        size_t pos;
        out = std::stod(s, &pos);
        return pos == s.size();
    }
    catch (...)
    {
        return false;
    }
}

// Число в строку
static std::string dtos(double v, int prec = 4)
{
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(prec) << v;
    return ss.str();
}

// ============================================================
// Конструктор - задать поля согласно Приложению 1 ТЗ
// ============================================================
Controller::Controller()
{
    // label, unit, value, defaultValue, min, max
    fields = {
        {"Latitude A (phi)", "deg", "45.0000", "45.0000", -90, 90},
        {"Latitude B (phi)", "deg", "60.0000", "60.0000", -90, 90},
        {"Length L", "m", "2.5000", "2.5000", 0.01, 1000},
        {"Mass m", "kg", "1.0000", "1.0000", 0.001, 1e6},
        {"Gravity g", "m/s2", "9.8067", "9.8067", 0.01, 100},
        {"x0", "m", "0.1000", "0.1000", -10, 10},
        {"y0", "m", "0.0000", "0.0000", -10, 10},
        {"Vx0", "m/s", "0.0000", "0.0000", -50, 50},
        {"Vy0", "m/s", "0.0000", "0.0000", -50, 50},
        {"Step dt", "s", "0.0100", "0.0100", 1e-4, 1.0},
        {"T_max", "s", "200.0", "200.0", 1, 1e6},
    };
}

// ============================================================
// По UML: getInput - вызывается каждый кадр
// ============================================================
void Controller::getInput(Renderer &renderer,
                          sf::RenderWindow &window,
                          Simulation &sim)
{
    if (panelOpen)
        draw(window, renderer, 430, 75, 540);
}

void Controller::handleStart(Simulation &sim) { sim.start(); }
void Controller::handleStop(Simulation &sim) { sim.stop(); }

void Controller::handleLatitudeChange(Simulation &sim, double newLat)
{
    sim.lat1 = newLat;
    sim.physics.setLatitude(newLat);
}

// ============================================================
// Сброс полей к значениям по умолчанию
// ============================================================
void Controller::resetToDefaults()
{
    for (auto &f : fields)
    {
        f.value = f.defaultValue;
        f.error = false;
        f.errorMsg = "";
    }
}

// ============================================================
// Клавиатура
// ============================================================
void Controller::handleKey(sf::Keyboard::Key key,
                           Simulation &sim, bool &paused)
{
    if (panelOpen)
    {
        if (key == sf::Keyboard::Key::Tab && focusedField >= 0)
            focusedField = (focusedField + 1) % (int)fields.size();
        if (key == sf::Keyboard::Key::Enter)
            applyNow = true;
        return;
    }

    switch (key)
    {
    case sf::Keyboard::Key::Space:
        paused = !paused;
        break;
    case sf::Keyboard::Key::R:
        handleStop(sim);
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
// Ввод текста
// ============================================================
void Controller::handleTextEntered(uint32_t unicode)
{
    if (!panelOpen || focusedField < 0)
        return;
    auto &f = fields[focusedField];

    if (unicode == 8)
    {
        if (!f.value.empty())
            f.value.pop_back();
    }
    else if (unicode >= 32 && unicode < 127)
    {
        char c = (char)unicode;
        if (std::isdigit(c) || c == '.' || c == 'e' || c == 'E' || (c == '-' && f.value.empty()))
            f.value += c;
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

    if (applyBtn.contains(pos))
    {
        applyNow = true;
        return true;
    }
    if (closeBtn.contains(pos))
    {
        panelOpen = false;
        return true;
    }
    if (resetBtn.contains(pos))
    {
        resetToDefaults();
        return true;
    }
    return true; // поглощаем все клики внутри панели
}

// ============================================================
// Синхронизация полей с симуляцией
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
// Применить поля (п.4.2 ТЗ: валидация входных данных)
// ============================================================
bool Controller::applyTo(Simulation &sim)
{
    bool ok = true;
    double v;

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
            ss << "[" << fields[i].minVal << ".." << fields[i].maxVal << "]";
            fields[i].errorMsg = ss.str();
            ok = false;
            return false;
        }
        return true;
    };

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

    // Подпись
    sf::Color labelCol = f.error ? sf::Color(255, 100, 80)
                                 : sf::Color(180, 190, 210);
    r.drawText(w, f.label, x, y + 3, 14, labelCol);

    // Поле ввода
    float bx = x + width * 0.52f;
    float bw = width * 0.33f;
    float bh = 24.f;

    sf::Color borderCol = f.error   ? sf::Color(220, 70, 50)
                          : focused ? sf::Color(80, 160, 255)
                                    : sf::Color(55, 70, 100);
    sf::Color bgCol = focused ? sf::Color(25, 35, 55)
                              : sf::Color(18, 22, 36);

    fillRect(w, bx, y, bw, bh, bgCol);
    drawBorder(w, bx, y, bw, bh, borderCol);

    // Значение + курсор
    std::string display = f.value + (focused ? "|" : "");
    r.drawText(w, display, bx + 4, y + 3, 14,
               focused ? sf::Color(200, 230, 255)
                       : sf::Color(160, 210, 230));

    // Единица
    r.drawText(w, f.unit, bx + bw + 5, y + 3, 13,
               sf::Color(90, 110, 145));

    // Ошибка
    if (f.error)
        r.drawText(w, "Err: " + f.errorMsg,
                   bx, y + bh + 1, 13, sf::Color(255, 90, 70));
}

// ============================================================
// Отрисовка всей панели параметров
// ============================================================
void Controller::draw(sf::RenderWindow &w, Renderer &r,
                      float px, float py, float width)
{
    int n = (int)fields.size();
    float fieldH = 30.f;
    float pad = 14.f;
    // высота панели: заголовок + поля + три кнопки + отступы
    float panelH = pad + 22 + n * fieldH + 10 + 32 + 10 + 32 + pad;

    // --- Фон панели ---
    fillRect(w, px, py, width, panelH, sf::Color(16, 20, 34, 248));
    drawBorder(w, px, py, width, panelH, sf::Color(65, 105, 200), 1.5f);

    float cx = px + pad;
    float cy = py + pad;

    // --- Заголовок ---
    r.drawText(w, "Simulation Parameters", cx, cy, 16,
               sf::Color(85, 150, 235));
    cy += 26.f;

    // --- Поля ---
    sf::Vector2f mouse(sf::Mouse::getPosition(w));
    for (int i = 0; i < n; i++)
    {
        float fy = cy + i * fieldH;
        sf::FloatRect fr(sf::Vector2f(cx, fy),
                         sf::Vector2f(width - 2 * pad, fieldH));
        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) && fr.contains(mouse))
        {
            for (auto &f : fields)
                f.error = false;
            focusedField = i;
        }
        drawField(w, r, fields[i], cx, fy, width - 2 * pad, i);
    }

    cy += n * fieldH + 10;

    // --- Кнопка "Reset to defaults" ---
    resetBtn = sf::FloatRect(sf::Vector2f(cx, cy),
                             sf::Vector2f(width - 2 * pad, 28));
    bool rh = resetBtn.contains(mouse);
    fillRect(w, cx, cy, width - 2 * pad, 28,
             rh ? sf::Color(55, 80, 55) : sf::Color(38, 58, 38));
    drawBorder(w, cx, cy, width - 2 * pad, 28, sf::Color(70, 130, 70));
    r.drawText(w, "Reset to default values",
               cx + 8, cy + 6, 14, sf::Color(160, 235, 160));
    cy += 36;

    // --- Кнопки Apply / Close ---
    float btnW = (width - 2 * pad - 8) / 2.f;

    applyBtn = sf::FloatRect(sf::Vector2f(cx, cy),
                             sf::Vector2f(btnW, 28));
    bool ah = applyBtn.contains(mouse);
    fillRect(w, cx, cy, btnW, 28,
             ah ? sf::Color(55, 125, 225) : sf::Color(38, 90, 180));
    drawBorder(w, cx, cy, btnW, 28, sf::Color(80, 150, 255));
    r.drawText(w, "Apply", cx + 8, cy + 6, 14,
               sf::Color(220, 235, 255));

    float bx2 = cx + btnW + 8;
    closeBtn = sf::FloatRect(sf::Vector2f(bx2, cy),
                             sf::Vector2f(btnW, 28));
    bool ch = closeBtn.contains(mouse);
    fillRect(w, bx2, cy, btnW, 28,
             ch ? sf::Color(130, 45, 45) : sf::Color(90, 32, 32));
    drawBorder(w, bx2, cy, btnW, 28, sf::Color(180, 70, 70));
    r.drawText(w, "Close", bx2 + 8, cy + 6, 14,
               sf::Color(255, 175, 175));

    cy += 34;
    r.drawText(w, "Tab: next field    Enter: apply",
               cx, cy, 13, sf::Color(55, 75, 110));
}