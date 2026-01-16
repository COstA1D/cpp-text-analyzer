#include <iostream>
#include <fstream>// Для чтения файлов .json
#include <string>
#include <vector>
#include <map>// Для подсчета частоты слов (слово -> количество)
#include <set>// Для быстрого поиска стоп-слов
#include <sstream>// Для разбиения текста на слова
#include <random>// Для случайного выбора текста при генерации файлов
#include <algorithm>// Для сортировки слов по популярности
#include <chrono>// Для замера времени 
#include <windows.h>  // Для консоли UTF-8 русской кодировки в Windows
#include "json.hpp"// Библиотека для чтения/записи JSON файлов

using namespace std;
using json = nlohmann::json;// Короткое имя для JSON

// ЧИТАЕМ JSON ФАЙЛ (открываем, проверяем, загружаем данные)
bool readJsonFile(const string& filename, json& data) {// Открываем файл в бинарном режиме (важно для UTF-8)

    ifstream file(filename, ios::binary);
    if (!file.is_open()) {
        cout << u8"Не удалось открыть файл " << filename << endl;
        return false;
    }
    file.seekg(0, ios::end);// Проверяем размер файла (пустой файл = ошибка)
    if (file.tellg() == 0) {
        cout << u8"файл " << filename << u8" пуст, пропускаем" << endl;
        return false;
    }
    file.seekg(0, ios::beg);// Возвращаемся в начало файла
    file.clear();  // Сбрасываем возможные ошибки
    try {// Пробуем прочитать JSON
        file >> data;//Сразу загружаем JSON в переменную data
    }
    catch (const json::parse_error& e) {
        cout << u8"Ошибка парсинга JSON файла  " << filename << ": " << e.what() << endl;
        return false;
    }
    return true;
}
// ПРОВЕРЯЕМ, ЧТО JSON ФАЙЛ ПРАВИЛЬНЫЙ (есть "text" и опционально "stopwords")
bool validateTextJson(const json& data) {
    if (!data.is_object() || !data.contains("text") || !data["text"].is_string()) {// Должны быть: объект {} + поле "text" + строка
        cout << u8"Ошибка: Ожидается JSON вида {\"text\": \"line\", \"stopwords\": [\"...\"]}" << endl;
        return false;
    }
    if (data.contains("stopwords")) {// Если есть stopwords - все должны быть строками
        for (const auto& v : data["stopwords"])
            if (!v.is_string()) return false;
    }
    return true;
}
// ГЕНЕРИРУЕМ ТЕСТОВЫЕ JSON ФАЙЛЫ (text_0.json, text_1.json и т.д.)
void generateJsonFiles(int fileCount) {
    vector<string> samples = {// 4 примера русских текстов для тестов
        u8"В начале было Слово и Слово было у Бога.",
        u8"Текст для частотного анализа текста.",
        u8"Солнце светит ярко, птицы поют весело.",
        u8"Зима холодная, снег белый и пушистый."
    };// Случайный выбор текста для каждого файла
    mt19937 gen(static_cast<unsigned>(time(nullptr)));// Генератор случайных чисел
    uniform_int_distribution<> textId(0, samples.size() - 1);
    for (int i = 0; i < fileCount; ++i) {
        json obj;// Создаем новый JSON объект
        obj["text"] = samples[textId(gen)];// Случайный текст
        obj["stopwords"] = json::array({ u8"и", u8"в", u8"у" });// Стоп-слова
        string filename = "text_" + to_string(i) + ".json";
        ofstream out(filename);// Создаем файл
        if (out.is_open()) {
            out << obj.dump(2, ' ', false, json::error_handler_t::replace); // Красиво пишем JSON
            cout << u8"Создан файл" << filename << endl;
        }
    }
}
// РАЗБИРАЕМ ТЕКСТ НА СЛОВА (убираем стоп-слова типа "и", "в", "у")
void splitWords(const string& text, const vector<string>& stopwords, vector<string>& words_out) {
    words_out.clear();// Очищаем результат


    // stringstream как ножницы - режет текст по пробелам
    stringstream ss(text);
    string word;

    while (ss >> word) {// Берем слово за словом
        // Проверяем, не стоп-слово ли это
        bool is_stop = false;
        for (const string& stop : stopwords) {
            if (word == stop) {// Точное совпадение
                is_stop = true;
                break;
            }
        }
        if (!is_stop) {
            words_out.push_back(word);  //  ОРИГИНАЛЬНОЕ слово, добавляем в результат
        }
    }
}
//АНАЛИЗ ОДНОГО ФАЙЛА (подсчет слов + топ-5)
void analyzeText(const json& data, bool timecheck, vector<string>& words_buffer, map<string, int>& freq_buffer, int fileIndex) {
    if (!validateTextJson(data)) return;// Проверяем формат
    // Извлекаем текст и стоп-слова из JSON
    string text = data["text"].get<string>();
    vector<string> stopwords;
    if (data.contains("stopwords")) {
        for (const auto& v : data["stopwords"])
            stopwords.push_back(v.get<string>());
    }

    splitWords(text, stopwords, words_buffer);  // Разбиваем текст на слова (убираем стоп-слова)
    freq_buffer.clear();// Считаем частоту каждого слова
    for (const string& w : words_buffer) {
        ++freq_buffer[w];// map автоматически считает
    }

    if (!timecheck) {// Выводим результаты (только если НЕ бенчмарк)
        cout << u8"\n Файл #" << fileIndex << " (text_" << fileIndex << ".json):" << endl;
        cout << u8"Всего слов: " << words_buffer.size()
            << u8", уникальных: " << freq_buffer.size() << endl;
        cout << u8"Топ-5:" << endl;

        vector<pair<string, int>> top(freq_buffer.begin(), freq_buffer.end());// Копируем в вектор и сортируем по убыванию частоты
        sort(top.rbegin(), top.rend());
        for (int i = 0; i < min(5, (int)top.size()); ++i) {
            cout << u8"  " << (i + 1) << u8") '" << top[i].first << u8"' - " << top[i].second << u8" раз" << endl;
        }
    }
}
// ГЛАВНОЕ МЕНЮ ПРОГРАММЫ (точка входа)
int main() {
    // Настройка Windows для UTF-8 (русский текст в консоли)
#ifdef _WIN32
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD mode = 0;
        GetConsoleMode(hOut, &mode);
        SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN);
        SetConsoleCP(65001); // UTF - 8 для ввода
        SetConsoleOutputCP(65001);// UTF-8 для вывода
