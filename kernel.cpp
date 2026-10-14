// kernel.cpp - ZiumOS kernel: native framebuffer GUI + PS/2 input
//
// Capabilities:
//   - Framebuffer via Multiboot2 info tag (GRUB2 configures VBE mode)
//   - Software-rendered GUI (no text-mode fakeout)
//   - PS/2 keyboard (IRQ1) + mouse (IRQ12) via PIC
//   - PIC remap, PIT timer, IDT, serial diagnostics on COM1

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

extern "C" uint32_t mb_info_ptr;

// ---- I/O ----
static inline void outb(uint16_t port, uint8_t val) {
    asm volatile("outb %1, %0" :: "Nd"(port), "a"(val));
}
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}
static inline void outw(uint16_t port, uint16_t val) {
    asm volatile("outw %1, %0" :: "Nd"(port), "a"(val));
}

// ---- Serial (COM1 115200) ----
static void serial_init() {
    outb(0x3F9, 0x00);
    outb(0x3F8 + 3, 0x80); outb(0x3F8 + 0, 0x01); outb(0x3F8 + 1, 0x00);
    outb(0x3F8 + 3, 0x03); outb(0x3F8 + 2, 0xC7); outb(0x3F8 + 4, 0x0B);
}
static void serial_putc(char c) {
    while (!(inb(0x3F8 + 5) & 0x20));
    outb(0x3F8, (uint8_t)c);
}
static void serial_puts(const char *s) { while (*s) serial_putc(*s++); }
static void serial_hex(uint32_t v) {
    const char h[] = "0123456789ABCDEF";
    serial_putc('0'); serial_putc('x');
    for (int i = 7; i >= 0; i--) serial_putc(h[(v >> (i*4)) & 0xF]);
}
static void serial_dec(uint32_t v) {
    char b[12]; int i = 11; b[11] = 0;
    if (v == 0) { serial_putc('0'); return; }
    while (v > 0 && i > 0) b[--i] = '0' + (v % 10), v /= 10;
    serial_puts(&b[i]);
}

// ---- Multiboot2 framebuffer parsing ----
static uint8_t  *g_fb = nullptr;
static uint32_t g_fb_w = 0, g_fb_h = 0, g_fb_bpp = 0, g_fb_pitch = 0;
static bool     g_fb_ok = false;

#pragma pack(push, 1)
struct Mb2Tag { uint32_t type; uint32_t size; };
struct Mb2Fb {
    uint32_t type, size;
    uint32_t fb_addr, fb_pitch, fb_width, fb_height;
    uint8_t  fb_bpp, fb_type;
    uint16_t reserved;
    uint32_t num_colors;
};
#pragma pack(pop)

static bool init_framebuffer() {
    serial_puts("MB2 fb: ptr=0x"); serial_hex((uint32_t)mb_info_ptr); serial_putc('\n');
    if (!mb_info_ptr) return false;
    uint32_t total = *(uint32_t *)mb_info_ptr;
    uint8_t *p = (uint8_t *)(mb_info_ptr + 8);
    while ((uint32_t)(p - (uint8_t *)(mb_info_ptr + 8)) < total) {
        uint32_t t = *(uint32_t *)p, s = *(uint32_t *)(p + 4);
        if (t == 0) break;
        if (t == 8 && s >= 32) {
            const Mb2Fb *fb = (const Mb2Fb *)p;
            g_fb      = (uint8_t *)fb->fb_addr;
            g_fb_w    = fb->fb_width;
            g_fb_h    = fb->fb_height;
            g_fb_bpp  = fb->fb_bpp;
            g_fb_pitch = fb->fb_pitch;
            serial_puts("  fb  "); serial_dec(g_fb_w); serial_putc('x'); serial_dec(g_fb_h);
            serial_puts("bpp="); serial_dec(g_fb_bpp);
            serial_puts(" pitch="); serial_dec(g_fb_pitch);
            serial_puts(" addr=0x"); serial_hex((uint32_t)fb->fb_addr); serial_putc('\n');
            if (g_fb_w && g_fb_h && g_fb_pitch && (g_fb_bpp == 32 || g_fb_bpp == 16) && fb->fb_addr) {
                g_fb_ok = true;
                return true;
            }
            return false;
        }
        p += (s + 7) & ~7u;
    }
    serial_puts("  no framebuffer tag\n");
    return false;
}

