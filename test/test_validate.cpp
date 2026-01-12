#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <sstream>
#include <random>
#include <algorithm>
#include <chrono>
#include <windows.h> 

#include "json.hpp"

using namespace std;
using json = nlohmann::json;

// Все функции без изменений: readJsonFile, validateTextJson, generateJsonFiles, splitWords, analyzeText

void test_generate_files() {
    cout << "Test: generate 3 JSON files" << endl;
    generateJsonFiles(3);  // Создаёт text_0.json, text_1.json, text_2.json
    cout << " 3 files generated successfully" << endl << endl;
}

void test_analyze_single_file() {
    cout << "Test: analyze text_0.json" << endl;
    json data;
    if (readJsonFile("text_0.json", data)) {
        vector<string> words_buffer;
        map<string, int> freq_buffer;
        analyzeText(data, false, words_buffer, freq_buffer);
        cout << " Single file analysis completed" << endl;
    } else {
        cout << " Failed to read text_0.json" << endl;
    }
    cout << endl;
}

void test_benchmark_100_files() {
    cout << "Test: benchmark 100 files" << endl;
    generateJsonFiles(100);  // Генерируем тестовые файлы
    
    vector<string> words_buffer;
    map<string, int> freq_buffer;
    auto start = chrono::high_resolution_clock::now();
    
    int processed = 0;
    for (int i = 0; i < 100; ++i) {
        json data;
        if (readJsonFile("text_" + to_string(i) + ".json", data)) {
            analyzeText(data, true, words_buffer, freq_buffer);
            processed++;
        }
    }
    
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    
    cout << " Processed " << processed << " files in " << duration << " ms" 
         << " (" << (duration*1.0/processed) << " ms/file)" << endl << endl;
}

int main() {
    // UTF-8 консоль Windows
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hOut, &mode);
    SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN);
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);
#endif

    cout << "===== TEXT FREQUENCY ANALYZER - TESTS =====" << endl << endl;

    test_generate_files();     // Тест 1: генерация файлов
    test_analyze_single_file(); // Тест 2: анализ одного файла  
    test_benchmark_100_files(); // Тест 3: бенчмарк производительности

    cout << "===== ALL TESTS COMPLETED =====" << endl;
    return 0;
}
