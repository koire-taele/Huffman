#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <clocale>
#include <Windows.h>
using namespace std;

// реализовано по алгоритму из предложенной преподавателем книги

bool tabSort(const pair<vector<unsigned char>, int> & freqA, const pair<vector<unsigned char>, int> & freqB)
{
    return freqA.second > freqB.second;
}

bool freqSort(const pair<vector<int>, int> & freqA, const pair<vector<int>, int> & freqB)
{
    return freqA.second > freqB.second;
}

vector<unsigned char> UTF8_Handler(ifstream & in, char & symbol)
{
    unsigned char handler; vector<unsigned char> charSet; // основная идея: хранить байты UTF-8 в наборе char-объектов
    handler = symbol;
    charSet.push_back(handler);
    if (handler < 128) return charSet; // эквивалентно проверке на соответствие шаблону 0xxxxxxx для UTF-8
    if (handler >= 192 || handler < 224) // эквивалентно проверке на соответствие шаблону 110xxxxx для UTF-8
    {
        in.get(symbol);
        handler = symbol;
        charSet.push_back(handler);
        return charSet;
    }
    if (handler >= 224 || handler < 240) // эквивалентно проверке на соответствие шаблону 1110xxxx для UTF-8
    {
        for (int i = 0; i < 2; i++)
        {
            in.get(symbol);
            handler = symbol;
            charSet.push_back(handler);
        }
        return charSet;
    }
    for (int i = 0; i < 3; i++) // если не прошла ни одна из проверок выше, то байт соответствует шаблону 11110xxx для UTF-8
    {
        in.get(symbol);
        handler = symbol;
        charSet.push_back(handler);
    }
    return charSet;
} // потом будем использовать эту функцию, чтобы аккуратно переносить UTF-8-символы в таблицу Хаффмана при кодировании и обратно в итоговый файл при декодировании

