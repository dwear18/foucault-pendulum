#include "renderer.h"
#include "pendulum.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>

// ============================================================
// Пиксельный шрифт 5x7 - позволяет рисовать текст без файла шрифта.
// Каждый символ - 5 байт (один на столбец), бит 0 = верхняя строка.
// Индекс массива = ASCII-код символа.
// ============================================================
static const uint8_t FONT[128][5] = {
    // 0-31: управляющие символы (пустые)
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    // 32-127: печатаемые символы
    {0x00, 0x00, 0x00, 0x00, 0x00}, // 32 ' '
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // 33 '!'
    {0x00, 0x07, 0x00, 0x07, 0x00}, // 34 '"'
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // 35 '#'
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // 36 '$'
    {0x23, 0x13, 0x08, 0x64, 0x62}, // 37 '%'
    {0x36, 0x49, 0x55, 0x22, 0x50}, // 38 '&'
    {0x00, 0x05, 0x03, 0x00, 0x00}, // 39 '\''
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // 40 '('
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // 41 ')'
    {0x14, 0x08, 0x3E, 0x08, 0x14}, // 42 '*'
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // 43 '+'
    {0x00, 0x50, 0x30, 0x00, 0x00}, // 44 ','
    {0x08, 0x08, 0x08, 0x08, 0x08}, // 45 '-'
    {0x00, 0x60, 0x60, 0x00, 0x00}, // 46 '.'
    {0x20, 0x10, 0x08, 0x04, 0x02}, // 47 '/'
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 48 '0'
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 49 '1'
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 50 '2'
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 51 '3'
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 52 '4'
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 53 '5'
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 54 '6'
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 55 '7'
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 56 '8'
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 57 '9'
    {0x00, 0x36, 0x36, 0x00, 0x00}, // 58 ':'
    {0x00, 0x56, 0x36, 0x00, 0x00}, // 59 ';'
    {0x08, 0x14, 0x22, 0x41, 0x00}, // 60 '<'
    {0x14, 0x14, 0x14, 0x14, 0x14}, // 61 '='
    {0x00, 0x41, 0x22, 0x14, 0x08}, // 62 '>'
    {0x02, 0x01, 0x51, 0x09, 0x06}, // 63 '?'
    {0x32, 0x49, 0x79, 0x41, 0x3E}, // 64 '@'
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // 65 'A'
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // 66 'B'
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // 67 'C'
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // 68 'D'
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // 69 'E'
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // 70 'F'
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // 71 'G'
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // 72 'H'
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // 73 'I'
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // 74 'J'
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // 75 'K'
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // 76 'L'
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // 77 'M'
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // 78 'N'
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // 79 'O'
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // 80 'P'
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // 81 'Q'
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // 82 'R'
    {0x46, 0x49, 0x49, 0x49, 0x31}, // 83 'S'
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // 84 'T'
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // 85 'U'
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // 86 'V'
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, // 87 'W'
    {0x63, 0x14, 0x08, 0x14, 0x63}, // 88 'X'
    {0x07, 0x08, 0x70, 0x08, 0x07}, // 89 'Y'
    {0x61, 0x51, 0x49, 0x45, 0x43}, // 90 'Z'
    {0x00, 0x7F, 0x41, 0x41, 0x00}, // 91 '['
    {0x02, 0x04, 0x08, 0x10, 0x20}, // 92 '\'
    {0x00, 0x41, 0x41, 0x7F, 0x00}, // 93 ']'
    {0x04, 0x02, 0x01, 0x02, 0x04}, // 94 '^'
    {0x40, 0x40, 0x40, 0x40, 0x40}, // 95 '_'
    {0x00, 0x01, 0x02, 0x04, 0x00}, // 96 '`'
    {0x20, 0x54, 0x54, 0x54, 0x78}, // 97 'a'
    {0x7F, 0x48, 0x44, 0x44, 0x38}, // 98 'b'
    {0x38, 0x44, 0x44, 0x44, 0x20}, // 99 'c'
    {0x38, 0x44, 0x44, 0x48, 0x7F}, // 100 'd'
    {0x38, 0x54, 0x54, 0x54, 0x18}, // 101 'e'
    {0x08, 0x7E, 0x09, 0x01, 0x02}, // 102 'f'
    {0x0C, 0x52, 0x52, 0x52, 0x3E}, // 103 'g'
    {0x7F, 0x08, 0x04, 0x04, 0x78}, // 104 'h'
    {0x00, 0x44, 0x7D, 0x40, 0x00}, // 105 'i'
    {0x20, 0x40, 0x44, 0x3D, 0x00}, // 106 'j'
    {0x7F, 0x10, 0x28, 0x44, 0x00}, // 107 'k'
    {0x00, 0x41, 0x7F, 0x40, 0x00}, // 108 'l'
    {0x7C, 0x04, 0x18, 0x04, 0x78}, // 109 'm'
    {0x7C, 0x08, 0x04, 0x04, 0x78}, // 110 'n'
    {0x38, 0x44, 0x44, 0x44, 0x38}, // 111 'o'
    {0x7C, 0x14, 0x14, 0x14, 0x08}, // 112 'p'
    {0x08, 0x14, 0x14, 0x18, 0x7C}, // 113 'q'
    {0x7C, 0x08, 0x04, 0x04, 0x08}, // 114 'r'
    {0x48, 0x54, 0x54, 0x54, 0x20}, // 115 's'
    {0x04, 0x3F, 0x44, 0x40, 0x20}, // 116 't'
    {0x3C, 0x40, 0x40, 0x20, 0x7C}, // 117 'u'
    {0x1C, 0x20, 0x40, 0x20, 0x1C}, // 118 'v'
    {0x3C, 0x40, 0x30, 0x40, 0x3C}, // 119 'w'
    {0x44, 0x28, 0x10, 0x28, 0x44}, // 120 'x'
    {0x0C, 0x50, 0x50, 0x50, 0x3C}, // 121 'y'
    {0x44, 0x64, 0x54, 0x4C, 0x44}, // 122 'z'
    {0x00, 0x08, 0x36, 0x41, 0x00}, // 123 '{'
    {0x00, 0x00, 0x7F, 0x00, 0x00}, // 124 '|'
    {0x00, 0x41, 0x36, 0x08, 0x00}, // 125 '}'
    {0x10, 0x08, 0x08, 0x10, 0x08}, // 126 '~'
    {0x00, 0x00, 0x00, 0x00, 0x00}, // 127 DEL
};

