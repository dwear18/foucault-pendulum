#include "controller.h"
#include "renderer.h"
#include <sstream>
#include <iomanip>
#include <cmath>

// ─── Вспомогательные функции ─────────────────────────────────────────────────

static void fillRect(sf::RenderWindow &w, float x, float y,
                     float wd, float ht, sf::Color c)
{
    sf::RectangleShape r(sf::Vector2f(wd, ht));
    r.setPosition(sf::Vector2f(x, y));
    r.setFillColor(c);
    w.draw(r);
}

static void strokeRect(sf::RenderWindow &w, float x, float y,
                       float wd, float ht, sf::Color c, float t = 1.f)
{
    sf::RectangleShape r(sf::Vector2f(wd, ht));
    r.setPosition(sf::Vector2f(x, y));
    r.setFillColor(sf::Color::Transparent);
    r.setOutlineThickness(t);
    r.setOutlineColor(c);
    w.draw(r);
}

static bool toDouble(const std::string &s, double &out)
{
    try
    {
        size_t p;
        out = std::stod(s, &p);
        return p == s.size();
    }
    catch (...)
    {
        return false;
    }
}

static std::string dtos(double v, int p = 4)
{
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(p) << v;
    return ss.str();
}

// ─── Конструктор ─────────────────────────────────────────────────────────────

