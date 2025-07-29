#pragma once
#include <stdlib.h>

typedef int8_t POS_T; // тип координат клетки (от -128 до 127)

struct move_pos
{
    POS_T x, y;             // from - исходная клетка
    POS_T x2, y2;           // to - целевая клетка
    POS_T xb = -1, yb = -1; // beaten - координаты убитой фигуры

    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2) : x(x), y(y), x2(x2), y2(y2) // Конструктор без взятия: задаем только откуда и куда
    {
    }
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2, const POS_T xb, const POS_T yb) // Конструктор с взятием
        : x(x), y(y), x2(x2), y2(y2), xb(xb), yb(yb)
    {
    }
    // Сравнение двух ходов
    bool operator==(const move_pos& other) const
    {
        return (x == other.x && y == other.y && x2 == other.x2 && y2 == other.y2);
    }
    bool operator!=(const move_pos& other) const // Возвращает true, если ходы отличаются
    {
        return !(*this == other);
    }
};