void encoder(ifstream & in, ofstream & out)
{
    vector<pair<vector<unsigned char>, int>> frequencies; // таблица частот: ключ - символ, значение - частота символа
    bool isThere = false;
    if (in.is_open())
    {
        char symbol, symbolToUpdate; int valueToUpdate; vector<unsigned char> utf8char;
        in.get(symbol);
        utf8char = UTF8_Handler(in, symbol);
        frequencies.push_back(pair<vector<unsigned char>, int>(utf8char, 1));
        while (in.get(symbol))
        {
            utf8char = UTF8_Handler(in, symbol);
            isThere = false;
            for (int i = 0; i < frequencies.size(); i++)
            {
                if (utf8char == frequencies[i].first)
                {
                    frequencies[i].second++;
                    isThere = true;
                    break;
                }
            }
            if (!isThere)
            {
                frequencies.push_back(pair<vector<unsigned char>, int>(utf8char, 1));
            }
        } // получаем таблицу частот
        if (frequencies.empty())
        {
            cout << "Error: frequencies tab is empty. The file itself is empty or there's a bug in the code." << endl;
            exit(1);
        }
        sort(frequencies.begin(), frequencies.end(), tabSort); // сортируем по частотам в порядке убывания
        int size = frequencies.size();
        vector<vector<int>> indexes;
        // хранение индексов в таком формате позволяет рассматривать каждый вектор из набора векторов
        // как набор индексов, для соответствующих символов которых при сложении частот надо обновить коды
        for (int i = 0; i < size; i++)
        {
            vector<int> another_i = {i};
            indexes.push_back(another_i);
        }
        vector<string> codes; // хранит в себе постепенно формируемые коды
        for (int i = 0; i < size; i++)
        {
            codes.push_back("");
        }

        // по алгоритму, предложенному в книге, складываются вероятности два последних элементов в таблице,
        // т.к. таблица отсортирована, то эти два элемента - два последних элемента в таблице

        vector<pair<vector<int>, int>> freqIndexes; // для сопоставления сумм частот и индексов символов, чьи частоты складывались
        for (int i = 0; i < size; i++)
        {
            vector<int> another_i = {i};
            freqIndexes.push_back(pair<vector<int>, int>(another_i, frequencies[i].second));
        }

        int currFreqISize = freqIndexes.size();
        while (currFreqISize > 2)
        {
            vector<int> preLastIndexes = freqIndexes[currFreqISize-2].first;
            vector<int> LastIndexes = freqIndexes[currFreqISize-1].first;
            int preLastFreqs = freqIndexes[currFreqISize-2].second;
            int LastFreqs = freqIndexes[currFreqISize-1].second; // берём последнюю и предпоследнюю строки в таблице
            
            // приписываем 0 и 1 тем символам, которые учавствовали в данной сумме (индексы здеь отображают соответствующие им символы)
            for (int stringI = 0; stringI < preLastIndexes.size(); stringI++)
            {
                codes[preLastIndexes[stringI]] = "0" + codes[preLastIndexes[stringI]];
            }
            for (int stringI = 0; stringI < LastIndexes.size(); stringI++)
            {
                codes[LastIndexes[stringI]] = "1" + codes[LastIndexes[stringI]];
            }

            int newFreq = preLastFreqs + LastFreqs;
            vector<int> newIndexes = preLastIndexes;
            for (int i = 0; i < LastIndexes.size(); i++) newIndexes.push_back(LastIndexes[i]);
            pair<vector<int>, int> newMember = {newIndexes, newFreq};
            freqIndexes.erase(freqIndexes.begin() + (freqIndexes.size()-1));
            freqIndexes.erase(freqIndexes.begin() + (freqIndexes.size()-1)); // стираем два последних элемента таблицы
            freqIndexes.push_back(newMember); // и добавляем новый
            sort(freqIndexes.begin(), freqIndexes.end(), freqSort); // таблица всегда должна быть отсортирована
            currFreqISize = freqIndexes.size();
        }
        if (size >= 1) // последняя итерация обновления кодов для символов, одновременно проверка на случай, если текст имел только один символ или два различных
        {
            for (int stringI = 0; stringI < freqIndexes[0].first.size(); stringI++)
            {
                codes[freqIndexes[0].first[stringI]] = "0" + codes[freqIndexes[0].first[stringI]];
            }
        }
        if (size >= 2)
        {
            for (int stringI = 0; stringI < freqIndexes[1].first.size(); stringI++)
            {
                codes[freqIndexes[1].first[stringI]] = "1" + codes[freqIndexes[1].first[stringI]];
            }
        }

        vector<pair<vector<unsigned char>, string>> huffmanTab; // составление таблицы Хаффмана
        for (int i = 0; i < size; i++) huffmanTab.push_back(pair<vector<unsigned char>, string>(frequencies[i].first, codes[i]));
        in.close();
        in.open("text.txt");
        int index;
        int textLength = 0;
        string result = ""; // составление закодированного текста
        while (in.get(symbol))
        {
            utf8char = UTF8_Handler(in, symbol);
            index = 0;
            while (utf8char != huffmanTab[index].first) index++;
            result += huffmanTab[index].second;
            textLength++;
        }
        
        out << textLength << ' ' << result.length() << ' '; // запись длины текста оригинального и зашифрованного, чтобы потом правильно декодировать
        
        // запись таблицы Хаффмана в файл; в первой строке пишем кол-во строк в таблице, чтобы потом верно считать её из файла с закодированным текстом
        out << size << endl;
        for (int i = 0; i < size; i++)
        {
            for (int j = 0; j < huffmanTab[i].first.size(); j++) out.put(huffmanTab[i].first[j]);
            out << huffmanTab[i].second << endl;
        }

        // далее запись самого закодированного текста
        int encodedLength = result.length();
        char byte = 0;
        int byteDiv = 0;
        char marker = 1;
        for (int i = 0; i < encodedLength; i++) // С++ не даёт записать один бит в файл, он даёт писать туда минимум 1 байт, поэтому приходится изворачиваться
        {
            byte = byte << 1;
            if (result[i] == '1')
            {
                byte |= marker;
            }
            byteDiv++;
            if (byteDiv == 8)
            {
                out.put(byte);
                byte = 0;
                byteDiv = 0;
            }
        }
        if (byteDiv != 0)
        {
            byte = byte << (8 - byteDiv);
            out.put(byte);
        }
        in.close();
        out.close();
    }
};

