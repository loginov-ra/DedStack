
#define DEBUG

#include "Stack.h"
#include <string>
#include <memory>

void testPushPop()
{
    Stack<std::string> strings;

    for (size_t i = 0; i < 100; ++i)
    {
        strings.push("a");     
    }
    
    ASSERT(strings.top() == "a", "Incorrect top!\n");

    for (size_t i = 0; i < 100; ++i)
        strings.pop();

    strings.pop();
}

void testEraseRight()
{
    Stack<std::string> strings(1000, "aaa");

    for (size_t i = 0; i < 985; ++i)
        strings.pop();

    std::string* ptr = const_cast<std::string*>(&strings.top());
    memset(ptr + 18, 0, sizeof(int));
    memset(ptr + 986, 0, sizeof(int));
    strings.push("a");
}

void testReallocBySharedPtr(int* raw)
{
    std::shared_ptr<int> ptr(raw);
    Stack<std::shared_ptr<int>> ptrs;
    for (size_t i = 0; i < 100; ++i)
        ptrs.push(std::shared_ptr<int>(ptr));
    
    ASSERT(ptrs.top().use_count() == 101, "Wrong copying");
}

void testHashSumCrash()
{
    Stack<std::string> strings(15, "a");
    std::string* ptr = const_cast<std::string*>(&strings[5]);
    memset(ptr, 0, 2 * sizeof(std::string));
    strings.push("hash_sum");
}

void testEraseStructMemLeft()
{
    Stack<int> st;
    st.push(20);
    st.push(18);
    
    memset(&st, 0, sizeof(st) / 3);
    st.push(1);
}

void testEraseStructMemRight()
{
    Stack<size_t> st;
    st.push(10);
    st.push(14);

    char* ptr = reinterpret_cast<char*>(&st) + sizeof(st) * 2 / 3;
    memset(ptr, 0, sizeof(st) / 3);
    st.push(std::hash<std::string>()("Otl_10"));
}

int main()
{
    //int* var = new int[5];
    testEraseStructMemRight();
    return 0;
}
