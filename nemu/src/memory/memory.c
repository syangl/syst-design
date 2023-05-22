#include "nemu.h"
#include "device/mmio.h"

#define PMEM_SIZE (128 * 1024 * 1024)

#define PTE_ADDR(pte) ((uint32_t)(pte) & ~0xfff)
#define PDX(va) (((uint32_t)(va) >> 22) & 0x3ff)
#define PTX(va) (((uint32_t)(va) >> 12) & 0x3ff)

#define pmem_rw(addr, type) *(type *)({\
    Assert(addr < PMEM_SIZE, "physical address(0x%08x) is out of bound", addr); \
    guest_to_host(addr); \
    })

uint8_t pmem[PMEM_SIZE];

/* Memory accessing interfaces */

paddr_t page_translate(vaddr_t vaddr,bool is_write) {
  CR0 cr0 = cpu.cr0;
  if (cr0.paging && cr0.protect_enable){
    CR3 crs = cpu.cr3;
    PDE *pgdirs = (PDE *)PTE_ADDR(crs.val);
    PDE pde = (PDE)paddr_read((uint32_t)(pgdirs + PDX(vaddr)), 4);
    PTE *ptable = (PTE *)PTE_ADDR(pde.val);

    PTE pte = (PTE)paddr_read((uint32_t)(ptable + PTX(vaddr)), 4);

    Assert(pte.present, "addr=0x%x", vaddr);
    pde.accessed = 1;
    pte.accessed = 1;
    if (is_write){
      pte.dirty = 1;
    }
    paddr_t paddr = PTE_ADDR(pte.val) | ((uint32_t)(vaddr) & 0xfff);
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

uint32_t vaddr_read(vaddr_t addr, int len) {
  if (cpu.cr0.paging){
    if ((addr & 0xFFF) + len > 0x1000){
      int len1, len2;
      len1 = 0x1000 - (addr & 0xfff);
      len2 = len - len1;
      paddr_t addr1 = page_translate(addr, false);
      uint32_t data1 = paddr_read(addr1, len1);
      paddr_t addr2 = page_translate(addr + len1, false);
      uint32_t data2 = paddr_read(addr2, len2);
      uint32_t data = (data2 << (len1 << 3)) + data1;
      return data;
    }else{
      paddr_t paddr = page_translate(addr, false);
      return paddr_read(paddr, len);
    }
  }
  else
    return paddr_read(addr, len);
}

void vaddr_write(vaddr_t addr, int len, uint32_t data) {
  if (cpu.cr0.paging){
    if ((addr & 0xFFF) + len > 0x1000){
      int len1, len2;
      len1 = 0x1000 - (addr & 0xfff); 
      len2 = len - len1;              
      paddr_t addr1 = page_translate(addr, true); 
      paddr_write(addr1, len1, data);             
      uint32_t data1 = data >> (len1 << 3);
      paddr_t addr2 = page_translate(addr + len1, true);
      paddr_write(addr2, len2, data1);
    }
    else
    {
      paddr_t paddr = page_translate(addr, true);
      return paddr_write(paddr, len, data);
    }
  }
  else
    return paddr_write(addr, len, data);
}

