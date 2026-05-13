#include <SFML/Graphics.hpp>
#include <optional>
#include <cmath>
#include <iostream>
#include <string>
#include "simulation.h"
#include "renderer.h"

int main()
{
    std::cout << "Starting FoucaultPendulum..." << std::endl;
    
    try {
        std::cout << "Creating window..." << std::endl;
        sf::RenderWindow window(
            sf::VideoMode(sf::Vector2u(1200u, 800u)),
            std::string("Foucault Pendulum Simulation"));
        window.setFramerateLimit(30);
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
                }
            }
            
            if (!paused) {
                double dt = clock.restart().asSeconds();
                if (dt > 0.1) dt = 0.1;
                sim.update(dt);
            } else {
                clock.restart();
            }
            
            window.clear(sf::Color::Black);
            
            // Draw simple trajectory area
            renderer.drawTrajectory(window, sim.historyX, sim.historyY);
            renderer.drawPendulum(window, sim.pendulum.x, sim.pendulum.y);
            
            // Draw simple graphs
            renderer.drawGraph(window, sim.historyX, sf::Vector2f(420, 100), sf::Vector2f(760, 200), sf::Color::Red);
            renderer.drawGraph(window, sim.historyY, sf::Vector2f(420, 320), sf::Vector2f(760, 200), sf::Color::Green);
            renderer.drawGraph(window, sim.historyAlpha, sf::Vector2f(420, 540), sf::Vector2f(760, 150), sf::Color::Yellow);
            
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