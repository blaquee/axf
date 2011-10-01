#ifndef wsexception_h__
#define wsexception_h__

#include <exception>
#include <string>

class WSException : public std::runtime_error
{
public:
    WSException(const std::string &msg);
    virtual ~WSException();
};

#endif // wsexception_h__


