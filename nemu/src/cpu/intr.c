#include "cpu/exec.h"
#include "memory/mmu.h"

void raise_intr(uint8_t NO, vaddr_t ret_addr) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * That is, use ``NO'' to index the IDT.
   */

  // TODO();
  vaddr_t gate_addr = cpu.idtr.base + 8 * NO;

  if (cpu.idtr.limit < 0){
    assert(0);
  }

  rtl_push(&cpu.flags);
  cpu.eflags.IF = 0;
  t0 = cpu.cs;
  rtl_push(&t0);
  rtl_push(&ret_addr);

  uint32_t high,low;
  low = vaddr_read(gate_addr,4) & 0xffff;
  high = vaddr_read(gate_addr + 4, 4) & 0xffff0000;

  decoding.jmp_eip = high | low;
  decoding.is_jmp = true;
}

void dev_raise_intr() {
  cpu.INTR = true;
}
