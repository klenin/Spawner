#ifndef _SP_MULTIBYTE_HPP_
#define _SP_MULTIBYTE_HPP_

#include <fstream>
#include <string.h>

wchar_t *a2w(const char *str);
char *w2a(const wchar_t *str);

#endif // _SP_MULTIBYTE_HPP_
