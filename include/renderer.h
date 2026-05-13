#pragma once

#include <SFML/Graphics.hpp>
#include <vector>

class Renderer
{
public:
    void drawPendulum(sf::RenderWindow &, double, double);

    void drawTrajectory(sf::RenderWindow &, const std::vector<double> &, const std::vector<double> &);

    void drawGraph(sf::RenderWindow &, const std::vector<double> &, sf::Vector2f, sf::Vector2f, sf::Color);
};