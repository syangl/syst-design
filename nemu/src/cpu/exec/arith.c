#include "cpu/exec.h"
#define INT_MAX 2147483647
#define INT_MIN (-INT_MAX - 1)

make_EHelper(add) {// add dest src ：add指令将两个操作数相加，且将相加后的结果保存到第一个操作数中。
  // TODO();
  // sext
  rtl_sext(&t1, &id_dest->val, id_dest->width);
	rtl_sext(&t2, &id_src->val, id_src->width);
  // add
	rtl_add(&t0, &t1, &t2); // t0 <- t1 + t2
  // set CF
	t3 = (t0 < t1); // t0和t1是无符号数，结果反而小于操作数，说明无符号溢出，CF置1
	rtl_set_CF(&t3);
  // set OF (+ + gets -, - - gets +) 
  // 首先检查t1和t2是否有相同的符号，然后检查结果t0的符号是否与t1（或t2，因为此时t1和t2的符号相同）不同，
  // 如果这两个条件同时满足，那么就意味着发生了溢出，应该设置OF标志。
	// t3 = ((((int32_t)(t1) >= 0) ^ (((int32_t)(t2) >= 0 ))) && (((int32_t)(t0) < 0) ^ (((int32_t)(t2) >= 0 )))); 
  t3 = ((int32_t)t1 < 0 == (int32_t)t2 < 0) && ((int32_t)t0 < 0 != (int32_t)t2 < 0);
	rtl_set_OF(&t3);
  // update ZFSF
	rtl_update_ZFSF(&t0, 4);
  // dest <- t0
	operand_write(id_dest, &t0); // 结果写出到reg或者mem；id_dest是decoding结构，记录了译码的dest，src1，src2，type，opcode，seq_eip等信息
  print_asm_template2(add); // debug
}

make_EHelper(sub) {
  // TODO();
  rtl_sext(&t1, &id_dest->val, id_dest->width);
	rtl_sext(&t2, &id_src->val, id_src->width);
	rtl_sub(&t0, &t1, &t2);

	t3 = (t0 > t1);
	rtl_set_CF(&t3);
	// (- + gets +, + - gets -)
  t3 = ((((int32_t)(t1) < 0) == (((int32_t)(t2) >> 31) == 0)) && (((int32_t)(t0) < 0) != ((int32_t)(t1) < 0)));
	rtl_set_OF(&t3);
	rtl_update_ZFSF(&t0, 4);
	
  operand_write(id_dest, &t0);
  print_asm_template2(sub);
}

make_EHelper(cmp) {
  // TODO();
	rtl_sext(&t1, &id_dest->val, id_dest->width);
	rtl_sext(&t2, &id_src->val, id_src->width);

	rtl_sub(&t0, &t1, &t2);
	t3 = (t0 > t1);
	rtl_set_CF(&t3);
	t3 = ((((int32_t)(t1) < 0) == (((int32_t)(t2) >> 31) == 0)) && (((int32_t)(t0) < 0) != ((int32_t)(t1) < 0)));
	rtl_set_OF(&t3);
	rtl_update_ZFSF(&t0, 4);
  print_asm_template2(cmp);
}

make_EHelper(inc) {
  // TODO();
	rtl_addi(&t2, &id_dest->val, 1);
	operand_write(id_dest, &t2);
	rtl_update_ZFSF(&t2, id_dest->width);

  // 考虑是否从最大正数溢出到负数
  t0 = ((int32_t)(id_dest->val) == INT_MAX) && ((int32_t)t2 < 0);
	rtl_set_OF(&t0);
  print_asm_template1(inc);
}

make_EHelper(dec) {
  // TODO();
  rtl_subi(&t2, &id_dest->val, 1);
	operand_write(id_dest, &t2);
	rtl_update_ZFSF(&t2, id_dest->width);

  // 考虑是否从最小负数正数溢出到正数
  t0 = ((int32_t)(id_dest->val) == INT_MIN) && ((int32_t)t2 > 0);
	rtl_set_OF(&t0);
  print_asm_template1(dec);
}

