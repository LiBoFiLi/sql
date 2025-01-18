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

    string tableName1 = trim(command.substr(tableNameStart, 
        (joinPos == string::npos ? (wherePos == string::npos ? command.size() : wherePos) : joinPos) - tableNameStart));

    if (!tables.count(tableName1)) {
        cout << "Error: Table " << tableName1 << " does not exist." << endl;
        return;
    }

    Table& table1 = tables[tableName1];
    vector<vector<string>> resultRows = table1.rows;
    vector<string> resultColumns;

    for (const auto& col : table1.columns) {
        resultColumns.push_back(col.name);
    }

    // JOIN
    if (joinPos != string::npos) {
        size_t joinTableStart = joinPos + 4;
        size_t onPos = command.find("ON", joinTableStart);
        string tableName2 = trim(command.substr(joinTableStart, onPos == string::npos ? 
            (wherePos == string::npos ? command.size() : wherePos) : onPos - joinTableStart));

        if (!tables.count(tableName2)) {
            cout << "Error: Table " << tableName2 << " does not exist." << endl;
            return;
        }

        Table& table2 = tables[tableName2];

        for (const auto& col : table2.columns) {
            resultColumns.push_back(col.name);
        }

        if (onPos != string::npos) {
            size_t onConditionStart = onPos + 2;
            string onCondition = trim(command.substr(onConditionStart, 
                wherePos == string::npos ? command.size() - onConditionStart : wherePos - onConditionStart));

            size_t eqPos = onCondition.find("=");
            if (eqPos == string::npos) {
                cout << "Error: Invalid ON condition." << endl;
                return;
            }

            string t1Column = trim(onCondition.substr(0, eqPos));
            string t2Column = trim(onCondition.substr(eqPos + 1));

            int t1Idx = -1, t2Idx = -1;
            for (size_t i = 0; i < table1.columns.size(); ++i) {
                if (table1.columns[i].name == t1Column) {
                    t1Idx = i;
                    break;
                }
            }
            for (size_t i = 0; i < table2.columns.size(); ++i) {
                if (table2.columns[i].name == t2Column) {
                    t2Idx = i;
                    break;
                }
            }

            if (t1Idx == -1 || t2Idx == -1) {
                cout << "Error: Column not found in ON condition." << endl;
                return;
            }

            vector<vector<string>> joinedRows;
            for (const auto& row1 : table1.rows) {
                for (const auto& row2 : table2.rows) {
                    if (row1[t1Idx] == row2[t2Idx]) {
                        vector<string> combinedRow = row1;
                        combinedRow.insert(combinedRow.end(), row2.begin(), row2.end());
                        joinedRows.push_back(combinedRow);
                    }
                }
            }
            resultRows = joinedRows;

        } else {
            vector<vector<string>> cartesianRows;
            for (const auto& row1 : table1.rows) {
                for (const auto& row2 : table2.rows) {
                    vector<string> combinedRow = row1;
                    combinedRow.insert(combinedRow.end(), row2.begin(), row2.end());
                    cartesianRows.push_back(combinedRow);
                }
            }
            resultRows = cartesianRows;
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

        string columnName1 = trim(whereCondition.substr(0, opPos));
        string rightOperand = trim(whereCondition.substr(opPos + 1));

        int columnIndex1 = -1, columnIndex2 = -1;
        bool isValueComparison = rightOperand.front() == '"' && rightOperand.back() == '"';

        
        for (size_t i = 0; i < resultColumns.size(); ++i) {
            if (resultColumns[i] == columnName1) {
                columnIndex1 = i;
            }
            if (!isValueComparison && resultColumns[i] == rightOperand) {
                columnIndex2 = i;
            }
        }

        if (columnIndex1 == -1 || (!isValueComparison && columnIndex2 == -1)) {
            cout << "Error: Column not found in WHERE condition." << endl;
            return;
        }

        vector<vector<string>> filteredRows;
        for (const auto& row : resultRows) {
            if (isValueComparison) {
                string value = rightOperand.substr(1, rightOperand.size() - 2); // Remove quotes
                if (row[columnIndex1] > value) {
                    filteredRows.push_back(row);
                }
            } else {
                if (row[columnIndex1] > row[columnIndex2]) {
                    filteredRows.push_back(row);
                }
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