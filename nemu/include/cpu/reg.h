#ifndef __REG_H__
#define __REG_H__

#include "common.h"
#include "memory/mmu.h"

enum { R_EAX, R_ECX, R_EDX, R_EBX, R_ESP, R_EBP, R_ESI, R_EDI };
enum { R_AX, R_CX, R_DX, R_BX, R_SP, R_BP, R_SI, R_DI };
enum { R_AL, R_CL, R_DL, R_BL, R_AH, R_CH, R_DH, R_BH };

/**
 * note：
 * 保护模式和实模式：
 * 实模式：当时由于CPU的性能有限，一共只有20位地址线（所以地址空间只有1MB），以及8个16位的通用寄存器，以及4个16位的段寄存器。
 *        所以为了能够通过这些16位的寄存器去构成20位的主存地址，必须采取一种特殊的方式：
 *        物理地址 = 段基址<<4 + 段内偏移（段基址存放高16位）
 * 保护模式、GDT、GDTR、段选择子（分段机制）：
 *        保护模式：
 *            随着CPU的发展，CPU的地址线的个数也从原来的20根变为现在的32根，所以可以访问的内存空间也从1MB变为现在4GB，寄存器的位数也变为32位。
 *            这些段寄存器存放的不再是段基址，实模式下寻址方式不安全，在保护模式下需要加一些限制，把这些关于内存段的限制信息放在全局描述符表(GDT)里。
 *        GDT：
 *            全局描述符表中含有一个个表项，每一个表项称为段描述符。而段寄存器在保护模式下存放的便是相当于一个数组索引的东西（段选择子），这个索引可以找到对应的表项。
 *            段描述符存放了段基址、段界限、内存段类型属性等(比如是数据段还是代码段，注意一个段描述符只能用来定义一个内存段)
 *        
 *        GDTR：
 *            全局描述符表位于内存中，需要用专门的寄存器指向它，这个专门的寄存器便是GDTR(一个48位的寄存器)，专门用来存储GDT的内存地址及大小。
 *        
 *        段选择子：
 *            选择子16位，0~1位用来存储RPL（请求特权级，跟用户态和内核态相关，可表示 0、 1、 2、 3 四种特权级）。
 *            第2位是TI位（Table Indicator），用来指示选择子是在GDT还是LDT中索引描述符。TI为0表示在GDT中索引描述符，TI为1表示在LDT中索引描述符。
 *            选择子第3～15位是描述符的索引值，用此值在GDT中索引描述符。 
 * 
 *        局部描述符表LDT：
 *            是每一项任务运行时都要使用的描述符表。在多任务操作系统管理下，每个任务通常包含两部分：与其他任务共用的部分及本任务独有的部分。
 *            与其他任务共用部分的段描述符存储在全局描述符表GDT内；本任务独有部分的段描述符存储在本任务的局部描述符表LDT内。
 *            这样，每个任务都有一个局部描述符表LDT，而每个LDT表又是一个段，它也就必须有一个对应的LDT描述符。该LDT描述符存储在全局描述符表中。
 *            局部描述符表LDT中所存储的属于本任务的段描述符通常有 代码段描述符、数据段描述符、调用门描述符及任务门描述符等。   
 *        
 *        任务状态段TSS：
 *            为了提供多任务，80386使用了特殊的数据结构，主要有任务状态段TSS。TSS存于GDT，TR寄存器16位可见部分存放TSS选择子。TSS保存任务现场寄存器状态，用于实现任务切换。  
 *  
 * 虚拟8086模式：
 *        实际是开机时，微机开始运行于实模式，之后执行引导程序，准备保护模式初始化，初始化GDT、LDT等表，之后马上进入保护模式。
 *        保护模式有很大的虚拟内存空间，开辟出1MB内存空间，创建一个进程，模拟出一个8086，让原来旧的8086程序可以照样运行，这个模式就是虚拟8086模式。
 *        也就是Windows中执行cmd时打开的黑色窗口。
 * 
 * 门描述符：IF
 *    在计算机中，用门来表示一段程序的入口。和段描述符对比一下，段描述符中指向的是一片内存区域，而门描述符一般包含程序所在段的选择子和段内偏移量
 *    任务门
 *        任务门和任务状态段TSS是 Intel 处理器在硬件一级提供的任务切换机制，所以任务门需要和 TSS 配合在一起使用，在任务门中记录的是 TSS 选择子，偏移量未使用。
 *        任务门可以存在于全局描述符表 GDT、局部描述符表 LDT、中断描述符表 IDT 中。描述符中任务门的 type 值为二进制 0101。
 *        大多数操作系统包括 Linux 都未用 TSS 实现任务切换。
 *    中断门
 *        中断门包含了中断处理程序所在段的段选择子和段内偏移地址. 当通过此方式进入中断后, 
 *        标志寄存器eflags 中的IF位自动置0, 也就是在进入中断后, 自动把中断关闭，避免中断嵌套. 
 *        Linux 就是利用中断门实现的系统调用, 就是那个著名的 int 0x80 . 中断门只允许存在于 IDT 中。描述符中中断门的 type为二进制 1110 .
 *    陷阱门
 *        陷阱门和中断门非常相似，区别是由陷阱门进入中断后，标志寄存器 eflags 中的IF位不会自动置0. 
 *        陷阱门只允许存在于 IDT 中。描述符中陆阱门的 type 值为二进制 1111 .
 *    调用门
 *        调用门是提供给用户进程进入特权级的方式，其 DPL = 3 。
 *        调用门中记录例程的地址，它不能用int指令调用，只能用 call jmp 指令。调用门可以安装在 GDT LDT 中。描述符中调用门的 type 值为二进制 1100 。
 * 
 *    除调用门外，另外的任务门、中断门、陷阱门都可以存在于中断描述符表中
 * 
 * 特权级：
 *    在i386中, 存在0, 1, 2, 3四个特权级, 0特权级最高, 3特权级最低.
*/

