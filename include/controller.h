#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include "simulation.h"

// Одно поле ввода параметра
struct Field
{
    std::string label; // подпись поля
    std::string unit;  // единица измерения
    std::string value; // текущее значение (строка)
    double minVal;
    double maxVal;
    bool error = false; // есть ли ошибка?
    std::string errorMsg;
};

// Контроллер - обрабатывает ввод пользователя, управляет симуляцией
// Соответствует UML-диаграмме классов
class Controller
{
public:
    Controller();

    // --- Методы по UML ---

    // Получить ввод пользователя (открыть/закрыть панель параметров)
    void getInput(class Renderer &renderer,
                  sf::RenderWindow &window,
                  Simulation &sim);

    // Запустить симуляцию
    void handleStart(Simulation &sim);

    // Остановить симуляцию
    void handleStop(Simulation &sim);

    // Сменить широту
    void handleLatitudeChange(Simulation &sim, double newLat);

    // --- Обработка событий SFML ---
    void handleTextEntered(uint32_t unicode);
    void handleKey(sf::Keyboard::Key key, Simulation &sim, bool &paused);
    bool handleClick(sf::Vector2f pos, Simulation &sim);

    // --- Панель параметров ---
    bool panelOpen = false;
    bool applyNow = false; // флаг: применить параметры

    // Синхронизировать поля с текущими параметрами симуляции
    void syncFrom(const Simulation &sim);

    // Применить поля к симуляции (возвращает false при ошибках)
    bool applyTo(Simulation &sim);

    // Нарисовать панель параметров
    void draw(sf::RenderWindow &window, class Renderer &renderer,
              float x, float y, float w);

private:
    std::vector<Field> fields; // поля ввода

    int focusedField = -1; // индекс активного поля

    sf::FloatRect applyBtn; // кнопка "Применить"
    sf::FloatRect closeBtn; // кнопка "Закрыть"

    void drawField(sf::RenderWindow &w, class Renderer &r,
                   Field &f, float x, float y, float width, int idx);
};

#endif