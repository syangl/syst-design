#include "proc.h"

#define MAX_NR_PROC 4

static PCB pcb[MAX_NR_PROC];
static int nr_proc = 0;
PCB *current = NULL;

uintptr_t loader(_Protect *as, const char *filename);

void load_prog(const char *filename) {
  int i = nr_proc ++;
  _protect(&pcb[i].as); // 调用_protect函数将内核页表的va->pa映射拷贝到用户进程的页表，因为所有用户进程都共享内核空间，所以内核态页表的内容也需要共享，缺少的话会发生缺页。然后loader加载程序。

  uintptr_t entry = loader(&pcb[i].as, filename);

  _Area stack;
  stack.start = pcb[i].stack;
  stack.end = stack.start + sizeof(pcb[i].stack);

  // Log("_umake\n");
  /**
   * 不用实现，理解流程
   * 实现进程调度，需要用umake来创建进程的上下文，保存在trap frame中。
   * 当需要进程切换时，就通过硬件产生一个外部中断（这里是时钟中断IRQ_TIME），封装成事件并进行分发。
   * 中断处理时，将旧进程的tf压栈，使用schedule函数得到新进程的tf地址，asm_trap让栈顶指针指向它，中断返回时（恢复现场）就完成了进程切换。
   * asm_trap会将通用寄存器的内容以及错误码errror code，异常号irq（就是上面说的中断号NO），eflags，cs，eip会打包成一个数据结构，称为trap frame（陷阱帧）。
   * trap frame完整地记录了中断发生时的状态，依靠它来恢复现场。
   * 使用trap frame保存现场之后，调用irq_handle（nexus-am\am\arch\x86-nemu\src\asye.c）把异常封装为事件ev.event，根据异常号irq进行分发（其实就是处理不同情况）
  */
  pcb[i].tf = _umake(&pcb[i].as, stack, stack, (void *)entry, NULL, NULL);
}

static int current_game = 0;
void switch_game(){
    current_game = 2 - current_game;
}


// 两个游戏进程切换
_RegSet* schedule(_RegSet *prev) {// prev是tf，系统调用处理时钟中断事件传参tf调用schedule，如果按键F12改变了current_game，就切换了，如果没有就是2000时间步修改一次current
  if (current != NULL){
    current->tf = prev;
  }else{
    current = &pcb[current_game];
  }

  static const int switch_max_times = 2000; // 2000时间步换一次
  static int schedule_times = 0;
  if (current == &pcb[current_game]){
    schedule_times++;
  }else{
    current = &pcb[current_game];
  }
  if (schedule_times == switch_max_times){
    current = &pcb[1];
    schedule_times = 0;
  }
  // current = (current == &pcb[0]? &pcb[1] : &pcb[0]);
  _switch(&current->as);
  return current->tf;
}