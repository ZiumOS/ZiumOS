#ifndef DRIVERS_SERIAL_H
#define DRIVERS_SERIAL_H

#include <stdint.h>

/* Serial port driver (COM1 = 0x3F8, 115200-8-N-1, FIFO enabled). */

void serial_init(void);
void serial_putc(char c);
void serial_write(const char *s);
void serial_write_string(const char *s);
void serial_write_uint(uint32_t value);
void serial_write_hex(uint32_t value);

#endif /* DRIVERS_SERIAL_H */