#include "nemu.h"
#include "device/mmio.h"

#define PMEM_SIZE (128 * 1024 * 1024)

#define pmem_rw(addr, type) *(type *)({\
    Assert(addr < PMEM_SIZE, "physical address(0x%08x) is out of bound", addr); \
    guest_to_host(addr); \
    })

uint8_t pmem[PMEM_SIZE];

/* Memory accessing interfaces */

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
      assert(0);
    }else{
      paddr_t paddr = page_translate(addr, false);
      return paddr_read(paddr, len);
    }
  }else
    return paddr_read(addr, len);
}

void vaddr_write(vaddr_t addr, int len, uint32_t data) {
  if (cpu.cr0.paging){
    if ((addr & 0xFFF) + len > 0x1000){
      assert(0);
    }else{
      paddr_t paddr = page_translate(addr, true);
      return paddr_write(paddr, len, data);
    }
  }else
    return paddr_write(addr, len, data);
}

paddr_t page_translate(vaddr_t vaddr,bool is_write) {
  vaddr_t CR3 = cpu.cr3.page_directory_base << 12;
  vaddr_t dir = (vaddr >> 22) * 4;
  // 取cr3的高20位与vaddr的高8位
  paddr_t pdAddr = CR3 + dir;
  PDE pd_val;
  assert(pd_val.present);

  vaddr_t t1 = pd_val.page_frame << 12;
  // 取第二个十位page
  vaddr_t t2 = ((vaddr >> 12) & 0x3FF) * 4;
  paddr_t ptAddr = t1 + t2;
  PTE pt_val;
  pt_val.val = paddr_read(ptAddr, 4);
  assert(pt_val.present);

  t1 = pt_val.page_frame << 12;
  // 取最后12位
  t2 = vaddr & 0xFFF;
  paddr_t paddr = t1 + t2;
  pd_val.accessed = 1;
  paddr_write(pdAddr, 4, pd_val.val);
  if ((pt_val.accessed == 0) || (pt_val.dirty == 0 && is_write)){
    pt_val.accessed = 1;
    pt_val.dirty = 1;
  }
  paddr_write(ptAddr, 4, pt_val.val);
  return paddr;
}
