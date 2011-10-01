
#include "wsexception.h"



WSException::WSException(const std::string &msg)
: std::runtime_error(msg)
{

}

WSException::~WSException()
{

}


