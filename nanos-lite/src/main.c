#include "common.h"

/* Uncomment these macros to enable corresponding functionality. */
#define HAS_ASYE
#define HAS_PTE

void init_mm(void);
void init_ramdisk(void);
void init_device(void);
void init_irq(void);
void init_fs(void);
uint32_t loader(_Protect *, const char *);
extern void load_prog(const char *filename);

int main() {
#ifdef HAS_PTE
  init_mm();
#endif

  Log("'Hello World!' from Nanos-lite");
  Log("Build time: %s, %s", __TIME__, __DATE__);

  init_ramdisk();

  init_device();

#ifdef HAS_ASYE
  Log("Initializing interrupt/exception handler...");
  init_irq();
#endif

  init_fs();

  // uint32_t entry = loader(NULL, "/bin/bmptest");
  // uint32_t entry = loader(NULL, "/bin/events");
  // uint32_t entry = loader(NULL, "/bin/pal");
  // ((void (*)(void))entry)();
  
  // load_prog("/bin/dummy");
  load_prog("/bin/pal"); // 0
  load_prog("/bin/hello"); // 1
  load_prog("/bin/videotest"); // 2

  _trap();
  
  panic("Should not reach here");
}
