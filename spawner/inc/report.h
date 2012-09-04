#ifndef _REPORT_H_
#define _REPORT_H_

#include <string>
#include "status.h"
using namespace std;

const char *get_exception_name(exception_t exception);
const char *get_exception_text(exception_t exception);
string get_exception_info(exception_t exception, string format);

#endif//_REPORT_H_