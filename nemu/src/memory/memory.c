#include "nemu.h"
#include "device/mmio.h"
// #include "x86.h"

#define PMEM_SIZE (128 * 1024 * 1024)

#define PTXSHFT   12      // Offset of PTX in a linear address
#define PDXSHFT   22      // Offset of PDX in a linear address
#define PTE_ADDR(pte) ((uint32_t)(pte) & ~0xfff)
#define PDX(va)     (((uint32_t)(va) >> PDXSHFT) & 0x3ff)
#define PTX(va)     (((uint32_t)(va) >> PTXSHFT) & 0x3ff)
#define OFF(va)     ((uint32_t)(va) & 0xfff)

#define pmem_rw(addr, type) *(type *)({\
    Assert(addr < PMEM_SIZE, "physical address(0x%08x) is out of bound", addr); \
    guest_to_host(addr); \
    })

uint8_t pmem[PMEM_SIZE];

/* Memory accessing interfaces */

paddr_t page_translate(vaddr_t vaddr,bool is_write) {
  CR0 cr0 = cpu.cr0;
  if (cr0.paging && cr0.protect_enable){
    // Log("page_translate\n");
    CR3 crs = cpu.cr3;
    PDE *pgdirs = (PDE *)PTE_ADDR(crs.val);
    PDE pde = (PDE)paddr_read((uint32_t)(pgdirs + PDX(vaddr)), 4);
    PTE *ptable = (PTE *)PTE_ADDR(pde.val);

    PTE pte = (PTE)paddr_read((uint32_t)(ptable + PTX(vaddr)), 4);

    Assert(pte.present, "addr = 0x%x", vaddr);
    pde.accessed = 1;
    pte.accessed = 1;
    if (is_write){
      pte.dirty = 1;
    }
    paddr_t paddr = PTE_ADDR(pte.val) | OFF(vaddr);
    return paddr;
  }

  return vaddr;
}




uint32_t paddr_read(paddr_t addr, int len) {
  int mmio_id = is_mmio(addr);
  if (mmio_id != -1)
    return mmio_read(addr,  len,  mmio_id);
  else
    return pmem_rw(addr, uint32_t) & (~0u >> ((4 - len) << 3));
}

void paddr_write(paddr_t addr, int len, uint32_t data) {
  int mmio_id=is_mmio(addr);
  if (mmio_id != -1)
    return mmio_write(addr,  len,  data,  mmio_id);
  else
    memcpy(guest_to_host(addr), &data, len);
}

uint32_t vaddr_read(vaddr_t addr, int len){
  if (PTE_ADDR(addr) != PTE_ADDR(addr + len - 1)){
    int num1 = 0x1000 - OFF(addr);
    int num2 = len - num1;
    paddr_t paddr1 = page_translate(addr, false);
    paddr_t paddr2 = page_translate(addr + num1, false);
    uint32_t low = paddr_read(paddr1, num1);
    uint32_t high = paddr_read(paddr2, num2);
    uint32_t res = high << (num1 * 8) | low;
    return res;
  }else{
    paddr_t paddr = page_translate(addr, false);
    return paddr_read(paddr, len);
  }
}

void vaddr_write(vaddr_t addr, int len, uint32_t data){
  if (PTE_ADDR(addr) != PTE_ADDR(addr + len - 1)){
    if (PTE_ADDR(addr) != PTE_ADDR(addr + len - 1)){
      int num1 = 0x1000 - OFF(addr);
      int num2 = len - num1;
      paddr_t paddr1 = page_translate(addr, true);
      paddr_t paddr2 = page_translate(addr + num1, true);
      uint32_t low = data & (~0u >> ((4 - num1) << 3));
      uint32_t high = data >> ((4 - num2) << 3);
      paddr_write(paddr1, num1, low);
      paddr_write(paddr2, num2, high);
      return;
    }
  }else{
    paddr_t paddr = page_translate(addr, true);
    paddr_write(paddr, len, data);
  }
}
