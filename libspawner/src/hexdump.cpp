#include "hexdump.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <memory>

#include "mutex.h"
#include "buffer.h"

void hexDump(const void *addr, int len) {
    static mutex_c mutex;
    mutex.lock();
    int i;
    unsigned char buff[17];
    unsigned char *pc = (unsigned char*)addr;

    for (i = 0; i < len; i++) {
        if ((i % 16) == 0) {
            if (i != 0)
                dprintf("  %s\n", buff);
            dprintf("  %04x ", i);
        }

        dprintf(" %02x", pc[i]);

        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
            buff[i % 16] = '.';
        else
            buff[i % 16] = pc[i];
        buff[(i % 16) + 1] = '\0';
    }

    while ((i % 16) != 0) {
        dprintf("   ");
        i++;
    }

    dprintf("  %s\n", buff);
    mutex.unlock();
}

void dprintf(const char* format, ...) {
  int final_n;
  int n = strlen(format) * 2;
  std::string str;
  std::unique_ptr<char[]> formatted;
  va_list ap;
  for (;;) {
    formatted.reset(new char[n]);
    strcpy(&formatted[0], format);
    va_start(ap, format);
    final_n = vsnprintf(&formatted[0], n, format, ap);
    va_end(ap);
    if (final_n < 0 || final_n >= n) {
      n += std::abs(final_n - n + 1);
    } else {
      break;
    }
  }
  static output_stdout_buffer_c stdout_buffer(FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_RED);
  stdout_buffer.write(formatted.get(), final_n);
}
