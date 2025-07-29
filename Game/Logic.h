#pragma once
#include <random>
#include <vector>

#include "../Models/Move.h"
#include "Board.h"
#include "Config.h"

const int INF = 1e9;

class Logic
{
public:
    Logic(Board* board, Config* config) : board(board), config(config)
    {
        rand_eng = std::default_random_engine(
            !((*config)("Bot", "NoRandom")) ? unsigned(time(0)) : 0);
        scoring_mode = (*config)("Bot", "BotScoringType");
        optimization = (*config)("Bot", "Optimization");
    }

private:
    vector<vector<POS_T>> make_turn(vector<vector<POS_T>> mtx, move_pos turn) const
    {
        if (turn.xb != -1) // Если в ходе есть побитая фигура — удаляем её с доски
            mtx[turn.xb][turn.yb] = 0;
        // Проверяем превращение шашки в дамку:
        // белая шашка (1) достигает верхнего ряда (x2 == 0)
        // чёрная шашка (2) достигает нижнего ряда (x2 == 7)
        if ((mtx[turn.x][turn.y] == 1 && turn.x2 == 0) || (mtx[turn.x][turn.y] == 2 && turn.x2 == 7))
            mtx[turn.x][turn.y] += 2;
        mtx[turn.x2][turn.y2] = mtx[turn.x][turn.y]; // Перемещаем фигуру из старой клетки в новую
        mtx[turn.x][turn.y] = 0; // Очищаем старую клетку
        return mtx; // Возвращаем новое состояние доски
    }

    double calc_score(const vector<vector<POS_T>>& mtx, const bool first_bot_color) const
    {
        // color - who is max player
        double w = 0, wq = 0, b = 0, bq = 0; // Счёт для обычных белых/черных 1 и белых/черных дамок 3
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                // Считаем количество обычных и дамок у каждой стороны
                w += (mtx[i][j] == 1);
                wq += (mtx[i][j] == 3);
                b += (mtx[i][j] == 2);
                bq += (mtx[i][j] == 4);
                if (scoring_mode == "NumberAndPotential") // Если режим оценки, по числу и потенциалу
                {
                    // Добавляем "потенциал продвижения" для обычных шашек
                    // Чем ближе к превращению в дамку, тем выше вес
                    w += 0.05 * (mtx[i][j] == 1) * (7 - i);
                    b += 0.05 * (mtx[i][j] == 2) * (i);
                }
            }
        }
        if (!first_bot_color)
        {
            swap(b, w); // меняем местами счёт белых и чёрных
            swap(bq, wq); // аналогично — для дамок
        }
        if (w + wq == 0)
            return INF; // У противника нет фигур, победа
        if (b + bq == 0)
            return 0; // У бота нет фигур, поражение
        int q_coef = 4; // вес дамки 
        if (scoring_mode == "NumberAndPotential")
        {
            q_coef = 5; // если учитываем потенциал, дамки становятся ещё важнее
        }
        return (b + bq * q_coef) / (w + wq * q_coef); // Возвращаем итоговую оценку позиции как отношение сил
    }

public:
    void find_turns(const bool color) // Функция используется для указания возможных ходов фигур указанного цвета
    {
        find_turns(color, board->get_board());
    }

    void find_turns(const POS_T x, const POS_T y) // Функция используется для указания возможных ходов одной фигуры в клетке x, y
    {
        find_turns(x, y, board->get_board());
    }

