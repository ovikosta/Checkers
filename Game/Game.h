#pragma once
#include <chrono>
#include <thread>

#include "../Models/Project_path.h"
#include "Board.h"
#include "Config.h"
#include "Hand.h"
#include "Logic.h"

class Game
{
public:
    Game() : board(config("WindowSize", "Width"), config("WindowSize", "Hight")), hand(&board), logic(&board, &config)
    {
        ofstream fout(project_path + "log.txt", ios_base::trunc);
        fout.close();
    }

    // to start checkers
    int play()
    {
        auto start = chrono::steady_clock::now(); // Засекаем время начала игры
        if (is_replay)
        {
            logic = Logic(&board, &config); // Пересоздаем игровую логику при повоторном запуске
            config.reload(); // Перезагружаем настройки из файла
            board.redraw(); // Перерисовываем доску
        }
        else
        {
            board.start_draw(); // Инициализируем стартовое отображение доски
        }
        is_replay = false; // Сбрасываем флаг повтора

        int turn_num = -1;
        bool is_quit = false;
        const int Max_turns = config("Game", "MaxNumTurns"); // Получаем максимальное количество ходов из конфигурации.
        while (++turn_num < Max_turns)
        {
            beat_series = 0;
            logic.find_turns(turn_num % 2); // Находим все доступные ходы для текущего игрока
            if (logic.turns.empty())
                break; // Прерываем игру, если ходов нет
            logic.Max_depth = config("Bot", string((turn_num % 2) ? "Black" : "White") + string("BotLevel")); // Устанавливаем глубину принятия решений для бота
            if (!config("Bot", string("Is") + string((turn_num % 2) ? "Black" : "White") + string("Bot"))) // Проверяем, ходит ли человек
            {
                auto resp = player_turn(turn_num % 2); // Выполняем ход игрока
                if (resp == Response::QUIT) // Игрок вышел из игры
                {
                    is_quit = true;
                    break;
                }
                else if (resp == Response::REPLAY) // Игрок хочет переиграть
                {
                    is_replay = true;
                    break;
                }
                else if (resp == Response::BACK) // Возврат хода
                {
                    if (config("Bot", string("Is") + string((1 - turn_num % 2) ? "Black" : "White") + string("Bot")) &&
                        !beat_series && board.history_mtx.size() > 2) // Если бот был предыдущим игроком и не было взятий
                    {
                        board.rollback(); // Откатываем ход бота
                        --turn_num;
                    }
                    if (!beat_series)
                        --turn_num;

                    board.rollback(); // Откатываем ход игрока
                    --turn_num;
                    beat_series = 0;
                }
            }
            else
                bot_turn(turn_num % 2); // Ход делает бот
        }
        auto end = chrono::steady_clock::now();
        ofstream fout(project_path + "log.txt", ios_base::app);
        fout << "Game time: " << (int)chrono::duration<double, milli>(end - start).count() << " millisec\n";  // Записываем общее время игры
        fout.close();

        if (is_replay) // Повтор запуска, если был REPLAY
            return play();
        if (is_quit)
            return 0;
        int res = 2; // 0 - ничья, 1 - победа черных, 2 - победа белых (по умолчанию)
        if (turn_num == Max_turns)
        {
            res = 0; // Если достигнут лимит ходов - ничья
        }
        else if (turn_num % 2)
        {
            res = 1; // Последним ходили черные — они выиграли
        }
        board.show_final(res); // Показываем итоговое состояние и результат игры
        auto resp = hand.wait(); // Ожидаем решение игрока о продолжении игры или завершении
        if (resp == Response::REPLAY)
        {
            is_replay = true;
            return play();
        }
        return res; // Возвращаем результат игры
    }

private:
    void bot_turn(const bool color)
    {
        auto start = chrono::steady_clock::now(); // Фиксируем время начала хода бота

        auto delay_ms = config("Bot", "BotDelayMS"); // Задержка хода
        // new thread for equal delay for each turn
        thread th(SDL_Delay, delay_ms); // Запуск потока
        auto turns = logic.find_best_turns(color);  // Получаем ход в соответствии с логикой
        th.join();
        bool is_first = true;
        // making moves
        for (auto turn : turns)
        {
            if (!is_first)
            {
                SDL_Delay(delay_ms); // Добавляем задержку между серией ходов
            }
            is_first = false;
            beat_series += (turn.xb != -1); // Если есть убитая фигура увеличиваем счетчик
            board.move_piece(turn, beat_series); // Двигаем фигуру по доске
        }

        auto end = chrono::steady_clock::now(); // Фиксируем время окончания хода
        ofstream fout(project_path + "log.txt", ios_base::app); // Записываем сколько времени занял ход
        fout << "Bot turn time: " << (int)chrono::duration<double, milli>(end - start).count() << " millisec\n";
        fout.close();
    }

