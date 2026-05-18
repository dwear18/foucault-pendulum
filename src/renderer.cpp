#include "renderer.h"
#include "pendulum.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>


// Вспомогательные функции рисования примитивов
static void fillRect(sf::RenderWindow &w, float x, float y,
                     float wd, float ht, sf::Color c)
{
    sf::RectangleShape r(sf::Vector2f(wd, ht));
    r.setPosition(sf::Vector2f(x, y));
    r.setFillColor(c);
    w.draw(r);
}

static void drawBorderRect(sf::RenderWindow &w, float x, float y,
                           float wd, float ht,
                           sf::Color fill, sf::Color border,
                           float thickness = 1.f)
{
    sf::RectangleShape r(sf::Vector2f(wd, ht));
    r.setPosition(sf::Vector2f(x, y));
    r.setFillColor(fill);
    r.setOutlineThickness(thickness);
    r.setOutlineColor(border);
    w.draw(r);
}

// Загрузка шрифта

bool Renderer::loadFont(const std::string &path)
{
    if (font.openFromFile(path))
    {
        fontLoaded = true;
        // Отключаем сглаживание для чёткого текста
        font.setSmooth(false);
        return true;
    }
    return false;
}

// Рисование текста через sf::Font

void Renderer::drawText(sf::RenderWindow &w,
                        const std::string &text,
                        float x, float y,
                        unsigned int size,
                        sf::Color color)
{
    if (!fontLoaded)
        return;

    sf::Text t(font, text, size);
    // Округляем позицию до целых пикселей - убирает размытость
    t.setPosition(sf::Vector2f(std::floor(x), std::floor(y)));
    t.setFillColor(color);
    w.draw(t);
}


// Кнопка

sf::FloatRect Renderer::drawButton(sf::RenderWindow &w,
                                   const std::string &label,
                                   float x, float y,
                                   float wd, float ht,
                                   sf::Color bg, sf::Color textColor)
{
    // Подсветка при наведении мыши
    sf::Vector2f mouse(sf::Mouse::getPosition(w));
    sf::FloatRect rect(sf::Vector2f(x, y), sf::Vector2f(wd, ht));
    bool hovered = rect.contains(mouse);

    // Немного светлее при наведении
    if (hovered)
    {
        bg.r = (uint8_t)std::min(255, (int)bg.r + 30);
        bg.g = (uint8_t)std::min(255, (int)bg.g + 30);
        bg.b = (uint8_t)std::min(255, (int)bg.b + 30);
    }

    drawBorderRect(w, x, y, wd, ht, bg,
                   sf::Color(bg.r / 2 + 60, bg.g / 2 + 60, bg.b / 2 + 60), 1.f);

    // Текст по центру кнопки
    if (fontLoaded)
    {
        sf::Text t(font, label, 14);
        sf::FloatRect tb = t.getLocalBounds();
        t.setPosition(sf::Vector2f(
            x + (wd - tb.size.x) / 2.f - tb.position.x,
            y + (ht - tb.size.y) / 2.f - tb.position.y));
        t.setFillColor(textColor);
        w.draw(t);
    }

    return rect;
}

bool Renderer::isHovered(sf::RenderWindow &w, sf::FloatRect rect)
{
    sf::Vector2f mouse(sf::Mouse::getPosition(w));
    return rect.contains(mouse);
}

// Маятник