// ---- Pixel ops ----
static uint32_t pack_color(uint8_t r, uint8_t g, uint8_t b) {
    if (g_fb_bpp == 32) return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
    return (((uint32_t)(r >> 3) & 0x1F) << 11) | (((uint32_t)(g >> 2) & 0x3F) << 5) | ((uint32_t)(b >> 3) & 0x1F);
}
static void draw_pixel(int x, int y, uint32_t c) {
    if (!g_fb_ok || x < 0 || (uint32_t)x >= g_fb_w || y < 0 || (uint32_t)y >= g_fb_h) return;
    uint32_t off = (uint32_t)y * g_fb_pitch + (uint32_t)x * (g_fb_bpp / 8);
    if (g_fb_bpp == 32) *(uint32_t *)(g_fb + off) = c;
    else                *(uint16_t *)(g_fb + off) = (uint16_t)c;
}
static void fill_screen(uint32_t c) {
    if (!g_fb_ok) return;
    for (uint32_t y = 0; y < g_fb_h; y++)
        for (uint32_t x = 0; x < g_fb_w; x++)
            draw_pixel((int)x, (int)y, c);
}
static void draw_rect(int x0, int y0, int w, int h, uint32_t c) {
    for (int y = y0; y < y0 + h; y++)
        for (int x = x0; x < x0 + w; x++)
            draw_pixel(x, y, c);
}

// ---- PS/2 input ----
static char g_last_key = 0;
static int  g_mouse_x = 0, g_mouse_y = 0;
static uint8_t g_mouse_btns = 0;
static uint8_t g_mouse_phase = 0;
static bool   g_shift = false;

static char sc1_to_ascii(uint8_t sc) {
    switch (sc) {
        case 0x01: return 0x1B; case 0x1C: return '\n'; case 0x39: return ' ';
        case 0x1E: return g_shift?'A':'a'; case 0x30: return g_shift?'B':'b';
        case 0x2E: return g_shift?'C':'c'; case 0x20: return g_shift?'D':'d';
        case 0x12: return g_shift?'E':'e'; case 0x21: return g_shift?'F':'f';
        case 0x22: return g_shift?'G':'g'; case 0x23: return g_shift?'H':'h';
        case 0x17: return g_shift?'I':'i'; case 0x24: return g_shift?'J':'j';
        case 0x25: return g_shift?'K':'k'; case 0x26: return g_shift?'L':'l';
        case 0x32: return g_shift?'M':'m'; case 0x31: return g_shift?'N':'n';
        case 0x18: return g_shift?'O':'o'; case 0x19: return g_shift?'P':'p';
        case 0x10: return g_shift?'Q':'q'; case 0x13: return g_shift?'R':'r';
        case 0x1F: return g_shift?'S':'s'; case 0x14: return g_shift?'T':'t';
        case 0x16: return g_shift?'U':'u'; case 0x3D: return g_shift?'V':'v';
        case 0x2D: return g_shift?'W':'w'; case 0x3E: return g_shift?'X':'x';
        case 0x15: return g_shift?'Y':'y'; case 0x2C: return g_shift?'Z':'z';
        case 0x02: return g_shift?'!':'1'; case 0x03: return g_shift?'@':'2';
        case 0x04: return g_shift?'#':'3'; case 0x05: return g_shift?'$':'4';
        case 0x06: return g_shift?'%':'5'; case 0x07: return g_shift?'^':'6';
        case 0x08: return g_shift?'&':'7'; case 0x09: return g_shift?'*':'8';
        case 0x0A: return g_shift?'(':'9'; case 0x0B: return g_shift?')':'0';
        default: return 0;
    }
}