// neg (negation) means a = 0 - a
make_EHelper(neg) {
  // TODO();
	rtl_mv(&t0, &id_dest->val);
	rtl_not(&t0);
	rtl_addi(&t0, &t0, 1); // 补码，全位求反再加1得相反数
	operand_write(id_dest, &t0);

	t1 = (id_dest->val != 0);
	rtl_set_CF(&t1);

	rtl_update_ZFSF(&t0, id_dest->width);
  // 如果-a没有溢出，不妨设-a为负数，那么a和-a的补码按位异或，结果的MSB必为1，因为a变成-a如果只是全位求反，按位异或结果为全1
  // 而实际上a变成-a全位求反又加了1，所以按位异或的结果从最低位开始的一些位会为0，
  // 而只要-a不溢出，则-a的补码表示最高位还是1（溢出时为0），那么按位异或结果的MSB必为1（溢出时为0）
	rtl_xor(&t1, &t0, &id_dest->val);
	rtl_not(&t1); // 取反让结果的MSB为0
	rtl_msb(&t1, &t1, id_dest->width);// 只取（~MSB）就可以判断溢出
	rtl_set_OF(&t1);
  print_asm_template1(neg);
}

/** 
 * 带进位加法指令，不用实现
 * adc 是带进位加法指令，它利用了CF位上记录的进位值
 * 指令格式： adc 操作对象1,操作对象2
 * 功能： 操作对象1 = 操作对象1 + 操作对象2 + CF
 */
make_EHelper(adc) {
  rtl_add(&t2, &id_dest->val, &id_src->val);
  rtl_sltu(&t3, &t2, &id_dest->val);
  rtl_get_CF(&t1);
  rtl_add(&t2, &t2, &t1);
  operand_write(id_dest, &t2);

  rtl_update_ZFSF(&t2, id_dest->width);

  rtl_sltu(&t0, &t2, &id_dest->val);
  rtl_or(&t0, &t3, &t0);
  rtl_set_CF(&t0);

  rtl_xor(&t0, &id_dest->val, &id_src->val);
  rtl_not(&t0);
  rtl_xor(&t1, &id_dest->val, &t2);
  rtl_and(&t0, &t0, &t1);
  rtl_msb(&t0, &t0, id_dest->width);
  rtl_set_OF(&t0);

  print_asm_template2(adc);
}

/** 
 * sbb是带借位减法指令，它利用了CF位上记录的借位值。
 * 指令格式：sbb 操作对象1,操作对象2
 * 功能：操作对象1 = 操作对象1 - 操作对象2 - CF
*/
make_EHelper(sbb) {
  rtl_sub(&t2, &id_dest->val, &id_src->val);
  rtl_sltu(&t3, &id_dest->val, &t2);
  rtl_get_CF(&t1);
  rtl_sub(&t2, &t2, &t1);
  operand_write(id_dest, &t2);

  rtl_update_ZFSF(&t2, id_dest->width);

  rtl_sltu(&t0, &id_dest->val, &t2);
  rtl_or(&t0, &t3, &t0);
  rtl_set_CF(&t0);

  rtl_xor(&t0, &id_dest->val, &id_src->val);
  rtl_xor(&t1, &id_dest->val, &t2);
  rtl_and(&t0, &t0, &t1);
  rtl_msb(&t0, &t0, id_dest->width);
  rtl_set_OF(&t0);

  print_asm_template2(sbb);
}

make_EHelper(mul) {
  rtl_lr(&t0, R_EAX, id_dest->width);
  rtl_mul(&t0, &t1, &id_dest->val, &t0);

  switch (id_dest->width) {
    case 1:
      rtl_sr_w(R_AX, &t1);
      break;
    case 2:
      rtl_sr_w(R_AX, &t1);
      rtl_shri(&t1, &t1, 16);
      rtl_sr_w(R_DX, &t1);
      break;
    case 4:
      rtl_sr_l(R_EDX, &t0);
      rtl_sr_l(R_EAX, &t1);
      break;
    default: assert(0);
  }

  print_asm_template1(mul);
}

