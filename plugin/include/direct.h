#ifndef __DIRECT_H
#define __DIRECT_H
const int _MAX_PATH=256;

#ifdef __win32__
#lib msvcrt40.dll
#else
#lib libc.so.6
#endif
extern "C" {
  char* _getcwd(char* buff, int sz);
}
#lib
#endif