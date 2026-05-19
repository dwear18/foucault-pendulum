#include "controller.h"
#include "renderer.h"
#include <sstream>
#include <iomanip>
#include <cmath>

// Вспомогательные функции рисования прямоугольников
// (используются для рисования фона и рамок кнопок/полей)

// Нарисовать закрашенный прямоугольник
static void fillRect(sf::RenderWindow &w, float x, float y,
                     float wd, float ht, sf::Color c)
{
    sf::RectangleShape r(sf::Vector2f(wd, ht));
    r.setPosition(sf::Vector2f(x, y));
    r.setFillColor(c);
    w.draw(r);
}

// Нарисовать прямоугольник только с контуром (без заливки)
static void strokeRect(sf::RenderWindow &w, float x, float y,
                       float wd, float ht, sf::Color c, float t = 1.f)
{
    sf::RectangleShape r(sf::Vector2f(wd, ht));
    r.setPosition(sf::Vector2f(x, y));
    r.setFillColor(sf::Color::Transparent); // без заливки
    r.setOutlineThickness(t);
    r.setOutlineColor(c);
    w.draw(r);
}

// Попробовать преобразовать строку в число.
// Возвращает true если получилось, false если строка не число.
static bool toDouble(const std::string &s, double &out)
{
    try
    {
        size_t pos;
        out = std::stod(s, &pos);
        // pos должен дойти до конца строки — иначе "123abc" тоже пройдёт
        return pos == s.size();
    }
    catch (...)
    {
        return false;
    }
}

// Число в строку с заданной точностью
static std::string dtos(double v, int prec = 4)
{
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(prec) << v;
    return ss.str();
}

