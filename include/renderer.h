#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

// Отрисовщик - рисует все элементы на экране
class Renderer
{
public:
    // Загрузить шрифт (вызвать один раз в начале программы)
    // Возвращает false если шрифт не найден
    bool loadFont(const std::string &path);

    // Нарисовать текст (использует загруженный шрифт)
    void drawText(sf::RenderWindow &window,
                  const std::string &text,
                  float x, float y,
                  unsigned int size,
                  sf::Color color);

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

    // Нарисовать кнопку; возвращает её прямоугольник
    sf::FloatRect drawButton(sf::RenderWindow &window,
                             const std::string &label,
                             float x, float y,
                             float w, float h,
                             sf::Color bg, sf::Color textColor);

    // Проверить, наведена ли мышь на прямоугольник
    bool isHovered(sf::RenderWindow &window, sf::FloatRect rect);

    // Есть ли загруженный шрифт?
    bool hasFont() const { return fontLoaded; }

private:
    sf::Font font;
    bool fontLoaded = false;

    // Общий метод рисования графика
    void drawGraph(sf::RenderWindow &window,
                   const std::vector<double> &data,
                   sf::Vector2f pos, sf::Vector2f size,
                   sf::Color color);
};