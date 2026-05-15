#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

// Отрисовщик - рисует все элементы на экране
// Соответствует UML-диаграмме классов
class Renderer
{
public:
    // Нарисовать маятник (нить + груз + вектор Кориолиса)
    void drawPendulum(sf::RenderWindow &window,
                      const class Pendulum &p,
                      float cx, float cy, float scale);

    // Нарисовать траекторию движения груза
    void drawTrajectory(sf::RenderWindow &window,
                        const std::vector<double> &xs,
                        const std::vector<double> &ys,
                        sf::Color color,
                        float cx, float cy, float scale);

    // Нарисовать график x(t)
    void drawGraphXvsTime(sf::RenderWindow &window,
                          const std::vector<double> &data,
                          sf::Vector2f pos, sf::Vector2f size,
                          sf::Color color);

    // Нарисовать график y(t)
    void drawGraphYvsTime(sf::RenderWindow &window,
                          const std::vector<double> &data,
                          sf::Vector2f pos, sf::Vector2f size,
                          sf::Color color);

    // Нарисовать график угла α(t)
    void drawGraphAlphaVsTime(sf::RenderWindow &window,
                              const std::vector<double> &data,
                              sf::Vector2f pos, sf::Vector2f size,
                              sf::Color color);

    // Вспомогательный метод: пиксельный текст (без шрифта)
    void drawText(sf::RenderWindow &window, const std::string &text,
                  float x, float y, float scale, sf::Color color);

private:
    // Нарисовать один символ пиксельным шрифтом
    void drawChar(sf::RenderWindow &window, char c,
                  float x, float y, float scale, sf::Color color);

    // Общий метод рисования графика (используется тремя выше)
    void drawGraph(sf::RenderWindow &window,
                   const std::vector<double> &data,
                   sf::Vector2f pos, sf::Vector2f size,
                   sf::Color color);
};