Controller::Controller()
{
    // Поля согласно Приложению 1 ТЗ
    fields = {
        {"Latitude A", "deg", "45.0000", "45.0000", -90, 90},
        {"Latitude B", "deg", "60.0000", "60.0000", -90, 90},
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

// ─── UML методы ──────────────────────────────────────────────────────────────

void Controller::getInput(Renderer &r, sf::RenderWindow &w, Simulation &sim)
{
    if (panelOpen)
        draw(w, r, w.getSize());
}

void Controller::handleStart(Simulation &sim) { sim.start(); }
void Controller::handleStop(Simulation &sim) { sim.stop(); }

void Controller::handleLatitudeChange(Simulation &sim, double lat)
{
    sim.lat1 = lat;
    sim.physics.setLatitude(lat);
}

void Controller::resetToDefaults()
{
    for (auto &f : fields)
    {
        f.value = f.defaultValue;
        f.error = false;
    }
}

// ─── Клавиатура ──────────────────────────────────────────────────────────────

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

// ─── Ввод текста ─────────────────────────────────────────────────────────────

void Controller::handleTextEntered(uint32_t u)
{
    if (!panelOpen || focusedField < 0)
        return;
    auto &f = fields[focusedField];
    if (u == 8)
    {
        if (!f.value.empty())
            f.value.pop_back();
    }
    else if (u >= 32 && u < 127)
    {
        char c = (char)u;
        if (std::isdigit(c) || c == '.' || c == 'e' || c == 'E' || (c == '-' && f.value.empty()))
            f.value += c;
    }
    f.error = false;
}

// ─── Клик мышью ──────────────────────────────────────────────────────────────

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
    return true;
}

// ─── Синхронизация ───────────────────────────────────────────────────────────

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

// ─── Применить параметры (п.4.2 ТЗ — валидация) ─────────────────────────────

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

// ─── Рисовать одно поле ──────────────────────────────────────────────────────

void Controller::drawField(sf::RenderWindow &w, Renderer &r,
                           Field &f, float x, float y,
                           float width, float fieldH, int idx)
{
    bool focused = (idx == focusedField);

    // Размеры масштабируются под ширину панели
    float labelW = width * 0.38f;
    float inputX = x + labelW + 6;
    float inputW = width * 0.42f;
    float unitX = inputX + inputW + 5;
    float inputH = fieldH - 6.f;

    // Автоматический размер шрифта — чтобы влезало в поле
    unsigned int fs = (unsigned int)std::max(11.f, std::min(15.f, fieldH * 0.48f));

    // Подпись
    sf::Color labelCol = f.error ? sf::Color(255, 100, 80)
                                 : sf::Color(185, 195, 215);
    r.drawText(w, f.label, x, y + (inputH - fs) / 2.f, fs, labelCol);

    // Поле ввода
    sf::Color borderCol = f.error   ? sf::Color(220, 70, 50)
                          : focused ? sf::Color(80, 160, 255)
                                    : sf::Color(55, 70, 100);
    fillRect(w, inputX, y, inputW, inputH, sf::Color(18, 22, 36));
    strokeRect(w, inputX, y, inputW, inputH, borderCol);

    std::string display = f.value + (focused ? "|" : "");
    r.drawText(w, display, inputX + 5, y + (inputH - fs) / 2.f,
               fs, focused ? sf::Color(200, 230, 255) : sf::Color(160, 210, 230));

    // Единица измерения
    r.drawText(w, f.unit, unitX, y + (inputH - fs) / 2.f,
               (unsigned int)(fs - 1), sf::Color(90, 110, 145));

    // Ошибка
    if (f.error)
        r.drawText(w, "Err: " + f.errorMsg,
                   inputX, y + inputH + 1, 11, sf::Color(255, 90, 70));
}

// ─── Рисовать всю панель ─────────────────────────────────────────────────────

void Controller::draw(sf::RenderWindow &w, Renderer &r, sf::Vector2u winSize)
{
    float WW = (float)winSize.x;
    float WH = (float)winSize.y;

    // Панель занимает 50% ширины окна и центрирована
    float panelW = std::min(580.f, WW * 0.5f);
    float px = (WW - panelW) / 2.f;

    int n = (int)fields.size();

    // Высота одного поля зависит от высоты окна
    float fieldH = std::max(26.f, std::min(36.f, (WH * 0.6f) / n));
    float pad = 12.f;
    float titleH = 26.f;
    float btnH = std::max(28.f, fieldH * 0.85f);
    float panelH = pad + titleH + n * fieldH + 10 + btnH + 10 + btnH + pad;

    // Вертикальный центр чуть выше середины окна
    float py = (WH - panelH) / 2.f;

    // Фон панели
    fillRect(w, px, py, panelW, panelH, sf::Color(16, 20, 34, 248));
    strokeRect(w, px, py, panelW, panelH, sf::Color(65, 105, 200), 1.5f);

    float cx = px + pad;
    float cy = py + pad;

    // Заголовок
    unsigned int titleFs = (unsigned int)std::max(14.f, std::min(18.f, panelW * 0.032f));
    r.drawText(w, "Simulation Parameters", cx, cy, titleFs,
               sf::Color(85, 150, 235));
    cy += titleH;

    // Поля ввода — проверяем клик мыши для активации
    sf::Vector2f mouse = w.mapPixelToCoords(sf::Mouse::getPosition(w));
    float fieldW = panelW - 2 * pad;

    for (int i = 0; i < n; i++)
    {
        float fy = cy + i * fieldH;
        sf::FloatRect fr(sf::Vector2f(cx, fy),
                         sf::Vector2f(fieldW, fieldH));
        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) && fr.contains(mouse))
        {
            for (auto &f : fields)
                f.error = false;
            focusedField = i;
        }
        drawField(w, r, fields[i], cx, fy, fieldW, fieldH, i);
    }

    cy += n * fieldH + 10;

    // Кнопка "Reset to defaults"
    resetBtn = sf::FloatRect(sf::Vector2f(cx, cy),
                             sf::Vector2f(fieldW, btnH));
    bool rh = resetBtn.contains(mouse);
    fillRect(w, cx, cy, fieldW, btnH,
             rh ? sf::Color(55, 80, 55) : sf::Color(38, 58, 38));
    strokeRect(w, cx, cy, fieldW, btnH, sf::Color(70, 130, 70));
    r.drawText(w, "Reset to default values",
               cx + 8, cy + (btnH - 14) / 2.f, 14,
               sf::Color(160, 235, 160));
    cy += btnH + 8;

    // Кнопки Apply / Close рядом
    float half = (fieldW - 6) / 2.f;

    applyBtn = sf::FloatRect(sf::Vector2f(cx, cy),
                             sf::Vector2f(half, btnH));
    bool ah = applyBtn.contains(mouse);
    fillRect(w, cx, cy, half, btnH,
             ah ? sf::Color(55, 125, 225) : sf::Color(38, 90, 180));
    strokeRect(w, cx, cy, half, btnH, sf::Color(80, 150, 255));
    r.drawText(w, "Apply", cx + 8, cy + (btnH - 14) / 2.f, 14,
               sf::Color(220, 235, 255));

    float bx2 = cx + half + 6;
    closeBtn = sf::FloatRect(sf::Vector2f(bx2, cy),
                             sf::Vector2f(half, btnH));
    bool ch = closeBtn.contains(mouse);
    fillRect(w, bx2, cy, half, btnH,
             ch ? sf::Color(130, 45, 45) : sf::Color(90, 32, 32));
    strokeRect(w, bx2, cy, half, btnH, sf::Color(180, 70, 70));
    r.drawText(w, "Close", bx2 + 8, cy + (btnH - 14) / 2.f, 14,
               sf::Color(255, 175, 175));

    cy += btnH + 6;
    r.drawText(w, "Tab: next field    Enter: apply",
               cx, cy, 12, sf::Color(55, 75, 110));
}