#include <windows.h>
#include <string>
#include <iostream>
#include <foreach.h>
#include <axf/pluginapi.h>

std::string s = "lol";
FOREACH_AUTO(c, s)
{
    std::cout << c << std::endl;
}
std::cout << pi_GetBaseDirectory() << std::endl;
