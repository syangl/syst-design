#include "common.h"

#define NAME(key) \
  [_KEY_##key] = #key,

static const char *keyname[256] __attribute__((used)) = {
  [_KEY_NONE] = "NONE",
  _KEYS(NAME)
};

extern int _screen_width();
extern int _screen_height();

size_t events_read(void *buf, size_t len) {
  int key = _read_key();
  bool down = false;
  if (key & 0x8000){
    key ^= 0x8000;
    down = true;
  }

  if (key == _KEY_NONE){
    unsigned long t = _uptime();
    sprintf(buf, "Time %u\n", t);
  }else{
    Log("key = %d\n", key);
    sprintf(buf, "%s %s\n", down ? "kd" : "ku", keyname[key]);
  }

  return strlen(buf);
}

static char dispinfo[128] __attribute__((used));

void dispinfo_read(void *buf, off_t offset, size_t len) {
  strncpy(buf, dispinfo + offset, len);
  // Log("point2");
}

void fb_write(const void *buf, off_t offset, size_t len) {
  int row, col;
  offset /= 4;
  col = offset % _screen_width();
  row = offset / _screen_width();
  Log("init_fs screen width: %d, screen height: %d", _screen_width(), _screen_height());
  _draw_rect((uint32_t *)buf, col, row, len / 4, 1);
}

void init_device() {
  _ioe_init();
  // Log("init_fs screen width: %d, screen height: %d", _screen_width(), _screen_height());
  // TODO: print the string to array `dispinfo` with the format
  // described in the Navy-apps convention
  sprintf(dispinfo, "Width:%d\tHeight:%d\n", _screen_width(), _screen_height());
}

size_t serial_write(const void *buf, size_t offset, size_t len){
  uintptr_t i = 0;
  for (; len > 0; len--){
    _putc(((char *)buf)[i]);
    i++;
  }
  return i;
}