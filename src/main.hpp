#include <iostream>
#include <fstream>// Работа с файлами (ifstream, ofstream)
#include <string>
#include <vector>
#include <map>// Словарь частот map<string, int>
#include <set> // Множество стоп-слов set<string>
#include <random>// Генератор случайных чисел mt19937
#include <algorithm>// sort(), min()
#include <chrono>// Замеры времени high_resolution_clock
#include <windows.h> // UTF-8 консоль Windows (SetConsoleCP)

#include "json.hpp"// Библиотека парсинга/генерации JSON

using namespace std;
using json = nlohmann::json;// Псевдоним для удобства

// Читает JSON-файл в объект data
bool readJsonFile(const string& filename, json& data) {
    ifstream file(filename, ios::binary);// Открываем файл в бинарном режиме (для UTF-8)
    file.seekg(0, ios::end);               // Переходим в конец файла
    if (file.tellg() == 0) {               // Проверяем, пустой ли файл
        cout << "file " << filename << " empty, skipping" << endl;
        return false;
    }
    file.seekg(0, ios::beg);               // Возвращаемся в начало файла
    file.clear();                          // Сбрасываем флаги ошибок ifstream
    try {
        file >> data;                      // Парсим JSON из файла
    }
    catch (const json::parse_error& e) {   // Ловим ошибки парсинга JSON
        cout << "Error parsing JSON file " << filename << ": " << e.what() << endl;
        return false;
    }
    return true;                           // Успешно прочитали
}

// Проверяет структуру JSON: {"text": "...", "stopwords": [...]}
bool validateTextJson(const json& data) {
    if (!data.is_object() || !data.contains("text") || !data["text"].is_string()) {
        // Корень должен быть объектом с обязательным строковым полем "text"
        cout << "Error: Expected JSON in file {\"text\": \"line\", \"stopwords\": [\"...\"]}" << endl;
        return false;
    }
    if (data.contains("stopwords")) {          // Опциональное поле stopwords
        for (const auto& v : data["stopwords"]) // Проверяем все элементы массива
            if (!v.is_string()) return false;  // Должны быть строки
    }
    return true;
}

// Генерирует fileCount JSON-файлов с случайным русским текстом
void generateJsonFiles(int fileCount) {
    vector<string> samples = {                 // 4 примера русских текстов
        u8"В начале было Слово и Слово было у Бога.",
        u8"Текст для частотного анализа текста.",
        u8"Солнце светит ярко, птицы поют весело.",
        u8"Зима холодная, снег белый и пушистый."
    };
    mt19937 gen(static_cast<unsigned>(time(nullptr)));  // Генератор случайных чисел
    uniform_int_distribution<> textId(0, samples.size() - 1); // 0-3 равномерно

    for (int i = 0; i < fileCount; ++i) {
        json obj;                              // Создаём JSON-объект
        obj["text"] = samples[textId(gen)];    // Случайный текст
        obj["stopwords"] = json::array({ u8"и", u8"в", u8"у" }); // Стоп-слова
        string filename = "text_" + to_string(i) + ".json"; // text_0.json, text_1.json...
        ofstream out(filename);                // Открываем для записи
        if (out.is_open()) {
            out << obj.dump(2, ' ', false, json::error_handler_t::replace); // Сохраняем с отступами
            cout << "Created " << filename << endl; // Подтверждение создания
        }
    }
}

// Разбивает текст на слова, фильтруя стоп-слова
void splitWords(const string& text, const vector<string>& stopwords, vector<string>& words_out) {
    set<string> stop(stopwords.begin(), stopwords.end()); // Быстрый поиск стоп-слов
    words_out.clear();                                     // Очищаем выходной буфер
    string cur;                                            // Текущая накопленная лексема

    for (char c : text) {                                  // По символам текста
        unsigned char uc = static_cast<unsigned char>(c);  // Беззнаковый байт
        if ((uc >= 0xC0 && uc <= 0xFF) || isalnum(uc)) {  // Кириллица UTF-8 или латинница/цифры
            cur += tolower(c);                             // Добавляем в нижнем регистре
        }
        else if (!cur.empty()) {                           // Конец слова (пробел/знак препинания)
            if (stop.find(cur) == stop.end()) {            // Не стоп-слово?
                words_out.push_back(cur);                  // Добавляем в результат
            }
            cur.clear();                                   // Сбрасываем буфер слова
        }
    }
    if (!cur.empty() && stop.find(cur) == stop.end()) {    // Последнее слово
        words_out.push_back(cur);
    }
}