void Renderer::drawPendulum(sf::RenderWindow &w,
                            const Pendulum &p,
                            float cx, float cy, float scale)
{
    float bx = cx + (float)p.x * scale;
    float by = cy + (float)p.y * scale;

    //  Нить: толстая линия 
    {
        float dx = bx - cx, dy = by - cy;
        float len = std::sqrt(dx * dx + dy * dy);
        if (len > 0.f)
        {
            float nx = -dy / len, ny = dx / len;
            sf::VertexArray quad(sf::PrimitiveType::TriangleStrip, 4);
            float hw = 1.5f;
            quad[0].position = {cx + nx * hw, cy + ny * hw};
            quad[1].position = {cx - nx * hw, cy - ny * hw};
            quad[2].position = {bx + nx * hw, by + ny * hw};
            quad[3].position = {bx - nx * hw, by - ny * hw};
            quad[0].color = quad[1].color = sf::Color(140, 180, 230, 200);
            quad[2].color = quad[3].color = sf::Color(90, 140, 200, 160);
            w.draw(quad);
        }
    }

    //  Точка крепления (крестик) 
    fillRect(w, cx - 5, cy - 1, 10, 2, sf::Color(160, 185, 220, 150));
    fillRect(w, cx - 1, cy - 5, 2, 10, sf::Color(160, 185, 220, 150));

    //  Вектор силы Кориолиса
    float fkx = (float)p.vy * 40.f;
    float fky = -(float)p.vx * 40.f;
    float fkLen = std::sqrt(fkx * fkx + fky * fky);
    if (fkLen > 2.f)
    {
        sf::VertexArray arr(sf::PrimitiveType::LineStrip, 2);
        arr[0].position = {bx, by};
        arr[0].color = sf::Color(200, 80, 255, 220);
        arr[1].position = {bx + fkx, by + fky};
        arr[1].color = sf::Color(200, 80, 255, 80);
        w.draw(arr);

        float nx = fkx / fkLen, ny = fky / fkLen;
        float px2 = -ny, py2 = nx;
        sf::VertexArray head(sf::PrimitiveType::Triangles, 3);
        head[0].position = {bx + fkx, by + fky};
        head[1].position = {bx + fkx - nx * 7 + px2 * 3, by + fky - ny * 7 + py2 * 3};
        head[2].position = {bx + fkx - nx * 7 - px2 * 3, by + fky - ny * 7 - py2 * 3};
        for (int i = 0; i < 3; i++)
            head[i].color = sf::Color(200, 80, 255, 180);
        w.draw(head);
    }

    //  Glow 
    for (int i = 3; i >= 1; i--)
    {
        float r = 13.f + i * 5;
        sf::CircleShape g(r);
        g.setOrigin(sf::Vector2f(r, r));
        g.setPosition(sf::Vector2f(bx, by));
        g.setFillColor(sf::Color(255, 70, 70, (uint8_t)(18 / i)));
        w.draw(g);
    }

    //  Груз 
    sf::CircleShape bob(13.f);
    bob.setOrigin(sf::Vector2f(13.f, 13.f));
    bob.setPosition(sf::Vector2f(bx, by));
    bob.setFillColor(sf::Color(225, 65, 55));
    bob.setOutlineThickness(2.f);
    bob.setOutlineColor(sf::Color(255, 150, 140));
    w.draw(bob);
}

// Траектория 

void Renderer::drawTrajectory(sf::RenderWindow &w,
                              const std::vector<double> &xs,
                              const std::vector<double> &ys,
                              sf::Color color,
                              float cx, float cy, float scale)
{
    if (xs.size() < 2)
        return;

    sf::VertexArray traj(sf::PrimitiveType::LineStrip);
    size_t n = xs.size();
    for (size_t i = 0; i < n; i++)
    {
        float t = (float)i / (float)(n - 1);
        sf::Vertex v;
        v.position = sf::Vector2f(cx + (float)xs[i] * scale,
                                  cy + (float)ys[i] * scale);
        v.color = sf::Color(color.r, color.g, color.b,
                            (uint8_t)(25 + t * 210));
        traj.append(v);
    }
    w.draw(traj);
}

// Общий метод рисования графика (внутренний)

