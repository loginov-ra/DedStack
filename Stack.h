
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
    const static size_t INITIAL_CAPACITY = 1;

    #ifdef DEBUG
        const static int    CANARY_CORRECT   = 0xBEDABEDA;
        const static size_t HASH_MUL         = 29;
        const static size_t HASH_MOD         = 1000000007;
    #endif

    char* bytes_;
    size_t capacity_;
    size_t size_;  

    #ifdef DEBUG
        std::hash<Type> hasher_;
        size_t hash_sum_;
        size_t size_cpy_;
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
        {
            getElementPointer(i)->~Type();
        }
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
    size_t size() const
    {
       return size_; 
    }

    size_t capacity() const
    {
        return size_;
    }

    bool ok() const
    {
        return this &&
               size_ <= capacity_ &&
               !(size_ > 0 && !bytes_)
               #ifdef DEBUG
                   &&
                   getCanaryLeft()  == this &&
                   getCanaryRight() == 0xBEDABEDA &&
                   hash_sum_ == calculateHashSum() &&
                   size_cpy_ == size_
               #endif
               ;
    }

    void dump(const char* reason = "Unknown reason") const
    {
        printf("D_U_M_P for Stack [this = %p][%s]\n", this, ok() ? "OK" : "FAIL");
        printf("Dump was called. Reason: %s\n", reason);
        printf(" Size: %lu, Capacity: %lu\n", size_, capacity_);
        printf("*Size: %lu\n", size_cpy_);
        printf("Bytes: %p\n", bytes_);

        if (size_ > capacity_)
        {
            printf("Size is more then capacity!\n Strange data!\n");
            return;
        }

        #ifdef DEBUG
            if (size_ != size_cpy_)
            {
                printf("Size is violated!\n");
                return;
            } 
 
            printf("Checksum: %lu (%s)\n", hash_sum_, (hash_sum_ == calculateHashSum()) ? "OK" : "FAIL");
        #endif

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
        #endif
    }

    Stack():
        bytes_(new char[INITIAL_CAPACITY * sizeof(Type) + CANARIES_SIZE]),
        capacity_(INITIAL_CAPACITY),
        size_(0)
        #ifdef DEBUG
            ,
            hasher_(),
            hash_sum_(0),
            size_cpy_(0)
        #endif
    {
        makeCanaries();
    }

    Stack(size_t size, const Type& elem):
        bytes_(new char[size * sizeof(Type) + CANARIES_SIZE]),
        capacity_(size),
        size_(size)
        #ifdef DEBUG
            ,
            hasher_(),
            hash_sum_(0),
            size_cpy_(size)
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
        capacity_(that.capacity_),
        size_(that.size_)
        #ifdef DEBUG
            ,
            hasher_(),
            hash_sum_(that.hash_sum_),
            size_cpy_(that.size_)
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
            size_cpy_ = that.size_;
        #endif
        size_ = that.size_;
        capacity_ = that.capacity_;
        copyElementsFromAnotherStack(that);
        makeCanaries();
    }

    Stack(Stack&& that):
        bytes_(that.bytes_),
        capacity_(that.capacity),
        size_(that.size_)
        #ifdef DEBUG
            ,
            hasher_(that.haser_),
            hash_sum_(that.hash_sum_),
            size_cpy_(that.size_)
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
            size_cpy_ = that.size_;
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
        ++size_cpy_;
        *getElementPointer(size_ - 1) = Type();
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
        --size_cpy_;
        #ifdef DEBUG
            hash_sum_ = calculateHashSum(); 
        #endif
    }
    
    const Type& operator [](size_t i) const
    {
        FUNCTION_GUARD(Stack<Type>);
        ASSERT(i < size_, "Out of range while getting element");
        return *getElementPointer(i);
    }

    const Type& top() const
    {
        FUNCTION_GUARD(Stack<Type>);
        return (*this)[size_ - 1];
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
