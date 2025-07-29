#pragma once
#include <fstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "../Models/Project_path.h"

class Config
{
public:
    Config()
    {
        reload();
    }
    // Загружает настройки из файла settings.json
    void reload()
    {
        // Открытие JSON-файла с настройками
        std::ifstream fin(project_path + "settings.json");
        // Считывание содержимого в объект json
        fin >> config;
        // Закрытие файла
        fin.close();
    }
    // Перегрузка оператора круглых скобок, позволяет обращаться к объекты, как к функции, например bool isWhiteBot = config("Bot", "IsWhiteBot")
    auto operator()(const string& setting_dir, const string& setting_name) const
    {
        return config[setting_dir][setting_name];
    }

private:
    json config;
};