// imul with one operand
make_EHelper(imul1) {
  rtl_lr(&t0, R_EAX, id_dest->width);
  rtl_imul(&t0, &t1, &id_dest->val, &t0);

  switch (id_dest->width) {
    case 1:
      rtl_sr_w(R_AX, &t1);
      break;
    case 2:
      rtl_sr_w(R_AX, &t1);
      rtl_shri(&t1, &t1, 16);
      rtl_sr_w(R_DX, &t1);
      break;
    case 4:
      rtl_sr_l(R_EDX, &t0);
      rtl_sr_l(R_EAX, &t1);
      break;
    default: assert(0);
  }

  print_asm_template1(imul);
}

// imul with two operands
make_EHelper(imul2) {
  rtl_sext(&id_src->val, &id_src->val, id_src->width);
  rtl_sext(&id_dest->val, &id_dest->val, id_dest->width);

  rtl_imul(&t0, &t1, &id_dest->val, &id_src->val);
  operand_write(id_dest, &t1);

  print_asm_template2(imul);
}

// imul with three operands
make_EHelper(imul3) {
  rtl_sext(&id_src->val, &id_src->val, id_src->width);
  rtl_sext(&id_src2->val, &id_src2->val, id_src->width);
  rtl_sext(&id_dest->val, &id_dest->val, id_dest->width);

  rtl_imul(&t0, &t1, &id_src2->val, &id_src->val);
  operand_write(id_dest, &t1);

  print_asm_template3(imul);
}

make_EHelper(div) {
  switch (id_dest->width) {
    case 1:
      rtl_li(&t1, 0);
      rtl_lr_w(&t0, R_AX);
      break;
    case 2:
      rtl_lr_w(&t0, R_AX);
      rtl_lr_w(&t1, R_DX);
      rtl_shli(&t1, &t1, 16);
      rtl_or(&t0, &t0, &t1);
      rtl_li(&t1, 0);
      break;
    case 4:
      rtl_lr_l(&t0, R_EAX);
      rtl_lr_l(&t1, R_EDX);
      break;
    default: assert(0);
  }

  rtl_div(&t2, &t3, &t1, &t0, &id_dest->val);

  rtl_sr(R_EAX, id_dest->width, &t2);
  if (id_dest->width == 1) {
    rtl_sr_b(R_AH, &t3);
  }
  else {
    rtl_sr(R_EDX, id_dest->width, &t3);
  }

  print_asm_template1(div);
}

make_EHelper(idiv) {
  rtl_sext(&id_dest->val, &id_dest->val, id_dest->width);

  switch (id_dest->width) {
    case 1:
      rtl_lr_w(&t0, R_AX);
      rtl_sext(&t0, &t0, 2);
      rtl_msb(&t1, &t0, 4);
      rtl_sub(&t1, &tzero, &t1);
      break;
    case 2:
      rtl_lr_w(&t0, R_AX);
      rtl_lr_w(&t1, R_DX);
      rtl_shli(&t1, &t1, 16);
      rtl_or(&t0, &t0, &t1);
      rtl_msb(&t1, &t0, 4);
      rtl_sub(&t1, &tzero, &t1);
      break;
    case 4:
      rtl_lr_l(&t0, R_EAX);
      rtl_lr_l(&t1, R_EDX);
      break;
    default: assert(0);
  }

  rtl_idiv(&t2, &t3, &t1, &t0, &id_dest->val);

  rtl_sr(R_EAX, id_dest->width, &t2);
  if (id_dest->width == 1) {
    rtl_sr_b(R_AH, &t3);
  }
  else {
    rtl_sr(R_EDX, id_dest->width, &t3);
  }

  print_asm_template1(idiv);
}