/* TODO: Re-organize the `CPU_state' structure to match the register
 * encoding scheme in i386 instruction format. For example, if we
 * access cpu.gpr[3]._16, we will get the `bx' register; if we access
 * cpu.gpr[1]._8[1], we will get the 'ch' register. Hint: Use `union'.
 * For more details about the register encoding scheme, see i386 manual.
 */

typedef struct {
  // struct {
  //   uint32_t _32;
  //   uint16_t _16;
  //   uint8_t _8[2];
  // } gpr[8];
  union{ // note：8个32位的通用寄存器
    union {
      uint32_t _32;
      uint16_t _16;
      uint8_t _8[2];
    } gpr[8];
    struct {
      rtlreg_t eax, ecx, edx, ebx, esp, ebp, esi, edi;
    };
  };

  // note：eflags的实现（查阅x86 eflags 结构对照格式）
  union { 
    struct
    {
      uint8_t CF : 1; // 进位标志
      uint8_t X1 : 5; // X表示本实现暂时不用
      uint8_t ZF : 1; // 零标志位
      uint8_t SF : 1; // 符号标志
      uint8_t X2 : 1; // TF暂时不用
      uint8_t IF : 1; // 陷入标志，开启后每个指令执行后都产生一个调试异常，便于观察执行情况
      uint8_t X3 : 1; 
      uint8_t OF : 1; // 溢出标志
      uint32_t X4 : 20;
    } eflags;
    uint32_t flags;
  };

  /**
   * 控制寄存器是一些特殊的寄存器（cr0~cr4），它们可以控制CPU的一些重要特性。
   * CR0（开启分页机制）：
   *    第0位是保护允许位PE(Protedted Enable)，用于启动保护模式，如果PE位置1，则保护模式启动，如果PE=0，则在实模式下运行。
   *    第1位是监控协处理位MP(Moniter coprocessor)，它与第3位一起决定：当TS=1时操作码WAIT是否产生一个“协处理器不能使用”的出错信号。
   *    第3位是任务转换位(Task Switch)，当一个任务转换完成之后，自动将它置1。随着TS=1，就不能使用协处理器。
   *    第2位是模拟协处理器位 EM (Emulate coprocessor)，如果EM=1，则不能使用协处理器，如果EM=0，则允许使用协处理器。
   *    第4位是微处理器的扩展类型位 ET(Processor Extension Type)，其内保存着处理器扩展类型的信息，如果ET=0，则标识系统使用的是287协处理器，
   *    如果ET=1，则表示系统使用的是387浮点协处理器。
   *    第31位是分页允许位(Paging Enable)，它表示芯片上的分页部件是否允许工作。
   *    第16位是写保护位即WP位(486系列之后)，只要将这一位置0就可以禁用写保护，置1则可将其恢复。
   * CR1：
   *    是未定义的控制寄存器，供将来的处理器使用。
   * CR2
   *    是页故障线性地址寄存器，保存最后一次出现页故障的全32位线性地址。
   * CR3
   *    是页目录基址寄存器，保存页目录表的物理地址，页目录表总是放在以4KB为单位的存储器边界上，因此，它的地址的低12位总为0，不起作用，即使写上内容，也不会被理会。
   * CR4
   *    在Pentium系列（包括486的后期版本）处理器中才实现，它处理的事务包括诸如何时启用虚拟8086模式等
   * 
   * 这几个寄存器与分页机制密切相关，因此，在进程管理及虚拟内存管理中会涉及到这几个寄存器。
  */
  CR0 cr0;
  CR3 cr3;
  bool INTR; // 可屏蔽中断请求

  /**
   * 中断描述符表IDT
   *    中断描述符表（Interrupt Descriptor Table，IDT）将每个异常或中断向量分别与它们的处理过程联系起来。与GDT和LDT表类似，IDT也是由8字节长描述符组成的一个数组。   
   *    为了构成IDT表中的一个索引值，处理器把异常或中断的向量号乘以8。因为最多只有256个中断或异常向量，所以IDT无需包含多于256个描述符。
   *    IDT中可以含有少于256个描述符，因为只有可能发生的异常或中断才需要描述符。不过IDT中所有空描述符项应该设置其存在位（标志）为0
   *
   * 中断描述符表寄存器IDTR
   *    与GDTR类似，IDTR寄存器用于存放中断描述符表IDT的32位线性基地址和16位表长度值。
   *    指令LIDT和SIDT分别用于加载和保存IDTR寄存器的内容。在机器刚加电或处理器复位后，基地址被默认地设置为0，而长度值被设置成0xFFFF
   * 
   * LIDT/LGDT指令：
   *    将源操作数中的值加载到全局描述符表格寄存器 (GDTR) 或中断描述符表格寄存器 (IDTR)。
  */
  struct {
    uint32_t base; 
    uint16_t limit;
  }idtr;

  /**
   * CS是代码段寄存器，IP为指令指针寄存器，它们一起合作指向了CPU当前要读取的指令地址，可以理解为CS和IP结合，组成了PC寄存器。
   * 任何时刻，8086 CPU都会将CS:IP指向的指令作为下一条需要取出的执行指令。
   * 8086CPU中的计算公式为 (CS << 4)|IP，即CS左移4位，然后再加上IP
  */
  uint16_t cs;


/* Do NOT change the order of the GPRs' definitions. */

/* In NEMU, rtlreg_t is exactly uint32_t. This makes RTL instructions
   * in PA2 able to directly access these registers.
   */
// rtlreg_t eax, ecx, edx, ebx, esp, ebp, esi, edi;

  vaddr_t eip;
} CPU_state;

extern CPU_state cpu;

static inline int check_reg_index(int index) {
  assert(index >= 0 && index < 8);
  return index;
}

#define reg_l(index) (cpu.gpr[check_reg_index(index)]._32)
#define reg_w(index) (cpu.gpr[check_reg_index(index)]._16)
#define reg_b(index) (cpu.gpr[check_reg_index(index) & 0x3]._8[index >> 2])

extern const char* regsl[];
extern const char* regsw[];
extern const char* regsb[];

static inline const char* reg_name(int index, int width) { // 寄存器的名称，调试用不重要
  assert(index >= 0 && index < 8);
  switch (width) {
    case 4: return regsl[index]; // 32位如eax
    case 1: return regsb[index]; // 16位如ax
    case 2: return regsw[index]; // 8位如al
    default: assert(0);
  }
}

#endif
