#include "cpu/exec.h"

make_EHelper(test) {
  // TODO();
  rtl_and(&t1, &id_dest->val, &id_src->val);
  t0 = 0;
  rtl_set_OF(&t0);
  rtl_set_CF(&t0);
  rtl_update_ZFSF(&t1, id_dest->width);
  print_asm_template2(test);
}

make_EHelper(and) {
  // TODO();
  rtl_and(&t1, &id_dest->val, &id_src->val);
  operand_write(id_dest, &t1);
  t0 = 0;
  rtl_set_OF(&t0);
  rtl_set_CF(&t0);
  rtl_update_ZFSF(&t1, id_dest->width);
  print_asm_template2(and);
}

make_EHelper(xor) {
  // TODO();
  rtl_xor(&t1, &id_dest->val, &id_src->val);
  operand_write(id_dest, &t1);
  t0 = 0;
  rtl_set_OF(&t0);
  rtl_set_CF(&t0);
  rtl_update_ZFSF(&t1, id_dest->width);
  print_asm_template2(xor);
}

make_EHelper(or) {
  // TODO();
  rtl_or(&t2, &id_dest->val, &id_src->val);
  operand_write(id_dest, &t2);
  rtl_update_ZFSF(&t2, id_dest->width);
  t0 = 0;
  rtl_set_OF(&t0);
  rtl_set_CF(&t0);
  print_asm_template2(or);
}

make_EHelper(sar) {
  // TODO();
  // unnecessary to update CF and OF in NEMU
  rtl_sar(&t2, &id_dest->val, &id_src->val);
  operand_write(id_dest, &t2);
  rtl_update_ZFSF(&t2, id_dest->width);
  print_asm_template2(sar);
}

make_EHelper(shl) {
  // TODO();
  // unnecessary to update CF and OF in NEMU
  rtl_shl(&t2, &id_dest->val, &id_src->val);
  operand_write(id_dest, &t2);
  rtl_update_ZFSF(&t2, id_dest->width);
  print_asm_template2(shl);
}

make_EHelper(shr) {
  // TODO();
  // unnecessary to update CF and OF in NEMU
  rtl_shr(&t2, &id_dest->val, &id_src->val);
  operand_write(id_dest, &t2);
  rtl_update_ZFSF(&t2, id_dest->width);
  print_asm_template2(shr);
}

make_EHelper(setcc) {
  uint8_t subcode = decoding.opcode & 0xf;
  rtl_setcc(&t2, subcode);
  operand_write(id_dest, &t2);

  print_asm("set%s %s", get_cc_name(subcode), id_dest->str);
}

make_EHelper(not) {
  // TODO();
  rtl_not(&id_dest->val);
  operand_write(id_dest, &id_dest->val);
  print_asm_template1(not);
}


// found microbench needs rol, so implements it
make_EHelper(rol) {
   // rol times
   for (t0=0; t0 < id_src->val; t0++) {
     // get highest bit to shift, then shl, finally add highest to lowest postion
     rtl_shri(&t1, &id_dest->val, id_dest->width*8-1);
     rtl_shli(&t2, &id_dest->val, 1);
     id_dest->val= t1 + t2;
   }
   rtl_set_CF(&t1);
   operand_write(id_dest, &id_dest->val);
   print_asm_template2(rol);
}   


// pa5 found following instruments aren't implemented.
make_EHelper(shld){
  rtl_shl(&t0, &id_dest->val, &id_src->val);
  rtl_li(&t2, id_src2->width);
  rtl_shli(&t2, &t2, 3);
  rtl_subi(&t2, &t2, id_src->val);
  rtl_shr(&t2, &id_src2->val, &t2);
  rtl_or(&t0, &t0, &t2);
  operand_write(id_dest, &t0);
  rtl_update_ZFSF(&t0, id_dest->width);
  print_asm_template3(shld);
}

make_EHelper(shrd){
  rtl_shr(&t0, &id_dest->val, &id_src->val);
  rtl_li(&t2, id_src2->width);
  rtl_shli(&t2, &t2, 3);
  rtl_subi(&t2, &t2, id_src->val);
  rtl_shl(&t2, &id_src2->val, &t2);
  rtl_or(&t0, &t0, &t2);
  operand_write(id_dest, &t0);
  rtl_update_ZFSF(&t0, id_dest->width);
  print_asm_template3(shrd);
}