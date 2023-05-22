#include "common.h"

#define DEFAULT_ENTRY ((void *)0x8048000)

extern void ramdisk_read(void *buf, off_t offset, size_t len);
extern size_t get_ramdisk_size();
extern int fs_open(const char *pathname, int flags, int mode);
extern ssize_t fs_read(int fd, void *buf, size_t len);
extern ssize_t fs_write(int fd, const void *buf, size_t len);
extern off_t fs_lseek(int fd, size_t offset, int whence);
extern int fs_close(int fd);
extern size_t fs_filesz(int fd);

uintptr_t loader(_Protect *as, const char *filename) {
  // TODO();
  // ramdisk_read(DEFAULT_ENTRY, 0, get_ramdisk_size());

  int index = fs_open(filename, 0, 0);
  size_t length = fs_filesz(index);
  void *va;
  void *pa;
  int page_count = length / 4096 + 1; 

  for (int i = 0; i < page_count; i++){
    va = DEFAULT_ENTRY + 4096 * i;
    pa = new_page();
    _map(as, va, pa);
    fs_read(index, pa, 4096);
  }

  fs_close(index);
  return (uintptr_t)DEFAULT_ENTRY;
}
