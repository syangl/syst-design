#include "proc.h"

#define MAX_NR_PROC 4

static PCB pcb[MAX_NR_PROC];
static int nr_proc = 0;
PCB *current = NULL;

uintptr_t loader(_Protect *as, const char *filename);

void load_prog(const char *filename) {
  int i = nr_proc ++;
  _protect(&pcb[i].as);

  uintptr_t entry = loader(&pcb[i].as, filename);

  // TODO: remove the following three lines after you have implemented _umake()
  // _switch(&pcb[i].as);
  // current = &pcb[i];
  // // Log("before %x\n", entry);
  // ((void (*)(void))entry)();
  _Area stack;
  stack.start = pcb[i].stack;
  stack.end = stack.start + sizeof(pcb[i].stack);

  // Log("_umake\n");
  pcb[i].tf = _umake(&pcb[i].as, stack, stack, (void *)entry, NULL, NULL);
}

static int current_game = 0;
void switch_game(){
    current_game = 2 - current_game;
}

_RegSet* schedule(_RegSet *prev) {
  if (current != NULL){
    current->tf = prev;
  }else{
    current = &pcb[current_game];
  }

  static const int switch_max_times = 2000;
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