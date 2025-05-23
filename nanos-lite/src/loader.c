#include "common.h"
// #include "fs.h"
// #include "memory.h"

#define DEFAULT_ENTRY ((void *)0x8048000)

extern void ramdisk_read(void *buf, off_t offset, size_t len);
extern size_t get_ramdisk_size();
extern int fs_open(const char *pathname, int flags, int mode);
extern ssize_t fs_read(int fd, void *buf, size_t len);
extern ssize_t fs_write(int fd, const void *buf, size_t len);
extern off_t fs_lseek(int fd, size_t offset, int whence);
extern int fs_close(int fd);
extern size_t fs_filesz(int fd);
extern void* new_page(void);

uintptr_t loader(_Protect *as, const char *filename) {
  // TODO();
  // ramdisk_read(DEFAULT_ENTRY, 0, get_ramdisk_size());

  int fd = fs_open(filename, 0, 0);
  int size = fs_filesz(fd);
  int page_nums = size / PGSIZE;
  if (size % PGSIZE != 0){ // 最后一页通过取余来计数
    page_nums++;
  }

  void *pa = NULL;
  void *va = DEFAULT_ENTRY;
  for (int i = 0; i < page_nums; i++){
    pa = new_page(); 
    _map(as, va, pa); // 直接调用map实现va和pa的映射
    fs_read(fd, pa, PGSIZE); // pa就是fs_read的buf，读取disk到buf
    va += PGSIZE;
  }
  fs_close(fd);
  return (uintptr_t)DEFAULT_ENTRY;
}
