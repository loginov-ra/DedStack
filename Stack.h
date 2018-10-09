
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <functional>
#include <iostream>

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
    const int    CANARY_CORRECT   = 0xBEDABEDA;
    const size_t HASH_MUL         = 29;
    const size_t HASH_MOD         = 1000000007;

    char* bytes_;
    std::hash<Type> hasher_;
    size_t hash_sum_;
    size_t size_;
    size_t capacity_;
    
    Type* getElementPointer(size_t i) const
    {
        ASSERT(i >= 0 && i < size_, "Out of range in getting pointer to element!");
        return reinterpret_cast<Type*>(bytes_ + sizeof(Stack*) + i * sizeof(Type));
    }
    
    void makeCanaries()
    {
        *reinterpret_cast<Stack**>(bytes_) = this;
        *reinterpret_cast<int*>(bytes_ + sizeof(Stack*) + capacity_ * sizeof(Type)) = CANARY_CORRECT;
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
        if (size_ == capacity_)
        {
            bytes_ = reinterpret_cast<char*>(realloc(bytes_, capacity_ * 2 * sizeof(Type) + sizeof(int) + sizeof(Stack*)));
            ASSERT(bytes_, "Unable to allocate bytes\n");
            //Erase old canary
            memset(bytes_ + sizeof(Stack*) + size_ * sizeof(Type), 0, sizeof(int));
            capacity_ *= 2;
            makeCanaries();
        }
    }

    size_t calculateHashSum()
    {
        long long res = 0;
        
        for (size_t i = 0; i < size_; ++i)
        {
            res = (res * HASH_MUL) % HASH_MOD;
            res = (res + hasher_(*getElementPointer(i))) % HASH_MOD;
        }

        return res ^ size_;
    }

    Stack* getCanaryLeft() const
    {
        return *reinterpret_cast<Stack**>(bytes_);
    }

    int getCanaryRight() const
    {
        return *reinterpret_cast<int*>(bytes_ + sizeof(Stack*) + capacity_ * sizeof(Type));
    }
    
public:
    bool ok()
    {
        return this &&
               size_ <= capacity_ &&
               !(size_ > 0 && !bytes_) &&
               getCanaryLeft()  == this &&
               getCanaryRight() == 0xBEDABEDA &&
               hash_sum_ == calculateHashSum();
    }

    void dump(const char* reason = "Unknown reason")
    {
        printf("D_U_M_P for Stack [this = %p][%s]\n", this, ok() ? "OK" : "FAIL");
        printf("Dump was called. Reason: %s\n", reason);
        printf("Size: %lu, Capacity: %lu\n", size_, capacity_);

        printf("Here are elements which stack contains:\n");
        printf("[%p]  %p (CANARY %s)\n", bytes_, getCanaryLeft(), getCanaryLeft() == this ? "OK" : "FAIL");
        
        for (size_t i = 0; i < size_; ++i)
        {
            printf("[%p]* ", bytes_ + sizeof(Stack*) + i * sizeof(Type));
            std::cout << *getElementPointer(i) << std::endl;
        }

        printf("[%p]  %X (CANARY %s)\n", bytes_ + sizeof(Stack*) + capacity_ * sizeof(Type), getCanaryRight(),
                                         getCanaryRight() == 0xBEDABEDA ? "OK" : "FAIL");

        printf("Checksum: %lu (%s)\n", hash_sum_, (hash_sum_ == calculateHashSum()) ? "OK" : "FAIL");
    }

    Stack():
        bytes_(new char[INITIAL_CAPACITY * sizeof(Type) + sizeof(int) + sizeof(Stack*)]),
        hasher_(),
        hash_sum_(0),
        size_(0),
        capacity_(INITIAL_CAPACITY)
    {
        makeCanaries();
    }

    Stack(size_t size, const Type& elem):
        bytes_(new char[size * sizeof(Type) + sizeof(int) + sizeof(Stack*)]),
        hasher_(),
        hash_sum_(0),
        size_(size),
        capacity_(size)
    {
        for (size_t i = 0; i < size; ++i)
        {
            *getElementPointer(i) = Type();
            *getElementPointer(i) = elem;
        }

        makeCanaries();
        hash_sum_ = calculateHashSum();
    }

    Stack(const Stack& that):
        bytes_(new char[that.capacity_ * sizeof(Type) + sizeof(int) + sizeof(Stack*)]),
        hasher_(),
        hash_sum_(that.hash_sum),
        size_(that.size_),
        capacity_(that.capacity_)
    {
        ASSERT(that.ok(), "Copied stack is not ok");
        *reinterpret_cast<Stack<Type>**>(bytes_) = this;
        copyElementsFromAnotherStack(that);
        makeCanaries();
    }

    const Stack& operator =(const Stack& that)
    {
        ASSERT(that.ok(), "Copied stack is not ok");
        destroyElements();
        delete[] bytes_;
        bytes_ = new char[that.capacity_ * sizeof(Type) + sizeof(int) + sizeof(Stack*)];
        hasher_ = that.hasher_;
        hash_sum_ = that.hash_sum_;
        size_ = that.size_;
        capacity_ = that.capacity_;
        copyElementsFromAnotherStack(that);
        makeCanaries();
    }

    Stack(Stack&& that):
        bytes_(that.bytes_),
        hasher_(that.haser_),
        hash_sum_(that.hash_sum_),
        size_(that.size_),
        capacity_(that.capacity_)
    {
        ASSERT(that.ok(), "Moved stack is not ok");
        makeCanaries();
        that.bytes_ = nullptr;
    }

    Stack&& operator =(Stack&& that)
    {
        ASSERT(that.ok(), "Moved stack is not ok");
        bytes_ = that.bytes_;
        hasher_ = that.hasher_;
        hash_sum_ = that.hash_sum_;
        size_ = that.size_;
        capacity_ = that.capacity_;
        copyElementsFromAnotherStack(that);
        makeCanaries();
        that.bytes_ = nullptr;
    }

    void push(const Type& elem)
    {
        tryToExpand();
        ++size_;
        *getElementPointer(size_ - 1) = elem;
        hash_sum_ = calculateHashSum();
    }

    void pop()
    {
        ASSERT(size_ > 0, "Called pop() of an empty stack");
        getElementPointer(size_ - 1)->~Type();
        --size_;
        hash_sum_ = calculateHashSum(); 
    }

    const Type& top() const
    {
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