#endif

    cout << u8"================ АНАЛИЗАТОР ЧАСТОТЫ СЛОВ ================" << endl;
    vector<string> words_buffer;// Временное хранилище слов
    map<string, int> freq_buffer;// Подсчет частоты слов
    string choose;// Выбор пользователя
    
    while (true) {
        cout << u8"\n1)Анализ файлов\n2) Отладка\n3) Выход\n> ";
        cin >> choose;
        if (choose == "1") {
            int amount; cout << u8"Количество файлов: "; cin >> amount;// Анализ существующих файлов
            for (int i = 0; i < amount; ++i) {
                json data;// JSON данные текущего файла
                if (readJsonFile("text_" + to_string(i) + ".json", data))
                    analyzeText(data, false, words_buffer, freq_buffer,i);
            }
        }
        else if (choose == "2") {
            cout << u8"1) Генерация файлов\n2) Бенчмарк\n> ";// Отладочные функции
            cin >> choose;
            if (choose == "1") {
                int amount; cout << u8"Количество файлов: "; cin >> amount;
                generateJsonFiles(amount);// Создаем тестовые файлы
            }
            else if (choose == "2") {// Замеряем скорость на N файлах
                int amount; cout << u8"Количество файлов: "; cin >> amount;
                auto total_start = chrono::high_resolution_clock::now();
                for (int i = 0; i < amount; ++i) {
                    json data;
                    if (readJsonFile("text_" + to_string(i) + ".json", data))
                        analyzeText(data, true, words_buffer, freq_buffer,i);
                }
                auto total_dur = chrono::duration_cast<chrono::milliseconds>(
                    chrono::high_resolution_clock::now() - total_start).count();
                cout << u8"Общее время (" << amount << u8" файлов): " << total_dur << u8"мс " << endl;
            }
        }
        else if (choose == "3") {
            break;
        }
    }
    return 0;
}
