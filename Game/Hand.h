#pragma once
#include <tuple>

#include "../Models/Move.h"
#include "../Models/Response.h"
#include "Board.h"

// methods for hands
class Hand
{
public:
    Hand(Board* board) : board(board) // Cохраняет указатель на объект доски
    {
    }
    tuple<Response, POS_T, POS_T> get_cell() const // Обработка клика игрока по доске или кнопке
    {
        SDL_Event windowEvent;
        Response resp = Response::OK;
        int x = -1, y = -1; // Пиксельные координаты мыши.
        int xc = -1, yc = -1; // Координаты клетки на доске
        while (true) // Основной цикл обработки событий
        {
            if (SDL_PollEvent(&windowEvent)) // Проверяем, есть ли событие в очереди
            {
                switch (windowEvent.type)
                {
                case SDL_QUIT:
                    resp = Response::QUIT; // Завершение игры
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    // Переводим координаты мыши в координаты клетки доски
                    x = windowEvent.motion.x;
                    y = windowEvent.motion.y;
                    xc = int(y / (board->H / 10) - 1);
                    yc = int(x / (board->W / 10) - 1);
                    if (xc == -1 && yc == -1 && board->history_mtx.size() > 1)
                    {
                        resp = Response::BACK; // Возврат хода
                    }
                    else if (xc == -1 && yc == 8)
                    {
                        resp = Response::REPLAY; // Переиграть
                    }
                    else if (xc >= 0 && xc < 8 && yc >= 0 && yc < 8)
                    {
                        resp = Response::CELL; // Клик по клетке доски
                    }
                    else // Клик вне интерактивных областей
                    {
                        xc = -1;
                        yc = -1;
                    }
                    break;
                case SDL_WINDOWEVENT:
                    if (windowEvent.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                    {
                        board->reset_window_size(); // Перерасчёт размеров при изменении окна
                        break;
                    }
                }
                if (resp != Response::OK) // Если был получен ответ, выходим из цикла
                    break;
            }
        }
        return { resp, xc, yc }; // Возвращаем тип действия, координаты клетки
    }
    // Ожидает событие QUIT или REPLAY
    Response wait() const
    {
        SDL_Event windowEvent;
        Response resp = Response::OK;
        while (true)
        {
            if (SDL_PollEvent(&windowEvent))
            {
                switch (windowEvent.type)
                {
                case SDL_QUIT:
                    resp = Response::QUIT; // Выход из игры
                    break;
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                    board->reset_window_size(); // Адаптация окна
                    break;
                case SDL_MOUSEBUTTONDOWN: {
                    int x = windowEvent.motion.x;
                    int y = windowEvent.motion.y;
                    int xc = int(y / (board->H / 10) - 1);
                    int yc = int(x / (board->W / 10) - 1);
                    if (xc == -1 && yc == 8)
                        resp = Response::REPLAY; // Переиграть
                }
                                        break;
                }
                if (resp != Response::OK)
                    break;
            }
        }
        return resp;
    }

private:
    Board* board; // Указатель на объект доски
};
