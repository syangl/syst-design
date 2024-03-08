#include "common.h"

#define NAME(key) \
  [_KEY_##key] = #key,

static const char *keyname[256] __attribute__((used)) = {
  [_KEY_NONE] = "NONE",
  _KEYS(NAME)
};

extern int _screen_width();
extern int _screen_height();
extern void switch_game();

size_t events_read(void *buf, size_t len) { //note：按键事件
  int key = _read_key();
  bool down = false;
  if (key & 0x8000){
    key ^= 0x8000; // 获得按键位置
    down = true;
  }

  if (down && key == _KEY_F12){
    switch_game(); // 按键切换进程
  }

  if (key == _KEY_NONE){
    unsigned long t = _uptime(); // 时钟事件，表示系统启动后的时间
    sprintf(buf, "t %u\n", t);
  }else{
    Log("key = %d\n", key); // 按键时间
    sprintf(buf, "%s %s\n", down ? "kd" : "ku", keyname[key]);
  }

  return strlen(buf);
}

static char dispinfo[128] __attribute__((used));

void dispinfo_read(void *buf, off_t offset, size_t len) {
  strncpy(buf, dispinfo + offset, len);
}

void fb_write(const void *buf, off_t offset, size_t len) {// fb_write函数来实现显存中图像的绘制。
  int row, col;
  offset /= 4;
  col = offset % _screen_width();
  row = offset / _screen_width();
  _draw_rect((uint32_t *)buf, col, row, len / 4, 1);
}

void init_device() {
  _ioe_init();
  // TODO: print the string to array `dispinfo` with the format
  // described in the Navy-apps convention
  sprintf(dispinfo, "WIDTH:%d\nHEIGHT:%d\n", _screen_width(), _screen_height());
}

size_t serial_write(const void *buf, size_t offset, size_t len){
  uintptr_t i = 0;
  for (; len > 0; len--){
    _putc(((char *)buf)[i]);
    i++;
  }
  return i;
}