// ============================================================
// Вспомогательные функции рисования примитивов
// ============================================================

// Нарисовать закрашенный прямоугольник
static void rect(sf::RenderWindow &w, float x, float y,
                 float wd, float ht, sf::Color c)
{
    sf::RectangleShape r(sf::Vector2f(wd, ht));
    r.setPosition(sf::Vector2f(x, y));
    r.setFillColor(c);
    w.draw(r);
}

// ============================================================
// Пиксельный текст
// ============================================================

void Renderer::drawChar(sf::RenderWindow &w, char ch,
                        float x, float y, float s, sf::Color c)
{
    // Рисуем символ по битовой карте шрифта
    unsigned char idx = (unsigned char)ch;
    if (idx > 127)
        return;
    for (int col = 0; col < 5; col++)
        for (int row = 0; row < 7; row++)
            if (FONT[idx][col] & (1 << row))
                rect(w, x + col * s, y + row * s, s, s, c);
}

void Renderer::drawText(sf::RenderWindow &w, const std::string &text,
                        float x, float y, float scale, sf::Color color)
{
    float cx = x;
    for (char c : text)
    {
        drawChar(w, c, cx, y, scale, color);
        cx += 6 * scale; // 5px символ + 1px пробел
    }
}

// ============================================================
// Маятник (по UML: drawPendulum)
// cx, cy - экранные координаты точки подвеса
// scale  - пикселей на метр
// ============================================================
void Renderer::drawPendulum(sf::RenderWindow &w,
                            const Pendulum &p,
                            float cx, float cy, float scale)
{
    // Экранные координаты груза
    float bx = cx + (float)p.x * scale;
    float by = cy + (float)p.y * scale;

    // --- Нить маятника ---
    {
        // Рисуем как тонкий прямоугольник для толщины
        float dx = bx - cx, dy = by - cy;
        float len = std::sqrt(dx * dx + dy * dy);
        if (len > 0.f)
        {
            float nx = -dy / len, ny = dx / len; // нормаль
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

    // --- Точка крепления (крестик сверху) ---
    rect(w, cx - 5, cy - 1, 10, 2, sf::Color(160, 185, 220, 160));
    rect(w, cx - 1, cy - 5, 2, 10, sf::Color(160, 185, 220, 160));

    // --- Вектор силы Кориолиса (п. 11 ТЗ) ---
    // Сила Кориолиса перпендикулярна скорости: F = 2m(v x Omega)
    // В 2D проекции: Fx ~ vy, Fy ~ -vx
    float fkx = (float)p.vy * 40.f;
    float fky = -(float)p.vx * 40.f;
    float fkLen = std::sqrt(fkx * fkx + fky * fky);
    if (fkLen > 2.f)
    {
        // Стрелка
        sf::VertexArray arr(sf::PrimitiveType::LineStrip, 2);
        arr[0].position = {bx, by};
        arr[0].color = sf::Color(200, 80, 255, 220);
        arr[1].position = {bx + fkx, by + fky};
        arr[1].color = sf::Color(200, 80, 255, 80);
        w.draw(arr);

        // Наконечник стрелки
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

    // --- Glow вокруг груза ---
    for (int i = 3; i >= 1; i--)
    {
        float r = 13.f + i * 5;
        sf::CircleShape g(r);
        g.setOrigin(sf::Vector2f(r, r));
        g.setPosition(sf::Vector2f(bx, by));
        g.setFillColor(sf::Color(255, 70, 70, (uint8_t)(18 / i)));
        w.draw(g);
    }

    // --- Груз ---
    {
        sf::CircleShape bob(13.f);
        bob.setOrigin(sf::Vector2f(13.f, 13.f));
        bob.setPosition(sf::Vector2f(bx, by));
        bob.setFillColor(sf::Color(225, 65, 55));
        bob.setOutlineThickness(2.f);
        bob.setOutlineColor(sf::Color(255, 150, 140));
        w.draw(bob);
        // Блик
        sf::CircleShape shine(3.5f);
        shine.setOrigin(sf::Vector2f(3.5f, 3.5f));
        shine.setPosition(sf::Vector2f(bx - 4.f, by - 5.f));
        shine.setFillColor(sf::Color(255, 220, 220, 100));
        w.draw(shine);
    }
}

// ============================================================
// Траектория (по UML: drawTrajectory)
// ============================================================
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
        // Старые точки прозрачнее, новые - ярче
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

// ============================================================
// Общий метод рисования графика (внутренний)
// ============================================================
void Renderer::drawGraph(sf::RenderWindow &w,
                         const std::vector<double> &data,
                         sf::Vector2f pos, sf::Vector2f size,
                         sf::Color color)
{
    if (data.size() < 2)
        return;

    // Найти диапазон значений
    double minV = *std::min_element(data.begin(), data.end());
    double maxV = *std::max_element(data.begin(), data.end());
    double range = (maxV == minV) ? 1.0 : (maxV - minV);

    // --- Сетка ---
    sf::Color gc(35, 40, 58);
    for (int i = 0; i <= 4; i++)
    {
        float yg = pos.y + i / 4.f * size.y;
        rect(w, pos.x, yg, size.x, 1.f, gc);
    }
    for (int i = 0; i <= 5; i++)
    {
        float xg = pos.x + i / 5.f * size.x;
        rect(w, xg, pos.y, 1.f, size.y, gc);
    }

    // --- Нулевая линия (если график проходит через ноль) ---
    if (minV < 0 && maxV > 0)
    {
        float zy = pos.y + size.y -
                   (float)((0 - minV) / range) * size.y;
        rect(w, pos.x, zy, size.x, 1.f,
             sf::Color(color.r / 3, color.g / 3, color.b / 3, 160));
    }

    // --- Заливка под кривой ---
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
            top.color = sf::Color(color.r, color.g, color.b, 32);
            bot.position = {xp, baseY};
            bot.color = sf::Color(color.r, color.g, color.b, 0);
            fill.append(top);
            fill.append(bot);
        }
        w.draw(fill);
    }

    // --- Линия графика ---
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

    // --- Текущая точка (живой индикатор) ---
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

    // --- Подписи мин/макс ---
    {
        std::ostringstream ss;
        sf::Color lc(color.r, color.g, color.b, 120);
        ss << std::fixed << std::setprecision(3) << maxV;
        drawText(w, ss.str(), pos.x + 3, pos.y + 2, 1.3f, lc);
        ss.str("");
        ss << std::fixed << std::setprecision(3) << minV;
        drawText(w, ss.str(), pos.x + 3, pos.y + size.y - 12, 1.3f, lc);
    }
}

// ============================================================
// Три публичных метода по UML
// ============================================================

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