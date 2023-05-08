#include "common.h"

#define DEFAULT_ENTRY ((void *)0x4000000)

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
  // Log("loader filename: %s", filename);
  int fd = fs_open(filename, 0, 0);
  fs_read(fd, (void *)DEFAULT_ENTRY, fs_filesz(fd));
  Log("point");
  fs_close(fd);
  return (uintptr_t)DEFAULT_ENTRY;
}
