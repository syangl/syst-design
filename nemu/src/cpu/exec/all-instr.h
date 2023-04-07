#include "cpu/exec.h"

make_EHelper(mov);

make_EHelper(operand_size);

make_EHelper(inv);
make_EHelper(nemu_trap);


// arith
make_EHelper(sub);
make_EHelper(sbb);
make_EHelper(add);
make_EHelper(div);
make_EHelper(idiv);
make_EHelper(adc);
make_EHelper(inc);
make_EHelper(dec);
make_EHelper(neg);
make_EHelper(mul);
make_EHelper(imul);
make_EHelper(imul1);
make_EHelper(imul2);
make_EHelper(cmp);

//control
make_EHelper(call);
make_EHelper(call_rm);
make_EHelper(ret);
make_EHelper(jmp);
make_EHelper(jmp_rm);
make_EHelper(jcc);

// data-mov
make_EHelper(mov);
make_EHelper(movzx);
make_EHelper(movsx);
make_EHelper(cltd);
make_EHelper(cwtl);
make_EHelper(push);
make_EHelper(pop);
make_EHelper(pusha);
make_EHelper(popa);
make_EHelper(leave);
make_EHelper(lea);