#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include "simulation.h"

// Одно поле ввода параметра
struct Field
{
    std::string label;        // подпись
    std::string unit;         // единица
    std::string value;        // текущее значение
    std::string defaultValue; // значение по умолчанию
    double minVal;
    double maxVal;
    bool error = false;
    std::string errorMsg;
};

// Контроллер - обработка ввода и панель параметров
class Controller
{
public:
    Controller();

    // Методы по UML
    void getInput(class Renderer &r, sf::RenderWindow &w, Simulation &sim);
    void handleStart(Simulation &sim);
    void handleStop(Simulation &sim);
    void handleLatitudeChange(Simulation &sim, double lat);

    // Обработка событий
    void handleTextEntered(uint32_t unicode);
    void handleKey(sf::Keyboard::Key key, Simulation &sim, bool &paused);
    bool handleClick(sf::Vector2f pos, Simulation &sim);

    // Панель параметров
    bool panelOpen = false;
    bool applyNow = false;

    void syncFrom(const Simulation &sim);
    bool applyTo(Simulation &sim);
    void resetToDefaults();

    // Рисовать панель с учётом реального размера окна
    void draw(sf::RenderWindow &w, class Renderer &r, sf::Vector2u winSize);

private:
    std::vector<Field> fields;
    int focusedField = -1;

    sf::FloatRect applyBtn;
    sf::FloatRect closeBtn;
    sf::FloatRect resetBtn;

    void drawField(sf::RenderWindow &w, class Renderer &r,
                   Field &f, float x, float y, float width,
                   float fieldH, int idx);
};

#endif