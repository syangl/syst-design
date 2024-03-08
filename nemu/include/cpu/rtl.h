#ifndef __RTL_H__
#define __RTL_H__

#include "nemu.h"

/**
 * note：
 * RTL基本指令：
 *    可使用这些操作将复杂指令分解成更简单的操作。
 *    NEMU 中的 RTL 寄存器：
 *      x86的八个通用寄存器(在 include/cpu/reg.h 中定义)
 *      id_src, id_src2 和 id_dest 中的访存地址 addr 和操作数内容 val (在 include/cpu/decode.h 中定义). 从概念上看, 它们分别与MAR和MDR有异曲同工之妙
 *      临时寄存器 t0~t3 和 at (在 src/cpu/decode/decode.c 中定义)
 * 
 * 再回顾一遍模拟器最核心的执行逻辑，应用程序执行的指令被模拟器译码分析，调用执行函数，执行函数通过RTL实现，这样运行在真机上的模拟器完成了相应的指令功能。
 * 
 * asm volatile用法：
 *    语法格式：asm volatile (assembly code : output operands : input operands : clobbered registers);
 *             assembly code: 包含实际汇编指令的字符串。
 *             output operands: 指定输出的操作数，即汇编代码的结果将写入这些变量。
 *             input operands: 指定输入的操作数，即嵌入汇编代码所需的输入数据。
 *             clobbered registers: 指定在嵌入的汇编代码中被修改的寄存器。
 *    举个例子：
 *    --------------------------------------------------------------------------------------
 *    |  int main() {                                                                      |
 *    |    int input = 10;                                                                 | 
 *    |    int result;                                                                     | 
 *    |    asm volatile (                                                                  |
 *    |      "movl %1, %0;" // 将输入值复制到输出值（从左往右，%0、%1对应后面的=r和r）         |  
 *    |      : "=r" (result) // 输出操作数，'r'表示通用寄存器，'='表示输出，没有'='表示输入     |
 *    |      : "r" (input)   // 输入操作数                                                  |
 *    |    );                                                                              |
 *    |                                                                                    | 
 *    |    printf("Result: %d\n", result); // result是10                                   |     
 *    |    return 0;                                                                       | 
 *    |  }                                                                                 | 
 *    --------------------------------------------------------------------------------------
*/

extern rtlreg_t t0, t1, t2, t3;
extern const rtlreg_t tzero;

/* RTL basic instructions */

static inline void rtl_li(rtlreg_t* dest, uint32_t imm) { // 立即数读入
  *dest = imm;
}

#define c_add(a, b) ((a) + (b))
#define c_sub(a, b) ((a) - (b))
#define c_and(a, b) ((a) & (b))
#define c_or(a, b)  ((a) | (b))
#define c_xor(a, b) ((a) ^ (b))
#define c_shl(a, b) ((a) << (b))
#define c_shr(a, b) ((a) >> (b))
#define c_sar(a, b) ((int32_t)(a) >> (b))
#define c_slt(a, b) ((int32_t)(a) < (int32_t)(b))
#define c_sltu(a, b) ((a) < (b))

// concat宏，函数名称是rtl_{name} ,对src1和src2进行c_{name}的逻辑运算，结果存入dest
#define make_rtl_arith_logic(name) \
  static inline void concat(rtl_, name) (rtlreg_t* dest, const rtlreg_t* src1, const rtlreg_t* src2) { \
    *dest = concat(c_, name) (*src1, *src2); \
  } \
  static inline void concat3(rtl_, name, i) (rtlreg_t* dest, const rtlreg_t* src1, int imm) { \
    *dest = concat(c_, name) (*src1, imm); \
  }


make_rtl_arith_logic(add)
make_rtl_arith_logic(sub)
make_rtl_arith_logic(and)
make_rtl_arith_logic(or)
make_rtl_arith_logic(xor)
make_rtl_arith_logic(shl)
make_rtl_arith_logic(shr)
make_rtl_arith_logic(sar)
make_rtl_arith_logic(slt)
make_rtl_arith_logic(sltu)

static inline void rtl_mul(rtlreg_t* dest_hi, rtlreg_t* dest_lo, const rtlreg_t* src1, const rtlreg_t* src2) {
  // 执行乘法操作，将输入操作数 *src1 和 *src2 相乘，并将结果的高位和低位分别存储在 *dest_hi 和 *dest_lo 所指向的内存位置。
  // 需要注意的是，mul 指令将两个操作数相乘的结果存储在edx:eax 寄存器对中，其中 edx 包含结果的高位，而 eax 包含结果的低位。
  asm volatile("mul %3" : "=d"(*dest_hi), "=a"(*dest_lo) : "a"(*src1), "r"(*src2)); // d是edx，a是eax
}

static inline void rtl_imul(rtlreg_t* dest_hi, rtlreg_t* dest_lo, const rtlreg_t* src1, const rtlreg_t* src2) {
  asm volatile("imul %3" : "=d"(*dest_hi), "=a"(*dest_lo) : "a"(*src1), "r"(*src2));
}

static inline void rtl_div(rtlreg_t* q, rtlreg_t* r, const rtlreg_t* src1_hi, const rtlreg_t* src1_lo, const rtlreg_t* src2) {
  asm volatile("div %4" : "=a"(*q), "=d"(*r) : "d"(*src1_hi), "a"(*src1_lo), "r"(*src2));
}

static inline void rtl_idiv(rtlreg_t* q, rtlreg_t* r, const rtlreg_t* src1_hi, const rtlreg_t* src1_lo, const rtlreg_t* src2) {
  asm volatile("idiv %4" : "=a"(*q), "=d"(*r) : "d"(*src1_hi), "a"(*src1_lo), "r"(*src2));
}

