
#include <cstdio>

template <class Type>
class ProtectionDumper
{
private:
    const Type* obj_;
    const char* function_;
    
    void createGuard(const char* place = "beginning")
    {
        if(!obj_->ok())
        {
            printf("Object failed during ok()\n");
            printf("Function: %s\n", function_);
            printf("Place: %s\n", place);
            obj_->dump("OK Failure");
            assert(obj_->ok());
        }
    }

    ProtectionDumper(const ProtectionDumper& that) = delete;
    const ProtectionDumper& operator =(const ProtectionDumper& that) = delete;

public:
    ProtectionDumper(const Type* obj, const char* function = "UNKNOWN"):
        obj_(obj),
        function_(function)
    {
        createGuard();
    }

    ~ProtectionDumper()
    {
        createGuard("end");
    }
};

#define FUNCTION_GUARD(Type) ProtectionDumper<Type> ___dumper(this, __PRETTY_FUNCTION__);
