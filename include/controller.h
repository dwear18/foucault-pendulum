#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include "simulation.h"

// Одно поле ввода параметра
struct Field
{
    std::string label;        // подпись поля
    std::string unit;         // единица измерения
    std::string value;        // текущее значение (строка)
    std::string defaultValue; // значение по умолчанию
    double minVal;
    double maxVal;
    bool error = false;
    std::string errorMsg;
};

// Контроллер - обрабатывает ввод пользователя, управляет симуляцией
// Соответствует UML-диаграмме классов
class Controller
{
public:
    Controller();

    // --- Методы по UML ---

    // Получить ввод (рисует панель если открыта)
    void getInput(class Renderer &renderer,
                  sf::RenderWindow &window,
                  Simulation &sim);

    // Запустить симуляцию
    void handleStart(Simulation &sim);

    // Остановить/сбросить симуляцию
    void handleStop(Simulation &sim);

    // Сменить широту
    void handleLatitudeChange(Simulation &sim, double newLat);

    // --- Обработка событий SFML ---
    void handleTextEntered(uint32_t unicode);
    void handleKey(sf::Keyboard::Key key, Simulation &sim, bool &paused);
    bool handleClick(sf::Vector2f pos, Simulation &sim);

    // --- Состояние панели ---
    bool panelOpen = false;
    bool applyNow = false; // флаг: применить параметры

    // Синхронизировать поля из симуляции
    void syncFrom(const Simulation &sim);

    // Применить поля к симуляции (с валидацией)
    bool applyTo(Simulation &sim);

    // Сбросить все поля к значениям по умолчанию
    void resetToDefaults();

    // Нарисовать панель параметров
    void draw(sf::RenderWindow &window, class Renderer &renderer,
              float x, float y, float w);

private:
    std::vector<Field> fields; // поля ввода
    int focusedField = -1;     // активное поле

    // Прямоугольники кнопок (для обработки кликов)
    sf::FloatRect applyBtn;
    sf::FloatRect closeBtn;
    sf::FloatRect resetBtn;

    void drawField(sf::RenderWindow &w, class Renderer &r,
                   Field &f, float x, float y, float width, int idx);
};

#endif