// Конструктор — задаём список полей ввода.
Controller::Controller()
{
    // Формат: { название, единица, значение, умолч., мин, макс }
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

// getInput — вызывается каждый кадр из main.
// Если панель открыта — рисуем её поверх всего.
void Controller::getInput(Renderer &r, sf::RenderWindow &w, Simulation &sim)
{
    if (panelOpen)
        draw(w, r, w.getSize());
}

// handleStart — запустить (или перезапустить) симуляцию
void Controller::handleStart(Simulation &sim)
{
    sim.start();
}

// handleStop — остановить симуляцию (сброс к начальному состоянию)
void Controller::handleStop(Simulation &sim)
{
    sim.stop();
}

// handleLatitudeChange — сменить широту без перезапуска
void Controller::handleLatitudeChange(Simulation &sim, double lat)
{
    sim.lat1 = lat;
    sim.physics.setLatitude(lat);
}

// Сбросить все поля к значениям по умолчанию
void Controller::resetToDefaults()
{
    for (auto &f : fields)
    {
        f.value = f.defaultValue;
        f.error = false;
        f.errorMsg = "";
    }
}

// Обработка нажатий клавиш
void Controller::handleKey(sf::Keyboard::Key key,
                           Simulation &sim, bool &paused)
{
    // Если панель параметров открыта — перехватываем только Tab и Enter
    if (panelOpen)
    {
        if (key == sf::Keyboard::Key::Tab && focusedField >= 0)
        {
            // Переключить на следующее поле по кругу
            focusedField = (focusedField + 1) % (int)fields.size();
        }
        if (key == sf::Keyboard::Key::Enter)
        {
            applyNow = true; // применить параметры
        }
        return; // остальные клавиши не обрабатываем пока панель открыта
    }

    // Управление симуляцией
    switch (key)
    {
    case sf::Keyboard::Key::Space:
        // Пробел = пауза / продолжить
        paused = !paused;
        break;

    case sf::Keyboard::Key::R:
        // R = сброс симуляции
        handleStop(sim);
        break;

    case sf::Keyboard::Key::Add:
    case sf::Keyboard::Key::Equal:
        // + = ускорить симуляцию (не более чем в 20 раз)
        sim.timeScale = std::min(20.0, sim.timeScale * 1.25);
        break;

    case sf::Keyboard::Key::Subtract:
    case sf::Keyboard::Key::Hyphen:
        // - = замедлить (не менее 0.1x)
        sim.timeScale = std::max(0.1, sim.timeScale / 1.25);
        break;

    case sf::Keyboard::Key::Num1:
        // 1 = нормальная скорость
        sim.timeScale = 1.0;
        break;

    case sf::Keyboard::Key::Num2:
        // 2 = включить/выключить второй канал (широта B)
        sim.showSecond = !sim.showSecond;
        break;

    case sf::Keyboard::Key::P:
        // P = открыть/закрыть панель параметров
        panelOpen = !panelOpen;
        if (panelOpen)
        {
            syncFrom(sim); // загрузить текущие параметры в поля
        }
        break;

    default:
        break;
    }
}

// Обработка ввода символов с клавиатуры (для полей ввода)
void Controller::handleTextEntered(uint32_t unicode)
{
    // Игнорируем если панель закрыта или нет активного поля
    if (!panelOpen || focusedField < 0)
        return;

    Field &f = fields[focusedField];

    if (unicode == 8)
    {
        // Backspace — удалить последний символ
        if (!f.value.empty())
        {
            f.value.pop_back();
        }
    }
    else if (unicode >= 32 && unicode < 127)
    {
        // Обычный символ — добавляем только если это цифра, точка, e или минус
        char c = (char)unicode;
        bool isDigit = (c >= '0' && c <= '9');
        bool isDot = (c == '.');
        bool isExp = (c == 'e' || c == 'E');
        bool isMinus = (c == '-' && f.value.empty()); // минус только в начале

        if (isDigit || isDot || isExp || isMinus)
        {
            f.value += c;
        }
    }

    // Сбросить метку ошибки при любом изменении
    f.error = false;
}

// Обработка кликов мышью
bool Controller::handleClick(sf::Vector2f pos, Simulation &sim)
{
    if (!panelOpen)
        return false;

    // Проверяем попадание в кнопки
    if (applyBtn.contains(pos))
    {
        applyNow = true; // запросить применение параметров
        return true;
    }
    if (closeBtn.contains(pos))
    {
        panelOpen = false; // закрыть панель
        return true;
    }
    if (resetBtn.contains(pos))
    {
        resetToDefaults(); // сбросить к умолчаниям
        return true;
    }

    // Поглощаем все клики внутри панели чтобы не нажимались кнопки позади
    return true;
}

// Заполнить поля из текущих параметров симуляции
void Controller::syncFrom(const Simulation &sim)
{
    fields[0].value = dtos(sim.lat1);       // широта A
    fields[1].value = dtos(sim.lat2);       // широта B
    fields[2].value = dtos(sim.pendulum.L); // длина маятника
    fields[3].value = dtos(sim.pendulum.m); // масса груза
    fields[4].value = dtos(9.80665);        // g (хранится в PhysicsEngine)
    fields[5].value = dtos(sim.x0);         // начальное x
    fields[6].value = dtos(sim.y0);         // начальное y
    fields[7].value = dtos(sim.vx0);        // начальная скорость Vx
    fields[8].value = dtos(sim.vy0);        // начальная скорость Vy
    fields[9].value = dtos(sim.dt, 4);      // шаг интегрирования
    fields[10].value = dtos(sim.T_max, 1);  // максимальное время
}

// Применить поля к симуляции.
// Здесь выполняется валидация.
// Возвращает true если все поля корректны.
bool Controller::applyTo(Simulation &sim)
{
    bool allOk = true;
    double v;

    // Проверить одно поле: разобрать число и проверить диапазон
    auto checkField = [&](int i) -> bool
    {
        fields[i].error = false;
        fields[i].errorMsg = "";

        // Пробуем преобразовать строку в число
        if (!toDouble(fields[i].value, v))
        {
            fields[i].error = true;
            fields[i].errorMsg = "not a number";
            allOk = false;
            return false;
        }

        // Проверяем допустимый диапазон
        if (v < fields[i].minVal || v > fields[i].maxVal)
        {
            fields[i].error = true;
            std::ostringstream ss;
            ss << "[" << fields[i].minVal
               << " .. " << fields[i].maxVal << "]";
            fields[i].errorMsg = ss.str();
            allOk = false;
            return false;
        }

        return true;
    };

    // Применяем каждый параметр если он прошёл проверку
    if (checkField(0))
        sim.lat1 = v; // широта A
    if (checkField(1))
        sim.lat2 = v; // широта B
    if (checkField(2))
        sim.pendulum.L = v; // длина
    if (checkField(3))
        sim.pendulum.m = v; // масса
    double g = 9.80665;
    if (checkField(4))
        g = v; // ускорение g
    if (checkField(5))
        sim.x0 = v; // x0
    if (checkField(6))
        sim.y0 = v; // y0
    if (checkField(7))
        sim.vx0 = v; // Vx0
    if (checkField(8))
        sim.vy0 = v; // Vy0
    if (checkField(9))
        sim.dt = v; // dt
    if (checkField(10))
        sim.T_max = v; // T_max

    // Применить гравитацию к обоим каналам
    if (allOk)
    {
        sim.physics.setG(g);
        sim.physics2.setG(g);
    }

    return allOk;
}

// Нарисовать одно поле ввода
void Controller::drawField(sf::RenderWindow &w, Renderer &r,
                           Field &f, float x, float y,
                           float width, float fieldH, int idx)
{
    bool focused = (idx == focusedField); // это поле активно?

    // Рассчитываем позиции элементов поля пропорционально ширине
    float labelW = width * 0.38f;      // ширина подписи
    float inputX = x + labelW + 6;     // начало поля ввода
    float inputW = width * 0.42f;      // ширина поля ввода
    float unitX = inputX + inputW + 5; // начало единицы измерения
    float inputH = fieldH - 6.f;       // высота поля ввода

    // Размер шрифта зависит от высоты поля
    unsigned int fs = (unsigned int)std::max(11.f,
                                             std::min(15.f, fieldH * 0.48f));

    //  Подпись 
    sf::Color labelColor = f.error
                               ? sf::Color(255, 100, 80)   // красный при ошибке
                               : sf::Color(185, 195, 215); // обычный серо-голубой

    r.drawText(w, f.label, x, y + (inputH - (float)fs) / 2.f,
               fs, labelColor);

    //  Поле ввода 
    sf::Color borderColor = f.error   ? sf::Color(220, 70, 50)  // красный
                            : focused ? sf::Color(80, 160, 255) // синий
                                      : sf::Color(55, 70, 100); // обычный

    fillRect(w, inputX, y, inputW, inputH, sf::Color(18, 22, 36));
    strokeRect(w, inputX, y, inputW, inputH, borderColor);

    // Значение + мигающий курсор "|" для активного поля
    std::string display = f.value + (focused ? "|" : "");
    sf::Color textColor = focused
                              ? sf::Color(200, 230, 255) // ярче когда активно
                              : sf::Color(160, 210, 230);

    r.drawText(w, display, inputX + 5,
               y + (inputH - (float)fs) / 2.f, fs, textColor);

    //  Единица измерения 
    r.drawText(w, f.unit, unitX,
               y + (inputH - (float)fs) / 2.f,
               (unsigned int)(fs - 1),
               sf::Color(90, 110, 145));

    //  Сообщение об ошибке (если есть) 
    if (f.error)
    {
        r.drawText(w, "Err: " + f.errorMsg,
                   inputX, y + inputH + 1,
                   11, sf::Color(255, 90, 70));
    }
}

// Нарисовать всю панель параметров.
// winSize — реальный размер окна (для масштабирования).
void Controller::draw(sf::RenderWindow &w, Renderer &r,
                      sf::Vector2u winSize)
{
    float WW = (float)winSize.x;
    float WH = (float)winSize.y;

    // Панель занимает 50% ширины окна и центрирована по горизонтали
    float panelW = std::min(580.f, WW * 0.50f);
    float px = (WW - panelW) / 2.f; // x-позиция левого края панели

    int n = (int)fields.size(); // количество полей

    // Высота одного поля — зависит от высоты окна
    float fieldH = std::max(26.f, std::min(36.f, (WH * 0.60f) / n));
    float pad = 12.f;
    float titleH = 26.f;
    float btnH = std::max(28.f, fieldH * 0.85f);

    // Общая высота панели
    float panelH = pad + titleH // заголовок
                   + n * fieldH // поля
                   + 10 + btnH  // кнопка Reset
                   + 10 + btnH  // кнопки Apply/Close
                   + pad;

    // Вертикально центрируем панель в окне
    float py = (WH - panelH) / 2.f;

    //  Фон панели 
    fillRect(w, px, py, panelW, panelH, sf::Color(16, 20, 34, 248));
    strokeRect(w, px, py, panelW, panelH, sf::Color(65, 105, 200), 1.5f);

    float cx = px + pad; // x начала контента
    float cy = py + pad; // y начала контента

    //  Заголовок 
    unsigned int titleFs = (unsigned int)std::max(14.f,
                                                  std::min(18.f, panelW * 0.032f));
    r.drawText(w, "Simulation Parameters", cx, cy,
               titleFs, sf::Color(85, 150, 235));
    cy += titleH;

    //  Поля ввода 
    float fieldW = panelW - 2.f * pad;

    // Получаем позицию мыши в логических координатах
    sf::Vector2f mouse = w.mapPixelToCoords(sf::Mouse::getPosition(w));

    for (int i = 0; i < n; i++)
    {
        float fy = cy + i * fieldH;

        // Активируем поле при клике на него
        sf::FloatRect fieldRect(sf::Vector2f(cx, fy),
                                sf::Vector2f(fieldW, fieldH));
        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) && fieldRect.contains(mouse))
        {
            // Сбросить ошибки и сделать это поле активным
            for (auto &f : fields)
                f.error = false;
            focusedField = i;
        }

        drawField(w, r, fields[i], cx, fy, fieldW, fieldH, i);
    }

    cy += n * fieldH + 10;

    //  Кнопка "Reset to default values" 
    resetBtn = sf::FloatRect(sf::Vector2f(cx, cy),
                             sf::Vector2f(fieldW, btnH));
    bool hovReset = resetBtn.contains(mouse);

    fillRect(w, cx, cy, fieldW, btnH,
             hovReset ? sf::Color(55, 80, 55) : sf::Color(38, 58, 38));
    strokeRect(w, cx, cy, fieldW, btnH, sf::Color(70, 130, 70));
    r.drawText(w, "Reset to default values",
               cx + 8, cy + (btnH - 14.f) / 2.f,
               14, sf::Color(160, 235, 160));
    cy += btnH + 8;

    // Кнопки Apply и Close рядом
    float half = (fieldW - 6.f) / 2.f;

    // Apply (применить)
    applyBtn = sf::FloatRect(sf::Vector2f(cx, cy),
                             sf::Vector2f(half, btnH));
    bool hovApply = applyBtn.contains(mouse);
    fillRect(w, cx, cy, half, btnH,
             hovApply ? sf::Color(55, 125, 225) : sf::Color(38, 90, 180));
    strokeRect(w, cx, cy, half, btnH, sf::Color(80, 150, 255));
    r.drawText(w, "Apply", cx + 8, cy + (btnH - 14.f) / 2.f,
               14, sf::Color(220, 235, 255));

    // Close (закрыть)
    float bx2 = cx + half + 6;
    closeBtn = sf::FloatRect(sf::Vector2f(bx2, cy),
                             sf::Vector2f(half, btnH));
    bool hovClose = closeBtn.contains(mouse);
    fillRect(w, bx2, cy, half, btnH,
             hovClose ? sf::Color(130, 45, 45) : sf::Color(90, 32, 32));
    strokeRect(w, bx2, cy, half, btnH, sf::Color(180, 70, 70));
    r.drawText(w, "Close", bx2 + 8, cy + (btnH - 14.f) / 2.f,
               14, sf::Color(255, 175, 175));

    cy += btnH + 6;

    // Подсказка
    r.drawText(w, "Tab: next field    Enter: apply",
               cx, cy, 12, sf::Color(55, 75, 110));
}