static inline void rtl_lm(rtlreg_t *dest, const rtlreg_t* addr, int len) {
  *dest = vaddr_read(*addr, len);
}

static inline void rtl_sm(rtlreg_t* addr, int len, const rtlreg_t* src1) {
  vaddr_write(*addr, len, *src1);
}

static inline void rtl_lr_b(rtlreg_t* dest, int r) {
  *dest = reg_b(r);
}

static inline void rtl_lr_w(rtlreg_t* dest, int r) {
  *dest = reg_w(r);
}

static inline void rtl_lr_l(rtlreg_t* dest, int r) {
  *dest = reg_l(r);
}

static inline void rtl_sr_b(int r, const rtlreg_t* src1) {
  reg_b(r) = *src1;
}

static inline void rtl_sr_w(int r, const rtlreg_t* src1) {
  reg_w(r) = *src1;
}

static inline void rtl_sr_l(int r, const rtlreg_t* src1) {
  reg_l(r) = *src1;
}

/* RTL psuedo instructions */

static inline void rtl_lr(rtlreg_t* dest, int r, int width) {
  switch (width) {
    case 4: rtl_lr_l(dest, r); return;
    case 1: rtl_lr_b(dest, r); return;
    case 2: rtl_lr_w(dest, r); return;
    default: assert(0);
  }
}

static inline void rtl_sr(int r, int width, const rtlreg_t* src1) {
  switch (width) {
    case 4: rtl_sr_l(r, src1); return;
    case 1: rtl_sr_b(r, src1); return;
    case 2: rtl_sr_w(r, src1); return;
    default: assert(0);
  }
}

// 置位和获取eflags值
#define make_rtl_setget_eflags(f) \
  static inline void concat(rtl_set_, f) (const rtlreg_t* src) { \
    /*TODO();*/cpu.eflags.f = (*src); \
  } \
  static inline void concat(rtl_get_, f) (rtlreg_t* dest) { \
    /*TODO();*/*dest = cpu.eflags.f; \
  }

make_rtl_setget_eflags(CF)
make_rtl_setget_eflags(OF)
make_rtl_setget_eflags(ZF)
make_rtl_setget_eflags(SF)

static inline void rtl_mv(rtlreg_t* dest, const rtlreg_t *src1) {
  // dest <- src1
  // TODO();
  *dest = (*src1); 
}

static inline void rtl_not(rtlreg_t* dest) {
  // dest <- ~dest
  // TODO();
  *dest = ~(*dest);
}
// signal extension
static inline void rtl_sext(rtlreg_t* dest, const rtlreg_t* src1, int width) {
  // dest <- signext(src1[(width * 8 - 1) .. 0])
  // TODO();
  if(width == 4)
    *dest = *src1;
  else if(width == 2)
  {
    uint16_t result = *src1;
    int16_t tmp = result;
    *dest = tmp;
  }
  else if(width == 1)
  {
    uint8_t result = *src1;
    int8_t tmp = result;
    *dest = tmp;
  }
}

static inline void rtl_push(const rtlreg_t* src1) {
  // esp <- esp - 4
  // M[esp] <- src1
  //TODO();
  cpu.esp -= 4;
  vaddr_write((cpu.esp), 4, (*src1));
}

static inline void rtl_pop(rtlreg_t* dest) {
  // dest <- M[esp]
  // esp <- esp + 4
  // TODO();
  *dest = vaddr_read(cpu.esp, 4);
  cpu.esp += 4;
}

static inline void rtl_eq0(rtlreg_t* dest, const rtlreg_t* src1) {
  // dest <- (src1 == 0 ? 1 : 0)
  // TODO();
  *dest = ((*src1) == 0 ? 1 : 0);
}

static inline void rtl_eqi(rtlreg_t* dest, const rtlreg_t* src1, int imm) {
  // dest <- (src1 == imm ? 1 : 0)
  // TODO();
  *dest = ((*src1) == imm ? 1 : 0);
}

static inline void rtl_neq0(rtlreg_t* dest, const rtlreg_t* src1) {
  // dest <- (src1 != 0 ? 1 : 0)
  // TODO();
  *dest = ((*src1) != 0 ? 1 : 0);
}
// Most Significant Bit
static inline void rtl_msb(rtlreg_t* dest, const rtlreg_t* src1, int width) {
  // dest <- src1[width * 8 - 1]
  // TODO();
  *dest = (uint32_t)*src1 >> (width * 8 - 1); // 如32位，逻辑右移31位得到最高有效位
}

static inline void rtl_update_ZF(const rtlreg_t* result, int width) {
  // eflags.ZF <- is_zero(result[width * 8 - 1 .. 0])
  // TODO();
  cpu.eflags.ZF = ((*result & (0xFFFFFFFF >> ((4 - width) * 8))) == 0); // 右移4-w位得到和w位宽相同的全1掩码，和result与运算得到w宽的结果，判断其是否为0
}

static inline void rtl_update_SF(const rtlreg_t* result, int width) {
  // eflags.SF <- is_sign(result[width * 8 - 1 .. 0])
  // TODO();
  cpu.eflags.SF = (((*result & (0xFFFFFFFF >> ((4 - width) * 8))) & (1 << (width * 8 - 1))) != 0); // 获取result[width*8-1...0]的符号位并置位（最高位）
}

static inline void rtl_update_ZFSF(const rtlreg_t* result, int width) {
  rtl_update_ZF(result, width);
  rtl_update_SF(result, width);
}

#endif
