#include <SFML/Graphics.hpp>
#include <optional>
#include <cmath>
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include "simulation.h"
#include "renderer.h"

// Рисует простой текст из прямоугольников (не требует шрифтов)
void drawSimpleRect(sf::RenderWindow& window, float x, float y, float w, float h, sf::Color color)
{
    sf::RectangleShape rect(sf::Vector2f(w, h));
    rect.setPosition(sf::Vector2f(x, y));
    rect.setFillColor(color);
    window.draw(rect);
}

int main()
{
    std::cout << "Starting FoucaultPendulum..." << std::endl;
    
    try {
        std::cout << "Creating window..." << std::endl;
        sf::RenderWindow window(
            sf::VideoMode(sf::Vector2u(1400u, 900u)),
            std::string("Foucault Pendulum Simulation"));
        window.setFramerateLimit(60);
        std::cout << "Window created successfully." << std::endl;
        
        std::cout << "Initializing simulation..." << std::endl;
        Simulation sim;
        Renderer renderer;
        
        sim.start();
        sim.physics.setLatitude(45.0);
        sim.pendulum.L = 2.5;
        sim.pendulum.m = 1.0;
        sim.pendulum.x = 0.1;
        sim.pendulum.y = 0.0;
        sim.dt = 0.01;
        sim.timeScale = 1.0;
        
        std::cout << "Entering main loop..." << std::endl;
        sf::Clock clock;
        bool paused = false;
        bool showInfo = true;
        
        while (window.isOpen())
        {
            while (const std::optional event = window.pollEvent())
            {
                if (event->is<sf::Event::Closed>())
                    window.close();
                
                if (const auto* keyEvent = event->getIf<sf::Event::KeyPressed>())
                {
                    if (keyEvent->code == sf::Keyboard::Key::Space) {
                        paused = !paused;
                    }
                    if (keyEvent->code == sf::Keyboard::Key::R) {
                        sim.start();
                    }
                    // Управление скоростью: + увеличить, - уменьшить
                    if (keyEvent->code == sf::Keyboard::Key::Add || 
                        keyEvent->code == sf::Keyboard::Key::Equal) {
                        sim.timeScale = std::min(10.0, sim.timeScale * 1.2);
                    }
                    if (keyEvent->code == sf::Keyboard::Key::Subtract) {
                        sim.timeScale = std::max(0.1, sim.timeScale / 1.2);
                    }
                    if (keyEvent->code == sf::Keyboard::Key::Num1) {
                        sim.timeScale = 1.0;  // Нормальная скорость
                    }
                    if (keyEvent->code == sf::Keyboard::Key::I) {
                        showInfo = !showInfo;
                    }
                }
            }
            
            if (!paused) {
                double dt = clock.restart().asSeconds();
                if (dt > 0.1) dt = 0.1;
                sim.update(dt);
            } else {
                clock.restart();
            }
            
            window.clear(sf::Color(20, 20, 25));
            
            // === ЛЕВАЯ ПАНЕЛЬ: ТРАЕКТОРИЯ ===
            // Фон траектории
            sf::RectangleShape trajBg(sf::Vector2f(450.f, 700.f));
            trajBg.setPosition(sf::Vector2f(10.f, 80.f));
            trajBg.setFillColor(sf::Color(35, 35, 45));
            trajBg.setOutlineThickness(2.f);
            trajBg.setOutlineColor(sf::Color(100, 150, 200));
            window.draw(trajBg);
            
            // Сетка в области траектории
            sf::Color gridColor(60, 60, 70);
            for (int i = 0; i < 450; i += 75) {
                sf::RectangleShape line(sf::Vector2f(1.f, 700.f));
                line.setPosition(sf::Vector2f(10.f + i, 80.f));
                line.setFillColor(gridColor);
                window.draw(line);
            }
            for (int i = 0; i < 700; i += 75) {
                sf::RectangleShape line(sf::Vector2f(450.f, 1.f));
                line.setPosition(sf::Vector2f(10.f, 80.f + i));
                line.setFillColor(gridColor);
                window.draw(line);
            }
            
            // Центр подвеса маятника
            sf::CircleShape center(5.f);
            center.setPosition(sf::Vector2f(230.f, 425.f));
            center.setFillColor(sf::Color::White);
            window.draw(center);
            
            // Визуализация траектории и маятника
            renderer.drawTrajectory(window, sim.historyX, sim.historyY);
            renderer.drawPendulum(window, sim.pendulum.x, sim.pendulum.y);
            
            // === ПРАВАЯ ПАНЕЛЬ: ГРАФИКИ ===
            // График X(t) - КРАСНЫЙ
            sf::RectangleShape graphBg1(sf::Vector2f(920.f, 210.f));
            graphBg1.setPosition(sf::Vector2f(470.f, 80.f));
            graphBg1.setFillColor(sf::Color(35, 35, 45));
            graphBg1.setOutlineThickness(2.f);
            graphBg1.setOutlineColor(sf::Color(200, 100, 100));
            window.draw(graphBg1);
            renderer.drawGraph(window, sim.historyX, sf::Vector2f(480, 90), sf::Vector2f(900, 190), sf::Color(255, 100, 100));
            
            // Подпись графика X(t)
            drawSimpleRect(window, 475, 85, 8, 8, sf::Color(255, 100, 100));  // Цветной квадрат
            drawSimpleRect(window, 488, 85, 50, 2, sf::Color(150, 150, 150)); // Линия подписи
            
            // График Y(t) - ЗЕЛЕНЫЙ
            sf::RectangleShape graphBg2(sf::Vector2f(920.f, 210.f));
            graphBg2.setPosition(sf::Vector2f(470.f, 300.f));
            graphBg2.setFillColor(sf::Color(35, 35, 45));
            graphBg2.setOutlineThickness(2.f);
            graphBg2.setOutlineColor(sf::Color(100, 200, 100));
            window.draw(graphBg2);
            renderer.drawGraph(window, sim.historyY, sf::Vector2f(480, 310), sf::Vector2f(900, 190), sf::Color(100, 255, 100));
            
            // Подпись графика Y(t)
            drawSimpleRect(window, 475, 305, 8, 8, sf::Color(100, 255, 100));  // Цветной квадрат
            drawSimpleRect(window, 488, 305, 50, 2, sf::Color(150, 150, 150)); // Линия подписи
            
            // График Alpha(t) - ЖЁЛТЫЙ
            sf::RectangleShape graphBg3(sf::Vector2f(920.f, 210.f));
            graphBg3.setPosition(sf::Vector2f(470.f, 520.f));
            graphBg3.setFillColor(sf::Color(35, 35, 45));
            graphBg3.setOutlineThickness(2.f);
            graphBg3.setOutlineColor(sf::Color(200, 200, 100));
            window.draw(graphBg3);
            renderer.drawGraph(window, sim.historyAlpha, sf::Vector2f(480, 530), sf::Vector2f(900, 190), sf::Color(255, 255, 100));
            
            // Подпись графика Alpha(t)
            drawSimpleRect(window, 475, 525, 8, 8, sf::Color(255, 255, 100));  // Цветной квадрат
            drawSimpleRect(window, 488, 525, 50, 2, sf::Color(150, 150, 150)); // Линия подписи
            
            // === ВЕРХНЯЯ ПАНЕЛЬ: УПРАВЛЕНИЕ И ИНФОРМАЦИЯ ===
            sf::RectangleShape topPanel(sf::Vector2f(1400.f, 70.f));
            topPanel.setFillColor(sf::Color(25, 35, 55));
            topPanel.setOutlineThickness(2.f);
            topPanel.setOutlineColor(sf::Color(80, 120, 180));
            window.draw(topPanel);
            
            // === НИЖНЯЯ ПАНЕЛЬ: ИНФОРМАЦИЯ И УПРАВЛЕНИЕ ===
            sf::RectangleShape bottomPanel(sf::Vector2f(1400.f, 80.f));
            bottomPanel.setPosition(sf::Vector2f(0.f, 820.f));
            bottomPanel.setFillColor(sf::Color(25, 35, 55));
            bottomPanel.setOutlineThickness(2.f);
            bottomPanel.setOutlineColor(sf::Color(80, 120, 180));
            window.draw(bottomPanel);
            
            // Индикатор паузы
            if (paused) {
                drawSimpleRect(window, 20.f, 30.f, 10.f, 10.f, sf::Color::Red);
                drawSimpleRect(window, 35.f, 30.f, 10.f, 10.f, sf::Color::Red);
            } else {
                sf::CircleShape playInd(5.f);
                playInd.setPosition(sf::Vector2f(20.f, 30.f));
                playInd.setFillColor(sf::Color::Green);
                window.draw(playInd);
            }
            
            // Информационные полоски о скорости симуляции
            float speedBarWidth = static_cast<float>(sim.timeScale) * 30.f;
            sf::RectangleShape speedBar(sf::Vector2f(speedBarWidth, 8.f));
            speedBar.setPosition(sf::Vector2f(60.f, 33.f));
            speedBar.setFillColor(sf::Color(150, 200, 255));
            window.draw(speedBar);
            
            // Фоновая полоса для спидбара
            sf::RectangleShape speedBarBg(sf::Vector2f(300.f, 8.f));
            speedBarBg.setPosition(sf::Vector2f(60.f, 33.f));
            speedBarBg.setFillColor(sf::Color(50, 50, 60));
            speedBarBg.setOutlineThickness(1.f);
            speedBarBg.setOutlineColor(sf::Color(100, 100, 120));
            window.draw(speedBarBg);
            window.draw(speedBar);
            
            // Показатели здоровья симуляции
            // Количество точек истории
            int histSize = static_cast<int>(sim.historyX.size());
            drawSimpleRect(window, 380.f, 30.f, static_cast<float>(histSize) / 20.f, 12.f, sf::Color(100, 200, 150));
            
            // Информация о времени симуляции
            drawSimpleRect(window, 650.f, 30.f, 3.f, 12.f, sf::Color(150, 150, 200));
            
            // Параметры симуляции (визуальные полоски)
            // Latitude
            float latBar = (45.0f + 90.f) / 180.f * 100.f;
            sf::RectangleShape latBarShape(sf::Vector2f(latBar, 6.f));
            latBarShape.setPosition(sf::Vector2f(800.f, 32.f));
            latBarShape.setFillColor(sf::Color(200, 150, 100));
            window.draw(latBarShape);
            
            // Pendulum length
            float lenBar = (sim.pendulum.L / 10.f) * 100.f;
            sf::RectangleShape lenBarShape(sf::Vector2f(lenBar, 6.f));
            lenBarShape.setPosition(sf::Vector2f(950.f, 32.f));
            lenBarShape.setFillColor(sf::Color(100, 150, 200));
            window.draw(lenBarShape);
            
            // Разделительные линии
            sf::RectangleShape divider1(sf::Vector2f(2.f, 900.f));
            divider1.setPosition(sf::Vector2f(463.f, 0.f));
            divider1.setFillColor(sf::Color(100, 120, 140));
            window.draw(divider1);
            
            sf::RectangleShape divider2(sf::Vector2f(1400.f, 2.f));
            divider2.setPosition(sf::Vector2f(0.f, 79.f));
            divider2.setFillColor(sf::Color(100, 120, 140));
            window.draw(divider2);
            
            sf::RectangleShape divider3(sf::Vector2f(1400.f, 2.f));
            divider3.setPosition(sf::Vector2f(0.f, 819.f));
            divider3.setFillColor(sf::Color(100, 120, 140));
            window.draw(divider3);
            
            // Декоративные элементы - "мониторы" для каждого графика
            for (int i = 0; i < 3; i++) {
                float yPos = 80.f + i * 220.f;
                sf::RectangleShape cornerTL(sf::Vector2f(10.f, 2.f));
                cornerTL.setPosition(sf::Vector2f(470.f, yPos));
                cornerTL.setFillColor(sf::Color(100, 150, 200));
                window.draw(cornerTL);
                
                sf::RectangleShape cornerTL2(sf::Vector2f(2.f, 10.f));
                cornerTL2.setPosition(sf::Vector2f(470.f, yPos));
                cornerTL2.setFillColor(sf::Color(100, 150, 200));
                window.draw(cornerTL2);
            }
            
            window.display();
        }
        
        std::cout << "Exiting normally." << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
