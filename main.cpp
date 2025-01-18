#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <sstream>
#include <iomanip>

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

void Create(const string& command) {
    size_t tableNameStart = command.find(" ") + 1;
    size_t tableNameEnd = command.find("(", tableNameStart);
    string tableName = trim(command.substr(tableNameStart, tableNameEnd - tableNameStart));

    if (tables.count(tableName)) {
        cout << "Error: Table " << tableName << " already exists." << endl;
        return;
    }

    Table table;
    table.name = tableName;

    size_t columnsStart = tableNameEnd + 1;
    size_t columnsEnd = command.find(")", columnsStart);
    string columnsPart = command.substr(columnsStart, columnsEnd - columnsStart);

    stringstream ss(columnsPart);
    string columnDef;

    while (getline(ss, columnDef, ',')) {
        columnDef = trim(columnDef);
        Column column;
        size_t spacePos = columnDef.find(" ");
        if (spacePos != string::npos) {
            column.name = trim(columnDef.substr(0, spacePos));
            column.indexed = trim(columnDef.substr(spacePos + 1)) == "INDEXED";
        } else {
            column.name = columnDef;
            column.indexed = false;
        }
        table.columns.push_back(column);
    }

    tables[tableName] = table;
    cout << "Table " << tableName << " has been created." << endl;
}
void Insert(const string& command) {
    size_t tableNameStart = command.find(" ") + 1;
    size_t tableNameEnd = command.find("(", tableNameStart);
    string tableName = trim(command.substr(tableNameStart, tableNameEnd - tableNameStart));

    if (!tables.count(tableName)) {
        cout << "Error: Table " << tableName << " does not exist." << endl;
        return;
    }

    Table& table = tables[tableName];
    size_t valuesStart = tableNameEnd + 1;
    size_t valuesEnd = command.find(")", valuesStart);
    string valuesPart = command.substr(valuesStart, valuesEnd - valuesStart);

    stringstream ss(valuesPart);
    string value;
    vector<string> row;

    while (getline(ss, value, ',')) {
        value = trim(value);
        if (value.front() == '"' && value.back() == '"') {
            value = value.substr(1, value.size() - 2);
        }
        row.push_back(value);
    }

    if (row.size() != table.columns.size()) {
        cout << "Error: Column count mismatch for table " << tableName << "." << endl;
        return;
    }

    table.rows.push_back(row);
    cout << "1 row has been inserted into " << tableName << "." << endl;
}

void Select(const string& command) {
    size_t fromPos = command.find("FROM");
    if (fromPos == string::npos) {
        cout << "Error: Missing FROM clause." << endl;
        return;
    }

    size_t tableNameStart = fromPos + 4;
    size_t wherePos = command.find("WHERE", tableNameStart);
    size_t joinPos = command.find("JOIN", tableNameStart);

    string tableName = trim(command.substr(tableNameStart, (wherePos == string::npos ? (joinPos == string::npos ? command.size() : joinPos) : wherePos) - tableNameStart));

    if (!tables.count(tableName)) {
        cout << "Error: Table " << tableName << " does not exist." << endl;
        return;
    }

    Table& table = tables[tableName];
    vector<vector<string>> resultRows = table.rows;
    vector<string> resultColumns;

    for (const auto& col : table.columns) {
        resultColumns.push_back(col.name); 
    }

    // JOIN
    if (joinPos != string::npos) {
        size_t joinTableStart = joinPos + 4;
        size_t onPos = command.find("ON", joinTableStart);
        string joinTableName = trim(command.substr(joinTableStart, onPos - joinTableStart));

        if (!tables.count(joinTableName)) {
            cout << "Error: Table " << joinTableName << " does not exist." << endl;
            return;
        }

        Table& joinTable = tables[joinTableName];

        if (onPos != string::npos) {
            size_t onConditionStart = onPos + 2;
            string onCondition = trim(command.substr(onConditionStart));

            size_t eqPos = onCondition.find("=");
            if (eqPos == string::npos) {
                cout << "Error: Invalid ON condition." << endl;
                return;
            }

            string leftColumn = trim(onCondition.substr(0, eqPos));
            string rightColumn = trim(onCondition.substr(eqPos + 1));

            int leftIdx = -1, rightIdx = -1;
            for (size_t i = 0; i < table.columns.size(); ++i) {
                if (table.columns[i].name == leftColumn) {
                    leftIdx = i;
                    break;
                }
            }

            for (size_t i = 0; i < joinTable.columns.size(); ++i) {
                if (joinTable.columns[i].name == rightColumn) {
                    rightIdx = i;
                    break;
                }
            }

            if (leftIdx == -1 || rightIdx == -1) {
                cout << "Error: Column not found in ON condition." << endl;
                return;
            }

            vector<vector<string>> joinedRows;
            for (const auto& row1 : table.rows) {
                for (const auto& row2 : joinTable.rows) {
                    if (row1[leftIdx] == row2[rightIdx]) {
                        vector<string> combinedRow = row1;
                        combinedRow.insert(combinedRow.end(), row2.begin(), row2.end());
                        joinedRows.push_back(combinedRow);
                    }
                }
            }

            resultRows = joinedRows;
            for (const auto& col : joinTable.columns) {
                resultColumns.push_back(col.name + " (joined)");
            }
        }
    }

    // WHERE
    if (wherePos != string::npos) {
        size_t whereConditionStart = wherePos + 5;
        string whereCondition = trim(command.substr(whereConditionStart));

        size_t opPos = whereCondition.find(">");
        if (opPos == string::npos) {
            cout << "Error: Unsupported WHERE condition." << endl;
            return;
        }

        string columnName = trim(whereCondition.substr(0, opPos));
        string value = trim(whereCondition.substr(opPos + 1));

        int columnIndex = -1;
        for (size_t i = 0; i < table.columns.size(); ++i) {
            if (table.columns[i].name == columnName) {
                columnIndex = i;
                break;
            }
        }

        if (columnIndex == -1) {
            cout << "Error: Column " << columnName << " not found in table " << tableName << "." << endl;
            return;
        }

        vector<vector<string>> filteredRows;
        for (const auto& row : resultRows) {
            if (row[columnIndex] > value) { 
                filteredRows.push_back(row);
            }
        }
        resultRows = filteredRows;
    }

    
    cout << "+";
    for (const auto& column : resultColumns) {
        cout << string(column.size() + 2, '-') << "+";
    }
    cout << endl;

    cout << "|";
    for (const auto& column : resultColumns) {
        cout << " " << column << " |";
    }
    cout << endl;

    cout << "+";
    for (const auto& column : resultColumns) {
        cout << string(column.size() + 2, '-') << "+";
    }
    cout << endl;

   
    for (const auto& row : resultRows) {
        cout << "|";
        for (size_t i = 0; i < row.size(); ++i) {
            cout << " " << setw(resultColumns[i].size()) << row[i] << " |";
        }
        cout << endl;
    }

    cout << "+";
    for (const auto& column : resultColumns) {
        cout << string(column.size() + 2, '-') << "+";
    }
    cout << endl;
}

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