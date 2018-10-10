
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <functional>
#include <iostream>
#include "ProtectionDumper.h"

#ifdef DEBUG
    #define L_CANARY_SIZE sizeof(Stack*)
    #define R_CANARY_SIZE sizeof(int)
    #define CANARIES_SIZE sizeof(Stack*) + sizeof(int)
#else
    #define L_CANARY_SIZE 0
    #define R_CANARY_SIZE 0
    #define CANARIES_SIZE 0
#endif

#define ASSERT(COND, MSG)                                       \
    if(!(COND))                                                 \
    {                                                           \
        fprintf(stderr, "Error happened: %s\n", MSG);           \
        fprintf(stderr, "Function: %s\n", __PRETTY_FUNCTION__); \
        fprintf(stderr, "Line: %d\n", __LINE__);                \
        assert(COND);                                           \
    }

template <typename Type>
class Stack
{
private:
    const size_t INITIAL_CAPACITY = 1;
    
    #ifdef DEBUG
        const int    CANARY_CORRECT   = 0xBEDABEDA;
        const size_t HASH_MUL         = 29;
        const size_t HASH_MOD         = 1000000007;
    #endif

    char* bytes_;
    size_t size_;  
    size_t capacity_;

    #ifdef DEBUG
        std::hash<Type> hasher_;
        size_t hash_sum_;
    #endif

    Type* getElementPointer(size_t i) const
    {
        ASSERT(i >= 0 && i < size_, "Out of range in getting pointer to element!");
        return reinterpret_cast<Type*>(bytes_ + L_CANARY_SIZE + i * sizeof(Type));
    }
    
    void makeCanaries()
    {
        #ifdef DEBUG
            *reinterpret_cast<Stack**>(bytes_) = this;
            *reinterpret_cast<int*>(bytes_ + L_CANARY_SIZE + capacity_ * sizeof(Type)) = CANARY_CORRECT;
        #endif
    }

    void copyElementsFromAnotherStack(const Stack& that)
    {
        ASSERT(that.size_ >= size_, "Copying less stack to bigger one");

        for (size_t i = 0; i < size_; ++i)
        {
            *getElementPointer(i) = Type();
            *getElementPointer(i) = *that.getElementPointer(i);   
        }
    }

    void destroyElements()
    {
        for (size_t i = 0; i < size_; ++i)
            getElementPointer(i)->~Type();
    }

    void tryToExpand()
    {
        FUNCTION_GUARD(Stack<Type>);
        if (size_ == capacity_)
        {
            bytes_ = reinterpret_cast<char*>(realloc(bytes_, capacity_ * 2 * sizeof(Type) + CANARIES_SIZE));
            ASSERT(bytes_, "Unable to allocate bytes\n");
            #ifdef DEBUG
                memset(bytes_ + L_CANARY_SIZE + size_ * sizeof(Type), 0, sizeof(int));
            #endif
            capacity_ *= 2;
            makeCanaries();
        }
    }
    
    size_t calculateHashSum() const
    {
        #ifdef DEBUG
            long long res = 0;
        
            for (size_t i = 0; i < size_; ++i)
            {
                res = (res * HASH_MUL) % HASH_MOD;
                res = (res + hasher_(*getElementPointer(i))) % HASH_MOD;
            }

            return res ^ size_;
        #endif
    }

    #ifdef DEBUG
        Stack* getCanaryLeft() const
        {
            return *reinterpret_cast<Stack**>(bytes_);
        }

        int getCanaryRight() const
        {
            return *reinterpret_cast<int*>(bytes_ + sizeof(Stack*) + capacity_ * sizeof(Type));
        }
    #endif
    
public:
    bool ok() const
    {
        return this &&
               size_ <= capacity_ &&
               !(size_ > 0 && !bytes_)
               #ifdef DEBUG
                   &&
                   getCanaryLeft()  == this &&
                   getCanaryRight() == 0xBEDABEDA &&
                   hash_sum_ == calculateHashSum()
               #endif
               ;
    }

