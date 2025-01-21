#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <queue>
#include <vector>
#include <unordered_map>
#include <string>
#include <bitset>
#include <map>
using namespace std;

struct Node
{
    char symbol;
    unsigned long long frequency;
    Node *left;
    Node *right;
}; // Узел дерева

struct Compare
{
    bool operator ()(Node* l, Node* r)
    {
        return ((r -> frequency) < (l -> frequency));
    }
}; // Используем priority_queue для создания дерева, потому для него задаём свой собственный Compare

Node* CreateNode(char sym = ' ', unsigned long long fr = 0, Node* l = NULL, Node* r = NULL)
{
    Node *node = new Node();
    node -> symbol = sym;
    node ->frequency = fr;
    node -> left = l;
    node -> right = r;
    return node;
} // Создание очередного узла

Node* createTree(priority_queue<Node*, vector<Node*>, Compare> nodeList)
{
    // Заполняем дерево, пока не выйдем в его корень
    Node* root = CreateNode();
    if (nodeList.size() == 0) return root;
    if (nodeList.size() == 1)
    {
        Node* theOnly = nodeList.top();
        root = CreateNode(' ', theOnly->frequency, theOnly, NULL);
        return root;
    }
    while (nodeList.size() != 1)
    {
        Node* l = nodeList.top(); nodeList.pop();
        Node* r = nodeList.top(); nodeList.pop();
        nodeList.push(CreateNode(' ', ((l->frequency) + (r->frequency)), l, r));
    }
    root = nodeList.top();
    return root;
}

void HuffmanTab(Node* root, unordered_map<char, string> & hCodes, string code)
{
    if (root == NULL) return;
    if (root->left == NULL && root->right == NULL)
    {
        if (code == "")
        {
           hCodes[root->symbol] = "0";
           return; 
        }
        hCodes[root->symbol] = code;
    }
    HuffmanTab(root->left, hCodes, code + "0");
    HuffmanTab(root->right, hCodes, code + "1");
}

void writeBin(ofstream & fout, int num) // Для записи частот в файл
{
    unsigned char byte = 0, marker = 1; int lim = 0; // unsigned char — тип данных, под переменную которого выделяется 8 бит = 1 байт
    bitset<32> bnum(num); string bstr = bnum.to_string(); int len = bstr.length();
    for (int i = 0; i < len; i++)
    {
        if (bstr[i] == '1') byte = byte | marker;
        lim++;
        if (lim == 8)
        {
            lim = 0;
            fout.put(byte);
            byte = 0;
        }
        byte = byte << 1;
    }
}

int readBin(ifstream & fin) // Для чтения частот из файла
{
    string bstr = "";
    for (int i = 0; i < 4; i++) // записываем 4 куска по 8 бит, всего 32 бит
    {
        unsigned char bit_raw = fin.get();
        bitset<8> bits(bit_raw);
        bstr += bits.to_string();
    }
    bitset<32> result(bstr);
    return result.to_ullong();
}

// Запись закодированных данных в файл
void encodedInBin(const string & encoded, ofstream & fout, map<char, int> hTab)
{
    unsigned char byte = 0, marker = 1; int lim = 0;
    ofstream bof("buffer.txt", ios::binary);
    for (size_t i = 0; i < encoded.size(); i++)
    {
        if (encoded[i] == '1')
            byte = byte | marker;   
        lim++;
        if (lim == 8)
        {
            bof.put(byte);
            byte = 0;
            lim = 0;
        }
        byte = byte << 1;
    }
    if (lim > 0) // Запись обрубков в файл
    {
        byte = byte << (8 - lim);
        bof.put(byte);
    }
    bof.close();
    ifstream bif("buffer.txt", ios::binary);
    fout << lim;
    for (auto elem: hTab) // Запись частот в файл
    {
        if (elem.first == '\n')
        {
            fout << '|';
            writeBin(fout, elem.second);
        }
        else
        {
            fout << elem.first;
            writeBin(fout, elem.second);
        }
    }
    fout << "\n";
    char current;
    while (bif.get(current)) fout << current;
    bif.close();
    remove("buffer.txt");
}

// Декодирование по таблице Хаффмана
string hCodesDC(const string & source, const unordered_map<char, string> & hCodes)
{
    string result = "", buffer = "";
    size_t hcSize = hCodes.size(), sSize = source.size();
    if (hcSize == 1)
    {
        for (auto p: hCodes) result += p.first;
        return result;
    }
    for (size_t i = 0; i < sSize; i++)
    {
        buffer += source[i];
        for (auto p: hCodes)
        {
            if (p.second == buffer)
            {
                if (p.first == '|')
                {
                    result += '\n';
                    buffer = ""; 
                    break;
                }
                result += p.first;
                buffer = "";
                break;
            }
        }
    }
    return result;
}


string ReadFreqs(ifstream & file, map<char, int> & frequencies)
{
    string result = ""; char current;
    unsigned char byte, marker = 1;
    size_t remainings = file.get() - '0';
    while (file.get(current))
    {
        if (current == '|') frequencies['\n'] = readBin(file);
        else if (current == '\n') break;
        else frequencies[current] = readBin(file);
    }
    while (file.get((char &)byte))
    {
        for (int i = 7; i >= 0; i--)
        {
            unsigned char buffer = (byte >> i);
            if (buffer & marker) result += '1';
            else result += '0';
        }
    }
    result = result.substr(0, result.size() - (8 - remainings));
    return result;
}

void hEncode(ifstream& in, ofstream& out) // функция кодировки, содержит в себе формирование частот,
                                          // создание дерева и использование функций для кодирования
{
    string source, result; map<char, int> frequencies;
    priority_queue<Node*, vector<Node*>, Compare> nodeInfo;
    unordered_map<char, string> huffmansTab;
    char symbol;
    while (in.get(symbol)) source += symbol;
    size_t sSize = source.size();
    for (size_t i = 0; i < source.size(); i++) frequencies[source[i]]++;
    for (auto p: frequencies) nodeInfo.push(CreateNode(p.first, p.second, NULL, NULL));
    Node *tree = createTree(nodeInfo);
    HuffmanTab(tree, huffmansTab, "");
    for (size_t i = 0; i < sSize; i++) result += huffmansTab[source[i]];
    encodedInBin(result, out, frequencies);
    in.close();
    out.close();
}


void hDecode(ifstream& in, ofstream& out) // функция декодировки
{
    string source = "";
    string result = "";
    unordered_map<char, string> huffmansTab;
    map<char, int> frequencies;
    priority_queue<Node*, vector<Node*>, Compare> nodeInfo;     
    source = ReadFreqs(in, frequencies);
    for (auto p: frequencies) nodeInfo.push(CreateNode(p.first, p.second, NULL, NULL));
    Node* tree = createTree(nodeInfo);
    HuffmanTab(tree, huffmansTab, "");
    result = hCodesDC(source, huffmansTab);
    out << result;
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
        ifstream input("text.txt", ios::binary);
        ofstream output("resultE.bin", ios::binary);
        if (!input.is_open() || !output.is_open())
        {
            cout << "Error: can't open file." << endl;
            exit(1);
        }
        hEncode(input, output);
        cout << "Encoding done. Result is saved in resultE.bin." << endl;
    }
    else
    {
        ifstream input("resultE.bin", ios::binary);
        ofstream output("resultD.txt", ios::binary);
        if (!input.is_open() || !output.is_open())
        {
            cout << "Error: can't open file." << endl;
            exit(1);
        }
        hDecode(input, output);
        cout << "Decoding done. Result is saved in resultD.txt." << endl;
    }
    return 0;
}
