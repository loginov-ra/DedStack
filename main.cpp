
#define DEBUG

#include "Stack.h"
#include <string>

int main()
{
    Stack<std::string> s(5, "aac");
    s.pop();
    s.push("aap");
    s.dump();
    return 0;
}
