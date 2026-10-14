#include "serial.h"
#include "io.h"

/* COM1 register layout.  QEMU exposes COM1 at 0x3F8 by default. */
#define COM1_DATA          0x3F8
#define COM1_IER         0x3F9
#define COM1_FCR         0x3FA
#define COM1_LCR         0x3FB
#define COM1_MCR         0x3FC
#define COM1_LSR         0x3FD
#define COM1_MSR         0x3FE

/* LSR status bits. */
#define LSR_THRE         0x20   /* transmit holding register empty  */

/* Line-control bits. */
#define LCR_DLAB         0x80   /* divisor latch access             */
#define LCR_WORD_8N1     0x03   /* 8 data bits, no parity, 1 stop   */

/* FIFO control bits. */
#define FCR_ENABLE       0x01
#define FCR_CLR_RX       0x02
#define FCR_CLR_TX       0x04
#define FCR_FIFO8        0xC0   /* 8-byte FIFO + trigger level 7   */

/* MCR bits. */
#define MCR_RTS          0x02
#define MCR_DTR          0x01

/* Baud rate divisor for 115200 bps (XTAL 1.8432 MHz / 16). */
#define BAUD_115200_L    0x01
#define BAUD_115200_H    0x00

/* Spin until the LSR bit is set.  No timeout needed — QEMU serial
   always has the FIFO.  Bounded to avoid an infinite hang if a
   hardware fault prevents the bit from ever being set. */
static inline void wait_thre(void) {
    int spin = 1000000;
    while (spin--) {
        if (inb(COM1_LSR) & LSR_THRE)
            return;
    }
}

void serial_init(void) {
    outb(COM1_IER,  0x00);               /* 1. disable all interrupts  */
    outb(COM1_LCR,  LCR_DLAB);            /* 2. enable divisor latch    */
    outb(COM1_DATA, BAUD_115200_L);       /* 3. baud divisor low        */
    outb(COM1_IER,  BAUD_115200_H);       /* 4. baud divisor high       */
    outb(COM1_LCR,  LCR_WORD_8N1);        /* 5. clear DLAB, 8N1         */
    outb(COM1_FCR,  FCR_ENABLE|FCR_CLR_RX|FCR_CLR_TX|FCR_FIFO8);
    outb(COM1_MCR,  MCR_RTS|MCR_DTR);     /* RTS/DTR high (loopback test) */
}

void serial_putc(char c) {
    if (c == '\n')
        serial_putc('\r');
    wait_thre();
    outb(COM1_DATA, (uint8_t)c);
}

void serial_write(const char *s) {
    while (*s)
        serial_putc(*s++);
}

void serial_write_string(const char *s) {
    serial_write(s);
}

void serial_write_uint(uint32_t value) {
    char buf[12];
    int i = 11;
    buf[11] = '\0';
    do {
        buf[i--] = (char)('0' + (int)(value % 10));
        value /= 10;
    } while (value != 0);
    serial_write(&buf[i + 1]);
}

void serial_write_hex(uint32_t value) {
    const char *digits = "0123456789abcdef";
    char buf[9];
    buf[8] = '\0';
    for (int i = 7; i >= 0; i--) {
        buf[i] = digits[value & 0xF];
        value >>= 4;
    }
    serial_write("0x");
    serial_write(buf);
}
