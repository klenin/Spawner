#ifndef _MULTIBYTE_H_
#define _MULTIBYTE_H_
#include <fstream>
#include <string.h>

wchar_t *a2w(const char *str);
char *w2a(const wchar_t *str);
#endif //_MULTIBYTE_H_
