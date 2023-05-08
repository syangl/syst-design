#include "common.h"
#include "syscall.h"

#include "fs.h"

size_t sys_write(int fd, const void *buf, size_t len){
  size_t byteswritten;
  switch (fd){
    case 1:
    case 2:
      byteswritten = 0;
      while (len--){
        _putc(((char *)buf)[byteswritten]);
        byteswritten++;
      }
      return byteswritten;
      break;
    default:
      return 0;
  }
}

_RegSet* do_syscall(_RegSet *r) {
  // Log("do_syscall\n");
  uintptr_t a[4];
  a[0] = SYSCALL_ARG1(r);
  a[1] = SYSCALL_ARG2(r);
  a[2] = SYSCALL_ARG3(r);
  a[3] = SYSCALL_ARG4(r);

  switch (a[0]) {
    case SYS_none:
      SYSCALL_ARG1(r) = 1;
      break;
    case SYS_exit:
      _halt(SYSCALL_ARG2(r));
      break;
    case SYS_write:
      SYSCALL_ARG1(r) = sys_write(a[1], (void*)a[2], a[3]); //fs_write(a[1], (void*)a[2], a[3]);
      // printf("eax=%d",SYSCALL_ARG1(r));
      break;
    case SYS_brk:
      SYSCALL_ARG1(r) = 0;
      break;
    case SYS_open:
      SYSCALL_ARG1(r) = fs_open((char*)a[1], a[2], a[3]);
      break;
    case SYS_close:
      SYSCALL_ARG1(r) = fs_close(a[1]);
      break;
    case SYS_read:
      SYSCALL_ARG1(r) = fs_read(a[1], (void*)a[2], a[3]);
      break;
    case SYS_lseek:
      SYSCALL_ARG1(r) = (int)fs_lseek(a[1], a[2], a[3]);
      break;
    default: panic("Unhandled syscall ID = %d", a[0]);
  }

  return NULL;
}