void Renderer::drawGraph(sf::RenderWindow &w,
                         const std::vector<double> &data,
                         sf::Vector2f pos, sf::Vector2f size,
                         sf::Color color)
{
    if (data.size() < 2)
        return;

    double minV = *std::min_element(data.begin(), data.end());
    double maxV = *std::max_element(data.begin(), data.end());
    double range = (maxV == minV) ? 1.0 : (maxV - minV);

    // Сетка
    sf::Color gc(32, 38, 56);
    for (int i = 0; i <= 4; i++)
    {
        float yg = pos.y + i / 4.f * size.y;
        fillRect(w, pos.x, yg, size.x, 1.f, gc);
    }
    for (int i = 0; i <= 5; i++)
    {
        float xg = pos.x + i / 5.f * size.x;
        fillRect(w, xg, pos.y, 1.f, size.y, gc);
    }

    // Нулевая линия
    if (minV < 0 && maxV > 0)
    {
        float zy = pos.y + size.y -
                   (float)((0 - minV) / range) * size.y;
        fillRect(w, pos.x, zy, size.x, 1.f,
                 sf::Color(color.r / 3, color.g / 3, color.b / 3, 160));
    }

    // Заливка под кривой
    {
        sf::VertexArray fill(sf::PrimitiveType::TriangleStrip);
        float baseY = pos.y + size.y;
        for (size_t i = 0; i < data.size(); i++)
        {
            float xp = pos.x + (i / (float)data.size()) * size.x;
            float yp = pos.y + size.y -
                       (float)((data[i] - minV) / range) * size.y;
            sf::Vertex top, bot;
            top.position = {xp, yp};
            top.color = sf::Color(color.r, color.g, color.b, 30);
            bot.position = {xp, baseY};
            bot.color = sf::Color(color.r, color.g, color.b, 0);
            fill.append(top);
            fill.append(bot);
        }
        w.draw(fill);
    }

    // Линия графика
    {
        sf::VertexArray line(sf::PrimitiveType::LineStrip);
        for (size_t i = 0; i < data.size(); i++)
        {
            float xp = pos.x + (i / (float)data.size()) * size.x;
            float yp = pos.y + size.y -
                       (float)((data[i] - minV) / range) * size.y;
            float t = (float)i / (float)(data.size() - 1);
            sf::Vertex v;
            v.position = {xp, yp};
            v.color = sf::Color(color.r, color.g, color.b,
                                (uint8_t)(60 + t * 180));
            line.append(v);
        }
        w.draw(line);
    }

    // Живая точка в конце
    {
        float xp = pos.x + size.x - 2;
        float yp = pos.y + size.y -
                   (float)((data.back() - minV) / range) * size.y;
        sf::CircleShape dot(3.f);
        dot.setOrigin(sf::Vector2f(3.f, 3.f));
        dot.setPosition(sf::Vector2f(xp, yp));
        dot.setFillColor(color);
        w.draw(dot);
    }

    // Мин/макс подписи
    if (fontLoaded)
    {
        auto fmtV = [](double v)
        {
            std::ostringstream ss;
            ss << std::fixed << std::setprecision(3) << v;
            return ss.str();
        };
        sf::Color lc(color.r, color.g, color.b, 130);
        drawText(w, fmtV(maxV), pos.x + 3, pos.y + 1, 11, lc);
        drawText(w, fmtV(minV), pos.x + 3, pos.y + size.y - 14, 11, lc);
    }
}

// Три публичных метода по UML

void Renderer::drawGraphXvsTime(sf::RenderWindow &w,
                                const std::vector<double> &data,
                                sf::Vector2f pos, sf::Vector2f size,
                                sf::Color color)
{
    drawGraph(w, data, pos, size, color);
}

void Renderer::drawGraphYvsTime(sf::RenderWindow &w,
                                const std::vector<double> &data,
                                sf::Vector2f pos, sf::Vector2f size,
                                sf::Color color)
{
    drawGraph(w, data, pos, size, color);
}

void Renderer::drawGraphAlphaVsTime(sf::RenderWindow &w,
                                    const std::vector<double> &data,
                                    sf::Vector2f pos, sf::Vector2f size,
                                    sf::Color color)
{
    drawGraph(w, data, pos, size, color);
}