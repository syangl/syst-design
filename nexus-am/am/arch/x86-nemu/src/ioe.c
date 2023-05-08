#include <am.h>
#include <x86.h>

#define RTC_PORT 0x48   // Note that this is not standard
static unsigned long boot_time;

void _ioe_init() {
  boot_time = inl(RTC_PORT);
}

unsigned long _uptime() {
  return inl(RTC_PORT)-boot_time;
}

uint32_t* const fb = (uint32_t *)0x40000;

_Screen _screen = {
  .width  = 400,
  .height = 300,
};

extern void* memcpy(void *, const void *, int);

void _draw_rect(const uint32_t *pixels, int x, int y, int w, int h) {
  int i;
  // for (i = 0; i < _screen.width * _screen.height; i++) {
  //   fb[i] = i;
  // }
  for (i = 0; i < h; i++){
    memcpy(fb + (y + i) * _screen.width + x, pixels + i * w, w * 4);
  }
}

void _draw_sync() {
}

int _read_key() {
    uint32_t code = _KEY_NONE;
    // state judge
    if (inb(0x64)) {
      // read 0x60 data port
      code = inl(0x60);
    }
    return code;
}

int _screen_width(){
  return _screen.width;
}

int _screen_height(){
  return _screen.height;
}