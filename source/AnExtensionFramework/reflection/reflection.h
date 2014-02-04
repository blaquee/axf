#ifndef reflection_h__
#define reflection_h__

#include "object.h"

class Reflector;
class Class_;

class Scope_
{
public:
    Scope_(Reflector *reflector);

    Scope_ operator[](Scope_ s)
    {
        return s;
    }
    Scope_ operator,(Scope_ s)
    {
        return s;
    }

protected:
    Reflector *reflector;
};

class Var_ : public Scope_
{
public:
    Var_(const char *name);
    Var_(Reflector *reflector, const char *name);

    Class_ &operator,(Class_ &s)
    {
        return s;
    }
    Var_ &operator,(Var_ &s)
    {
        return s;
    }
private:
    const char *name;
};

class Class_ : public Scope_
{
public:
    Class_(const char *name);
    Class_(Reflector *reflector, const char *name);
    Class_ &Parent(const char *name){}

    Class_ operator[](const Class_ &s)
    {
        return s;
    }
    Var_ operator[](const Var_ &s)
    {
        return s;
    }
    Class_ operator,(Class_ s)
    {
        return s;
    }
    Var_ operator,(Var_ s)
    {
        return s;
    }
private:
    const char *name;
};


class Namespace_ : public Scope_
{
public:
    Namespace_(const char *name);
    Namespace_(Reflector *reflector,const char *name);

    Class_ operator[](const Class_ &s)
    {
        return s;
    }
    Var_ operator[](const Var_ &s)
    {
        return s;
    }
    Namespace_ operator[](const Namespace_ &s)
    {
        return s;
    }

    Class_ operator,(Class_ s)
    {
        return s;
    }
    Var_ operator,(Var_ s)
    {
        return s;
    }
    Namespace_ operator,(Namespace_ s)
    {
        return s;
    }

private:
    const char *name;
};

class Reflector
{
public:
    TypeRegistry registry;

    Namespace_ NamespaceBegin(const char *name)
    {
        return Namespace_(this, name);
    }
private:

};

#define namespace_begin(reflector, name_) reflector->NamespaceBegin(#name_)

#define namespace_(name_) Namespace_(#name_)
#define class_(name_) Class_(#name_)
#define var(name_) Var_(#name_)
#define parent(name_) Parent(#name_)



namespace Example
{
    class Test
    {
        int a;
        float b;
        char c;
        unsigned int d;
        double e;
        char f[100];

        int dostuff(float i);

        class InnerClass
        {
            int a;
        };
    };

    class TestInterface
    {
    public:
        virtual void a()=0;
    };

    namespace InnerNamespace
    {
        class TestConcrete : public TestInterface, public Test
        {
            const char ch;
        public:
            void a();
        };
    }
}

void registerit(Reflector *reflector)
{
namespace_begin(reflector, Example)
[
    class_(Example::Test)   // 2 levels of nesting, good
    [
        var(&Example::Test::a),
        var(&Example::Test::b),
        var(&Example::Test::c),
        var(&Example::Test::d),
        var(&Example::Test::e),
        var(&Example::Test::f),
        var(&Example::Test::dostuff),

        class_(Example::Test::InnerClass)   // 3 levels of nesting, avoid!
        [
            var(&Example::Test::InnerClass::a)
        ]
    ],

    class_(Example::TestInterface)  // 2 levels of nesting, good
    [
        var(&Example::TestInterface::a)
    ],

    namespace_(Example::InnerNamespace)
    [
        class_(Example::InnerNamespace::TestConcrete)  // 3 levels of nesting, avoid!
        .parent(Example::TestInterface)
        .parent(Example::Test)
        [
            var(&Example::InnerNamespace::TestConcrete::ch)
        ]
    ]
]; //end of Module Example
}


#endif // reflection_h__