void decoder(ifstream & in, ofstream & out)
{
    // Восстанавливаем данные о длине оригинального и закодированного сообщений, а также таблицу Хаффмана 
    string data;
    string toDecode = "";
    string code;
    string strTextLength;
    string strEncodedLength;
    string strHuffSize;
    getline(in, data);
    bool afterSpaceFirst = false; bool afterSpaceSecond = false;
    for (int i = 0; i < data.length(); i++)
    {
        if (data[i] == ' ' && !afterSpaceFirst)
        {
            afterSpaceFirst = true;
            continue;
        }
        if (data[i] == ' ' && !afterSpaceSecond)
        {
            afterSpaceSecond = true;
            continue;
        }
        if (!afterSpaceFirst)
        {
            strTextLength += data[i];
        }
        else if (!afterSpaceSecond)
        {
            strEncodedLength += data[i];
        }
        else
        {
            strHuffSize += data[i];
        }
    }
    int textLength = stoi(strTextLength);
    int encodedLength = stoi(strEncodedLength);
    int size = stoi(strHuffSize);
    char symbol;
    vector<unsigned char> utf8char;
    vector<pair<vector<unsigned char>, string>> huffmanTab;
    for (int i = 0; i < size; i++)
    {
        in.get(symbol);
        utf8char = UTF8_Handler(in, symbol);
        getline(in, code);
        huffmanTab.push_back(pair<vector<unsigned char>, string>(utf8char, code));
    }

    unsigned char marker; unsigned char u_symbol; unsigned char byte;
    while(in.get(symbol))
    {
        u_symbol = symbol;
        marker = 128;
        for (int i = 0; i < 8; i++)
        {
            if (u_symbol & marker) toDecode += '1';
            else toDecode += '0';
            marker = marker >> 1;
        }
    }

    string another_code;
    for (int i = 0; i < encodedLength; i++)
    {
        another_code += toDecode[i];
        for (int j = 0; j < size; j++)
        {
            if (another_code == huffmanTab[j].second)
            {
                for (int k = 0; k < huffmanTab[j].first.size(); k++) out.put(huffmanTab[j].first[k]);
                another_code = "";
                break;
            }
        }
    }
    in.close();
    out.close();
}

bool mode()
{
    char input;
    while (true)
    {
        cout << "Choose working mode: encoder or decoder [e/d]: ";
        cin >> input;
        if (input == 'e') return false;
        if (input == 'd') return true;
        else cout << "Incorrect input. Please try again." << endl;
    }
}

int main()
{
    bool codeOrDecode = mode();
    if (!codeOrDecode)
    {
        ifstream einput("text.txt", ios::in);
        ofstream eoutput("resultE.bin", ios::out | ios::binary);
        if (!einput.is_open() || !eoutput.is_open())
        {
            cout << "Error: can't open file." << endl;
            exit(1);
        }
        encoder(einput, eoutput);
        cout << "Encoding done. Result is saved in resultE.bin." << endl;
    }
    else
    {
        ifstream dinput("resultE.bin", ios::binary);
        ofstream doutput("resultD.txt", ios::binary);
        if (!dinput.is_open() || !doutput.is_open()) {
            cout << "Error: can't open file." << endl;
            exit(1);
        }
        decoder(dinput, doutput);
        cout << "Decoding done. Result is saved in resultD.txt." << endl;        
    }
}