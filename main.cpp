#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>

using namespace std;

struct Column {
    string name;
    bool indexed;
};

struct Table {
    string name;
    vector<Column> columns;
    vector<vector<string>> rows; 
};


unordered_map<string, Table> tables;

string trim(const string& str) {
    const string whitespace = " \t\n\r";
    size_t start = str.find_first_not_of(whitespace);
    if (start == string::npos) return "";
    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
}

void Create(const string& command) {}
void Insert(const string& command) {}
void Select(const string& command) {}

int main(){
    string command;
    cout << "Mini SQL Console. Type your commands (end with ;):\n";

    

    while (true) {
        cout << ">> ";
        getline(cin, command, ';');
        //command = "CREATE  1 ;";
        command = trim(command);
        //cout << command << '\n';

        if (command.empty()) continue;
        
        
        if (command.find("CREATE") == 0) {
            Create(command);
        } else if (command.find("INSERT") == 0) {
            Insert(command);
        } else if (command.find("SELECT") == 0) {
            Select(command);
        } else if (command == "Q") {
            break;
        } else {
            cout << "Error: Unknown command." << endl;
        }
    }
    return 0;
}