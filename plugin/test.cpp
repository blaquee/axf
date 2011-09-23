#include <windows.h>
#include <string>
#include <iostream>
#include <foreach.h>
std::string s = "lol";
std::cout << s << std::endl;

FOREACH_AUTO(c, s)
{
    std::cout << c << std::endl;
}
