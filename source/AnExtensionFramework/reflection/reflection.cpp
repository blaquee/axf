#include "reflection.h"


Scope_::Scope_( Reflector *reflector )
    : reflector(reflector)
{

}

Namespace_::Namespace_( Reflector *reflector,const char *name )
    : Scope_(reflector), name(name)
{

}
Namespace_::Namespace_( const char *name )
    : Scope_(0), name(name)
{

}

Class_::Class_( Reflector *reflector, const char *name )
    : Scope_(reflector), name(name)
{

}

Class_::Class_( const char *name )
    : Scope_(0), name(name)
{

}

Var_::Var_( Reflector *reflector, const char *name )
    : Scope_(reflector), name(name)
{

}
Var_::Var_( const char *name )
    : Scope_(0), name(name)
{

}
