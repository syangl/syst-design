#include "fs.h"

extern size_t ramdisk_read(void *buf, size_t offset, size_t len);
extern size_t ramdisk_write(const void *buf, size_t offset, size_t len);
extern size_t dispinfo_read(void *buf, size_t offset, size_t len);
extern size_t fb_write(const void *buf, size_t offset, size_t len);
extern size_t events_read(void *buf, size_t len);
extern size_t serial_write(const void *buf, size_t offset, size_t len);
extern int _screen_width();
extern int _screen_height();

typedef struct {
  char *name;
  size_t size;
  off_t disk_offset;
  off_t open_offset;
} Finfo;

enum {FD_STDIN, FD_STDOUT, FD_STDERR, FD_FB, FD_EVENTS, FD_DISPINFO, FD_NORMAL};

/* This is the information about all files in disk. */
static Finfo file_table[] __attribute__((used)) = {
  {"stdin (note that this is not the actual stdin)", 0, 0, 0},
  {"stdout (note that this is not the actual stdout)", 0, 0, 0},
  {"stderr (note that this is not the actual stderr)", 0, 0, 0},
  [FD_FB] = {"/dev/fb", 0, 0, 0},
  [FD_EVENTS] = {"/dev/events", 0, 0, 0},
  [FD_DISPINFO] = {"/proc/dispinfo", 128, 0, 0},
#include "files.h"
};

#define NR_FILES (sizeof(file_table) / sizeof(file_table[0]))

void init_fs() {
  // TODO: initialize the size of /dev/fb
  file_table[FD_FB].size = _screen_width() * _screen_height() * 4;
}

int fs_open(const char *pathname, int flags, int mode){
  for (int i = 0; i < NR_FILES; i++){
    // Log("file_table name: %s, pathname: %s", file_table[i].name, pathname);   
    if (strcmp(file_table[i].name, pathname) == 0){
      file_table[i].open_offset = 0;
      return i;
    }
  }
  assert(0);
  return -1;
}

ssize_t fs_read(int fd, void *buf, size_t len){
  size_t fs_size = fs_filesz(fd);
  switch (fd){
    case FD_STDIN:
    case FD_STDOUT: 
    case FD_STDERR:
    case FD_FB:
      break;
    case FD_EVENTS:
      len = events_read(buf, len);
      break;
    case FD_DISPINFO:
      if (file_table[fd].open_offset >= file_table[fd].size)
        return 0;
      if (file_table[fd].open_offset + len > fs_size)
        len = file_table[fd].size - file_table[fd].open_offset;
      
      dispinfo_read(buf, file_table[fd].open_offset, len);
      file_table[fd].open_offset += len;
      break;
    default:
      if (file_table[fd].open_offset >= fs_size)
        return 0;
      if (file_table[fd].open_offset + len > fs_size)
        len = fs_size - file_table[fd].open_offset;

      ramdisk_read(buf, file_table[fd].disk_offset + file_table[fd].open_offset, len);
      file_table[fd].open_offset += len;
      break;
  }

  return len;
}
ssize_t fs_write(int fd, const void *buf, size_t len){
  // Log("fs_write\n");
  size_t fs_size = fs_filesz(fd);
  switch (fd){
    case FD_STDIN:
      break;
    case FD_STDOUT:
    case FD_STDERR:
      serial_write(buf, 0, len);
      break;
    case FD_FB:
      if (file_table[fd].open_offset >= fs_size)
        return 0;
      if (file_table[fd].open_offset + len > fs_size)
        len = fs_size - file_table[fd].open_offset;
      
      fb_write(buf, file_table[fd].open_offset, len);
      file_table[fd].open_offset += len;
      break;
    case FD_EVENTS:
    case FD_DISPINFO:
      break;
    default:
      if (file_table[fd].open_offset >= fs_size)
        return 0;
      if (file_table[fd].open_offset + len > fs_size)
        len = fs_size - file_table[fd].open_offset;

      ramdisk_write(buf, file_table[fd].disk_offset + file_table[fd].open_offset, len);
      file_table[fd].open_offset += len;
      break;
  }

  return len;
}

size_t fs_filesz(int fd){
  return file_table[fd].size;
}

off_t fs_lseek(int fd, off_t offset, int whence){
  size_t result = -1;
  switch (whence){
    case SEEK_SET:
      if (offset >= 0 && offset <= fs_filesz(fd)){
        file_table[fd].open_offset = offset;
        result = file_table[fd].open_offset = offset;
      }
      break;
    case SEEK_CUR:
      if ((offset + file_table[fd].open_offset >= 0) &&
          (offset + file_table[fd].open_offset <= fs_filesz(fd)))
      {
        file_table[fd].open_offset += offset;
        result = file_table[fd].open_offset;
      }
      break;
    case SEEK_END:
      file_table[fd].open_offset = fs_filesz(fd) + offset;
      result = file_table[fd].open_offset;
      break;
  }

  return result;
}

int fs_close(int fd){
  return 0;
}