private:
    void find_turns(const bool color, const vector<vector<POS_T>>& mtx) // Основная логика перебора всех фигур указанного цвета на доске
    {
        vector<move_pos> res_turns;
        bool have_beats_before = false;
        for (POS_T i = 0; i < 8; ++i) // Обходит все клетки
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                if (mtx[i][j] && mtx[i][j] % 2 != color)
                {
                    find_turns(i, j, mtx);
                    if (have_beats && !have_beats_before) // Если найден хотя бы один бой
                    {
                        have_beats_before = true;
                        res_turns.clear(); // Cбрасываются обычные ходы
                    }
                    if ((have_beats_before && have_beats) || !have_beats_before)
                    {
                        res_turns.insert(res_turns.end(), turns.begin(), turns.end()); // Записывет ход
                    }
                }
            }
        }
        turns = res_turns; // Сохраняются все найденные ходы
        shuffle(turns.begin(), turns.end(), rand_eng); // Ходы перемешиваются
        have_beats = have_beats_before;
    }

    void find_turns(const POS_T x, const POS_T y, const vector<vector<POS_T>>& mtx)
    {
        turns.clear();
        have_beats = false;
        POS_T type = mtx[x][y]; // Получаем тип фигуры на указанной клетке (1,2 — шашки, 3,4 — дамки)
        // check beats
        switch (type)
        {
        case 1:
        case 2:
            // check pieces
            for (POS_T i = x - 2; i <= x + 2; i += 4)
            {
                for (POS_T j = y - 2; j <= y + 2; j += 4)
                {
                    if (i < 0 || i > 7 || j < 0 || j > 7)
                        continue; // Пропускаем выход за границы
                    POS_T xb = (x + i) / 2, yb = (y + j) / 2; // Координаты возможной убитой фигуры 
                    if (mtx[i][j] || !mtx[xb][yb] || mtx[xb][yb] % 2 == type % 2)
                        continue; // Целевая клетка занят, между — никого, между — своя фигура
                    turns.emplace_back(x, y, i, j, xb, yb); // Добавляем ход с взятием
                }
            }
            break;
        default:
            // check queens дамка
            for (POS_T i = -1; i <= 1; i += 2) // Перебор по диагоналям.
            {
                for (POS_T j = -1; j <= 1; j += 2)
                {
                    POS_T xb = -1, yb = -1; // Координаты убиваемой фигуры
                    for (POS_T i2 = x + i, j2 = y + j; i2 != 8 && j2 != 8 && i2 != -1 && j2 != -1; i2 += i, j2 += j)
                    {
                        if (mtx[i2][j2])
                        {
                            if (mtx[i2][j2] % 2 == type % 2 || (mtx[i2][j2] % 2 != type % 2 && xb != -1)) // Если фигура своя — нельзя бить, прерываем, если уже есть побитая и встречаем ещё одну — прерываем
                            {
                                break;
                            }
                            xb = i2; // Запоминаем возможную побитую фигуру
                            yb = j2;
                        }
                        if (xb != -1 && xb != i2) // Если уже есть побитая и прошли через неё - добавляем ход
                        {
                            turns.emplace_back(x, y, i2, j2, xb, yb);
                        }
                    }
                }
            }
            break;
        }
        // check other turns
        if (!turns.empty()) // Обнаружены обязательные бои
        {
            have_beats = true;
            return;
        }
        switch (type)
        {
        case 1:
        case 2:
            // check pieces
        {
            POS_T i = ((type % 2) ? x - 1 : x + 1); // Белые ходят вверх, чёрные вниз
            for (POS_T j = y - 1; j <= y + 1; j += 2)
            {
                if (i < 0 || i > 7 || j < 0 || j > 7 || mtx[i][j])
                    continue; // Пропускаем, если выход за границы или клетка занята
                turns.emplace_back(x, y, i, j); // Добавляем обычный ход
            }
            break;
        }
        default:
            // check queens дамки могут двигаться на любое количество пустых клеток по диагонали
            for (POS_T i = -1; i <= 1; i += 2)
            {
                for (POS_T j = -1; j <= 1; j += 2)
                {
                    for (POS_T i2 = x + i, j2 = y + j; i2 != 8 && j2 != 8 && i2 != -1 && j2 != -1; i2 += i, j2 += j)
                    {
                        if (mtx[i2][j2])
                            break; // Прерываем, если клетка занята
                        turns.emplace_back(x, y, i2, j2); // Добавляем обычный ход дамки
                    }
                }
            }
            break;
        }
    }

public:
    vector<move_pos> turns; // Список всех допустимых ходов
    bool have_beats; //Флаг, указывающий, есть ли обязательные бои
    int Max_depth; // Максимальная глубина рекурсивного анализа

private:
    default_random_engine rand_eng; // Генератор случайных чисел для перемешивания ходов
    string scoring_mode; // Режим оценки позиций
    string optimization; // Режим оптимизации поиска
    vector<move_pos> next_move; // Структура хода
    vector<int> next_best_state; // Структура состояния после хода
    Board* board; // Указатель на игровую доску
    Config* config; // Указатель на объект конфигурации
};