extern "C" void handle_pic(int irq) {
    if (irq == 1) {
        uint8_t sc = inb(0x60);
        if (sc == 0x2A || sc == 0x36) g_shift = true;
        else if (sc == 0xAA || sc == 0xB6) g_shift = false;
        else if (!(sc & 0x80)) g_last_key = sc1_to_ascii(sc);
    } else if (irq == 12) {
        uint8_t d = inb(0x60);
        switch (g_mouse_phase) {
            case 0: g_mouse_btns = d & 0x07; g_mouse_phase = 1; break;
            case 1: g_mouse_x += (d & 0x10) ? -(d & 0xF) : (d & 0xF); g_mouse_phase = 2; break;
            case 2: g_mouse_y += (d & 0x20) ? -(d & 0xF) : (d & 0xF); g_mouse_phase = 0; break;
        }
    }
}

// ---- IDT ----
struct IdtEntry { uint16_t base_lo; uint16_t sel; uint8_t zero; uint8_t flags; uint16_t base_hi; } __attribute__((packed));
struct IdtPtr { uint16_t limit; uint32_t base; } __attribute__((packed));
static IdtEntry idt[256];
static IdtPtr idt_ptr;
extern "C" void isr_stub();

static void init_idt() {
    for (int i = 0; i < 256; i++) {
        idt[i].base_lo = 0; idt[i].sel = 0x08; idt[i].zero = 0; idt[i].flags = 0x8E; idt[i].base_hi = 0;
    }
    for (int i = 0; i < 16; i++) {
        uintptr_t b = (uintptr_t)isr_stub;
        idt[0x20 + i].base_lo = b & 0xFFFF;
        idt[0x20 + i].base_hi = (b >> 16) & 0xFFFF;
    }
    idt_ptr.limit = 256 * 8 - 1;
    idt_ptr.base = (uint32_t)&idt;
    asm volatile("lidt %0" :: "m"(idt_ptr));
}

// ---- PIC / PIT / PS/2 ----
static void pic_remap(uint8_t off) {
    outb(0x20, 0x11); outb(0x21, off);
    outb(0x21, 0x04); outb(0x21, 0x01); outb(0x21, 0xFB);
    outb(0xA0, 0x11); outb(0xA1, off + 8);
    outb(0xA1, 0x02); outb(0xA1, 0x01); outb(0xA1, 0xEF);
    outb(0x21, 0x06);   // mask: timer+keyboard unmasked
}
static void pit_init() {
    outb(0x43, 0x36); outb(0x40, 0xFF); outb(0x40, 0xFF);
}
static void ps2_init() {
    inb(0x60);
    outb(0x64, 0xAE); outb(0x64, 0xA8);
    outb(0x64, 0xD4); outb(0x60, 0xEA);
    outb(0x64, 0xD4); outb(0x60, 0xF4);
    serial_puts("PS/2 keyboard+mouse ready\n");
}

// ---- GUI ----
static void gui_draw() {
    fill_screen(pack_color(30, 144, 255));
    draw_rect(80, 60, 700, 40, pack_color(20, 20, 40));
    draw_rect(80, 100, 700, 520, pack_color(240, 240, 245));
    draw_rect(120, 140, 80, 80, pack_color(255, 0, 0));
    draw_rect(220, 140, 80, 80, pack_color(0, 180, 0));
    draw_rect(320, 140, 80, 80, pack_color(0, 100, 255));
    serial_puts("GUI drawn\n");
}

// ---- Main ----
extern "C" void kernel_main() {
    serial_init();
    serial_puts("ZiumOS kernel\n");

    pic_remap(0x20);
    pit_init();
    init_idt();

    if (init_framebuffer()) {
        ps2_init();
        gui_draw();
        serial_puts("res="); serial_dec(g_fb_w); serial_putc('x'); serial_dec(g_fb_h); serial_putc('\n');
    } else {
        serial_puts("No framebuffer — GUI disabled\n");
        volatile uint16_t *vga = (uint16_t *)0xB8000;
        const char *msg = "ZiumOS: no framebuffer, GUI off.";
        for (int i = 0; msg[i]; i++) vga[i] = ((uint16_t)msg[i]) | 0x0700;
        for (;;) asm volatile("hlt");
    }
    asm volatile("sti");
    for (;;) asm volatile("hlt");
}