// Анализирует один JSON-файл: частоты слов + бенчмарк времени
void analyzeText(const json& data, bool timecheck, vector<string>& words_buffer, map<string, int>& freq_buffer) {
    if (!validateTextJson(data)) return;                   // Проверяем структуру

    string text = data["text"].get<string>();              // Извлекаем текст
    vector<string> stopwords;
    if (data.contains("stopwords")) {                      // Извлекаем стоп-слова (опционально)
        for (const auto& v : data["stopwords"])
            stopwords.push_back(v.get<string>());
    }

    splitWords(text, stopwords, words_buffer);

    freq_buffer.clear();                                   // Очищаем частоты
    for (const string& w : words_buffer) {                 // Считаем частоты слов
        ++freq_buffer[w];
    }

    if (!timecheck) {                                      // Подробный вывод (не бенчмарк)
        cout << "Total words: " << words_buffer.size()
            << ", unique: " << freq_buffer.size() << endl;
        cout << "Top-5:" << endl;

        vector<pair<string, int>> top(freq_buffer.begin(), freq_buffer.end()); // Копируем в вектор
        sort(top.rbegin(), top.rend());                    // Сортируем по убыванию частоты

        for (int i = 0; i < min(5, (int)top.size()); ++i) { // Первые 5 слов
            cout << "  Word #" << (i + 1) << ": " << top[i].second << " times" << endl;
        }
    }
}

int main() {
#ifdef _WIN32                                          // Только для Windows
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);     // Дескриптор консоли
    DWORD mode = 0;
    GetConsoleMode(hOut, &mode);                       // Текущий режим консоли
    SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN);
    // Включаем UTF-8 + цвета + нормальные переносы строк
    SetConsoleCP(65001);                               // UTF-8 для ввода
    SetConsoleOutputCP(65001);                         // UTF-8 для вывода
#endif

    cout << u8"================ TEXT FREQUENCY ANALYZER ================" << endl;
    vector<string> words_buffer;                       // Переиспользуемый буфер слов
    map<string, int> freq_buffer;                      // Переиспользуемый буфер частот
    string choose;                                     // Ввод пользователя

    while (true) {                                     // Главное меню
        cout << "\n1) Analyze files\n2) Debugging\n3) Exit\n> ";
        cin >> choose;

        if (choose == "1") {                           // Анализ файлов
            int amount; cout << "Number of files: "; cin >> amount;
            for (int i = 0; i < amount; ++i) {
                json data;                             // JSON для текущего файла
                if (readJsonFile("text_" + to_string(i) + ".json", data))
                    analyzeText(data, false, words_buffer, freq_buffer);
            }
        }
        else if (choose == "2") {                      // Режим отладки
            cout << "1) Generation\n2) Benchmark\n> ";
            cin >> choose;
            if (choose == "1") {
                int amount; cout << "Quantity: "; cin >> amount;
                generateJsonFiles(amount);             // Генерация файлов
            }
            else if (choose == "2") {
                int amount; cout << "Quantity: "; cin >> amount;
                auto total_start = chrono::high_resolution_clock::now(); // Старт замера
                for (int i = 0; i < amount; ++i) {
                    json data;
                    if (readJsonFile("text_" + to_string(i) + ".json", data))
                        analyzeText(data, true, words_buffer, freq_buffer); // Только время
                }
                auto total_dur = chrono::duration_cast<chrono::milliseconds>(
                    chrono::high_resolution_clock::now() - total_start).count();
                cout << "Total time (" << amount << " files): " << total_dur << " ms" << endl;
            }
        }
        else if (choose == "3") {                      // Выход
            break;
        }
    }
    return 0;
}