    Response player_turn(const bool color)
    {
        // return 1 if quit
        vector<pair<POS_T, POS_T>> cells;
        for (auto turn : logic.turns)
        {
            cells.emplace_back(turn.x, turn.y);
        }
        board.highlight_cells(cells); // Подсвечиваем фигуры, которыми можно ходить
        move_pos pos = { -1, -1, -1, -1 };
        POS_T x = -1, y = -1;
        // trying to make first move
        while (true)
        {
            auto resp = hand.get_cell(); // Ждём нажатия на клетку игроком
            if (get<0>(resp) != Response::CELL) // Если игрок нажал кнопку "Назад", "Выход" или "Повтор"
                return get<0>(resp); // Прерываем выполнение и возвращаем соответствующий Response
            pair<POS_T, POS_T> cell{ get<1>(resp), get<2>(resp) }; // Получаем координаты выбранной клетки

            bool is_correct = false;
            for (auto turn : logic.turns)
            {
                if (turn.x == cell.first && turn.y == cell.second)
                {
                    is_correct = true;
                    break;
                }
                if (turn == move_pos{ x, y, cell.first, cell.second })
                {
                    pos = turn;
                    break;
                }
            }
            if (pos.x != -1)
                break; // Игрок выбрал корректную пару, откуда и куда пойдет фигура
            if (!is_correct)
            {
                if (x != -1)
                {
                    board.clear_active();
                    board.clear_highlight();
                    board.highlight_cells(cells);
                }
                x = -1;
                y = -1;
                continue;
            }
            x = cell.first;
            y = cell.second;
            board.clear_highlight();
            board.set_active(x, y);
            vector<pair<POS_T, POS_T>> cells2;
            for (auto turn : logic.turns)
            {
                if (turn.x == x && turn.y == y)
                {
                    cells2.emplace_back(turn.x2, turn.y2);
                }
            }
            board.highlight_cells(cells2);
        }
        board.clear_highlight();
        board.clear_active();
        board.move_piece(pos, pos.xb != -1); // Делаем ход, проверяя, было ли взятие
        if (pos.xb == -1)
            return Response::OK; // Если не было боя — ход завершен
        // continue beating while can
        beat_series = 1;
        while (true)
        {
            logic.find_turns(pos.x2, pos.y2);
            if (!logic.have_beats) // Если больше нет взятий — завершаем цепочку боёв
                break;

            vector<pair<POS_T, POS_T>> cells;
            for (auto turn : logic.turns)
            {
                cells.emplace_back(turn.x2, turn.y2);
            }
            board.highlight_cells(cells);
            board.set_active(pos.x2, pos.y2);
            // trying to make move
            while (true)
            {
                auto resp = hand.get_cell(); // Снова ожидаем клик игрока на цель для следующего боя
                if (get<0>(resp) != Response::CELL)
                    return get<0>(resp); // Игрок может нажать "назад", "выход" и т.д.
                pair<POS_T, POS_T> cell{ get<1>(resp), get<2>(resp) };

                bool is_correct = false;
                for (auto turn : logic.turns)
                {
                    if (turn.x2 == cell.first && turn.y2 == cell.second)
                    {
                        is_correct = true;
                        pos = turn;
                        break;
                    }
                }
                if (!is_correct)
                    continue;

                board.clear_highlight();
                board.clear_active();
                beat_series += 1;
                board.move_piece(pos, beat_series);
                break;
            }
        }

        return Response::OK;
    }

private:
    Config config;
    Board board;
    Hand hand;
    Logic logic;
    int beat_series;
    bool is_replay = false;
};
