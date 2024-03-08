#include "common.h"
#include "syscall.h"

#include "fs.h"

/** note：系统调用
 * main.c => load_prog 
*/

extern int mm_brk(uint32_t new_brk);

// 检查fd的值，如果fd是 1 或 2（分别代表 stdout 和 stderr），则将buf为首地址的len 字节输出到串口（使用 _putc() 即可）。
// fd是文件描述符，是打开文件表（文件描述表）的索引，用于指代被打开的文件。规定系统刚刚启动的时候，0是标准输入，1是标准输出，2是标准错误。
// 这意味着如果此时去打开一个新的文件，它的文件描述符会是3，再打开一个文件文件描述符就是4
// 串口，原名叫做串行接口。串行口不同于并行口之处在于它的数据和控制信息是一位接一位地传送，传送距离较并行口更长，因此若要进行较长距离的通信时，应使用串行口。
size_t sys_write(int fd, const void *buf, size_t len){ 
  size_t byteswritten;
  switch (fd){
    case 1:
    case 2:
      byteswritten = 0;
      while (len--){
        _putc(((char *)buf)[byteswritten]); // 将字符写入标准输出stdout
        byteswritten++;
      }
      return byteswritten;
      break;
    default:
      return 0;
  }
}

_RegSet* do_syscall(_RegSet *r) { // 系统调用
  // Log("step in do_syscall\n");
  uintptr_t a[4];
  a[0] = SYSCALL_ARG1(r); // SYSCALL_ARG1(r) 即 tf->eax
  a[1] = SYSCALL_ARG2(r); // SYSCALL_ARG2(r) 即 tf->ebx
  a[2] = SYSCALL_ARG3(r); // SYSCALL_ARG3(r) 即 tf->ecx
  a[3] = SYSCALL_ARG4(r); // SYSCALL_ARG4(r) 即 tf->edx

  switch (a[0]) {
    case SYS_none:
      SYSCALL_ARG1(r) = 1;
      break;
    case SYS_exit:
      _halt(SYSCALL_ARG2(r));
      break;
    case SYS_write:
      SYSCALL_ARG1(r) = fs_write(a[1], (void*)a[2], a[3]); // sys_write(a[1], (void*)a[2], a[3]); 
      break;
    case SYS_brk:
      SYSCALL_ARG1(r) = mm_brk(a[1]);
      break;
    case SYS_open:
      // Log("fs_open pathname: %s", (char*)a[1]);
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