    void dump(const char* reason = "Unknown reason") const
    {
        printf("D_U_M_P for Stack [this = %p][%s]\n", this, ok() ? "OK" : "FAIL");
        printf("Dump was called. Reason: %s\n", reason);
        printf("Size: %lu, Capacity: %lu\n", size_, capacity_);

        printf("Here are elements which stack contains:\n");
        
        #ifdef DEBUG
            printf("[%p]  %p (CANARY %s)\n", bytes_, getCanaryLeft(), getCanaryLeft() == this ? "OK" : "FAIL");
        #endif

        for (size_t i = 0; i < size_; ++i)
        {
            printf("[%p]* ", bytes_ + L_CANARY_SIZE + i * sizeof(Type));
            std::cout << *getElementPointer(i) << std::endl;
        }
        
        #ifdef DEBUG
            printf("[%p]  %X (CANARY %s)\n", bytes_ + L_CANARY_SIZE + capacity_ * sizeof(Type), getCanaryRight(),
                                             getCanaryRight() == 0xBEDABEDA ? "OK" : "FAIL");

            printf("Checksum: %lu (%s)\n", hash_sum_, (hash_sum_ == calculateHashSum()) ? "OK" : "FAIL");
        #endif
    }

    Stack():
        bytes_(new char[INITIAL_CAPACITY * sizeof(Type) + CANARIES_SIZE]),
        size_(0),
        capacity_(INITIAL_CAPACITY)
        #ifdef DEBUG
            ,
            hasher_(),
            hash_sum_(0)
        #endif
    {
        makeCanaries();
    }

    Stack(size_t size, const Type& elem):
        bytes_(new char[size * sizeof(Type) + CANARIES_SIZE]),
        size_(size),
        capacity_(size)
        #ifdef DEBUG
            ,
            hasher_(),
            hash_sum_(0)
        #endif
    {
        for (size_t i = 0; i < size; ++i)
        {
            *getElementPointer(i) = Type();
            *getElementPointer(i) = elem;
        }

        makeCanaries();
        #ifdef DEBUG
            hash_sum_ = calculateHashSum();
        #endif
    }

    Stack(const Stack& that):
        bytes_(new char[that.capacity_ * sizeof(Type) + sizeof(int) + sizeof(Stack*)]),
        size_(that.size_),
        capacity_(that.capacity_)
        #ifdef DEBUG
            ,
            hasher_(),
            hash_sum_(that.hash_sum_)
        #endif

    {
        ASSERT(that.ok(), "Copied stack is not ok");
        #ifdef DEBUG
            *reinterpret_cast<Stack<Type>**>(bytes_) = this;
        #endif
        copyElementsFromAnotherStack(that);
        makeCanaries();
    }

    const Stack& operator =(const Stack& that)
    {
        ASSERT(that.ok(), "Copied stack is not ok");
        destroyElements();
        delete[] bytes_;
        bytes_ = new char[that.capacity_ * sizeof(Type) + CANARIES_SIZE];
        #ifdef DEBUG
            hasher_ = that.hasher_;
            hash_sum_ = that.hash_sum_;
        #endif
        size_ = that.size_;
        capacity_ = that.capacity_;
        copyElementsFromAnotherStack(that);
        makeCanaries();
    }

    Stack(Stack&& that):
        bytes_(that.bytes_),
        size_(that.size_),
        capacity_(that.capacity)
        #ifdef DEBUG
        ,
        hasher_(that.haser_),
        hash_sum_(that.hash_sum_)
        #endif
    {
        ASSERT(that.ok(), "Moved stack is not ok");
        makeCanaries();
        that.bytes_ = nullptr;
    }

    Stack&& operator =(Stack&& that)
    {
        ASSERT(that.ok(), "Moved stack is not ok");
        bytes_ = that.bytes_;
        #ifdef DEBUG
            hasher_ = that.hasher_;
            hash_sum_ = that.hash_sum_;
        #endif
        size_ = that.size_;
        capacity_ = that.capacity_;
        copyElementsFromAnotherStack(that);
        makeCanaries();
        that.bytes_ = nullptr;
    }

    void push(const Type& elem)
    {
        FUNCTION_GUARD(Stack<Type>);
        tryToExpand();
        ++size_;
        *getElementPointer(size_ - 1) = elem;
        #ifdef DEBUG
            hash_sum_ = calculateHashSum();
        #endif
    }

    void pop()
    {
        FUNCTION_GUARD(Stack<Type>);
        ASSERT(size_ > 0, "Called pop() of an empty stack");
        getElementPointer(size_ - 1)->~Type();
        --size_;
        #ifdef DEBUG
            hash_sum_ = calculateHashSum(); 
        #endif
    }

    const Type& top() const
    {
        FUNCTION_GUARD(Stack<Type>);
        return *getElementPointer(size_ - 1);
    }

    ~Stack()
    {
        if (bytes_)
        {
            destroyElements();
            delete[] bytes_;
        }
    }
};
