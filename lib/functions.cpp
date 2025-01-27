#include "functions.h"
#include <algorithm>
#include <cctype>

using namespace std;

void function1(string& str) {
    sort(str.rbegin(), str.rend());
    string result;
    for (char c : str) {
        if ((c % 2) == 0) {
            result += "KB";
        } else {
            result += c;
        }
    }
    str = result;
}

int function2(const string& str) {
    int sum = 0;
    for (char c : str) {
        if (isdigit(c)) {
            sum += c;
        }
    }
    return sum;
}

bool function3(const string& str) {
    size_t len = str.size();
    return len > 2 && len % 32 == 0;
}