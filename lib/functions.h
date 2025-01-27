#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <string>

#ifdef __cplusplus
extern "C" {
#endif

void function1(std::string& str);
int function2(const std::string& str);
bool function3(const std::string& str);

#ifdef __cplusplus
}
#endif

#endif // FUNCTIONS_H