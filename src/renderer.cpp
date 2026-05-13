#include "renderer.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

void Renderer::drawPendulum(sf::RenderWindow &window, double x, double y)
{
    // Точка подвеса
    sf::CircleShape pivot(5.f);
    pivot.setPosition(sf::Vector2f(195.f, 295.f));
    pivot.setFillColor(sf::Color::White);
    window.draw(pivot);
    
    // Нить маятника
    sf::VertexArray line(sf::PrimitiveType::LineStrip, 2);
    line[0].position = sf::Vector2f(200.f, 300.f);
    line[0].color = sf::Color::Cyan;
    line[1].position = sf::Vector2f(200.f + (float)x * 150.f, 300.f + (float)y * 150.f);
    line[1].color = sf::Color::Cyan;
    window.draw(line);
    
    // Груз маятника
    sf::CircleShape bob(8.f);
    bob.setPosition(sf::Vector2f(192.f + (float)x * 150.f, 292.f + (float)y * 150.f));
    bob.setFillColor(sf::Color::Red);
    window.draw(bob);
    
    // Вектор ускорения Кориолиса (условный)
    if (std::abs(x) > 0.01 || std::abs(y) > 0.01) {
        sf::VertexArray accelArrow(sf::PrimitiveType::LineStrip, 2);
        accelArrow[0].position = sf::Vector2f(200.f + (float)x * 150.f, 300.f + (float)y * 150.f);
        accelArrow[0].color = sf::Color::Magenta;
        accelArrow[1].position = sf::Vector2f(200.f + (float)x * 150.f + (float)y * 30.f,
                                   300.f + (float)y * 150.f - (float)x * 30.f);
        accelArrow[1].color = sf::Color::Magenta;
        window.draw(accelArrow);
    }
}

void Renderer::drawTrajectory(sf::RenderWindow &window,
                              const std::vector<double> &xs,
                              const std::vector<double> &ys)
{
    if (xs.size() < 2)
        return;

    sf::VertexArray trajectory(sf::PrimitiveType::LineStrip);

    for (size_t i = 0; i < xs.size(); i++)
    {
        sf::Vertex v;
        v.position = sf::Vector2f(200 + (float)xs[i] * 150,
                      300 + (float)ys[i] * 150);
        
        // Затухание цвета для старых точек
        float alpha = 200.0f * (1.0f - (float)(xs.size() - i - 1) / (float)xs.size());
        unsigned char alphaVal = static_cast<unsigned char>(std::max(0.0f, std::min(255.0f, alpha)));
        v.color = sf::Color(0, 255, 100, alphaVal);
        trajectory.append(v);
    }

    window.draw(trajectory);
}

void Renderer::drawGraph(sf::RenderWindow &window,
                         const std::vector<double> &data,
                         sf::Vector2f origin,
                         sf::Vector2f size,
                         sf::Color color)
{
    if (data.size() < 2)
        return;

    // Вычисляем мин и макс для масштабирования
    double minV = *std::min_element(data.begin(), data.end());
    double maxV = *std::max_element(data.begin(), data.end());
    double range = (maxV - minV == 0) ? 1 : (maxV - minV);

    // Рамка графика - уже нарисована в main.cpp, здесь рисуем содержимое

    // Сетка на графике (мелкие линии)
    sf::Color gridColor(50, 50, 65);
    for (int i = 0; i <= 10; i++) {
        float yPos = origin.y + (i / 10.0f) * size.y;
        sf::RectangleShape gridLine(sf::Vector2f(size.x, 1.f));
        gridLine.setPosition(sf::Vector2f(origin.x, yPos));
        gridLine.setFillColor(gridColor);
        window.draw(gridLine);
    }
    
    // Вертикальная сетка (временные метки)
    for (int i = 0; i <= 5; i++) {
        float xPos = origin.x + (i / 5.0f) * size.x;
        sf::RectangleShape gridLine(sf::Vector2f(1.f, size.y));
        gridLine.setPosition(sf::Vector2f(xPos, origin.y));
        gridLine.setFillColor(gridColor);
        window.draw(gridLine);
    }

    // Линия графика (основная визуализация)
    sf::VertexArray line(sf::PrimitiveType::LineStrip);

    for (size_t i = 0; i < data.size(); i++)
    {
        float x = origin.x + (i / (float)data.size()) * size.x;
        float y = origin.y + size.y -
                  ((data[i] - minV) / range) * size.y;

        sf::Vertex v;
        v.position = sf::Vector2f(x, y);
        v.color = color;
        line.append(v);
    }

    window.draw(line);
    
    // Точки данных (маленькие кружки в ключевых точках)
    if (data.size() > 50) {
        sf::Color pointColor = color;
        pointColor.a = 150;  // Полупрозрачные
        for (size_t i = 0; i < data.size(); i += data.size() / 10)
        {
            float x = origin.x + (i / (float)data.size()) * size.x;
            float y = origin.y + size.y -
                      ((data[i] - minV) / range) * size.y;
            
            sf::CircleShape point(2.f);
            point.setPosition(sf::Vector2f(x - 1, y - 1));
            point.setFillColor(pointColor);
            window.draw(point);
        }
    }
    
    // Минимальное и максимальное значение - горизонтальные линии
    sf::RectangleShape minLine(sf::Vector2f(size.x, 1.f));
    minLine.setPosition(sf::Vector2f(origin.x, origin.y + size.y));
    minLine.setFillColor(sf::Color(color.r / 2, color.g / 2, color.b / 2, 100));
    window.draw(minLine);
    
    sf::RectangleShape maxLine(sf::Vector2f(size.x, 1.f));
    maxLine.setPosition(sf::Vector2f(origin.x, origin.y));
    maxLine.setFillColor(sf::Color(color.r / 2, color.g / 2, color.b / 2, 100));
    window.draw(maxLine);
}