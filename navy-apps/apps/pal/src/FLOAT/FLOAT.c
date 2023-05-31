#include "FLOAT.h"
#include <stdint.h>
#include <assert.h>

FLOAT F_mul_F(FLOAT a, FLOAT b) {
  // assert(0);
  
  return (a * b) >> 16;
}

FLOAT F_div_F(FLOAT a, FLOAT b) {
  // assert(0);
  assert(b != 0);
  FLOAT x = Fabs(a);
  FLOAT y = Fabs(b);
  FLOAT ret = x / y;
 
  x = x % y;
  for (int i = 0; i < 16; i++){
    x = x << 1;
    ret = ret << 1;
    if (x >= y){
      x -= y;
      ret++;
    }
  }

  if (((a ^ b) & 0x80000000) == 0x80000000){
    ret = -ret;
  }
  return ret;
}

FLOAT f2F(float a) {
  /* You should figure out how to convert `a' into FLOAT without
   * introducing x87 floating point instructions. Else you can
   * not run this code in NEMU before implementing x87 floating
   * point instructions, which is contrary to our expectation.
   *
   * Hint: The bit representation of `a' is already on the
   * stack. How do you retrieve it to another variable without
   * performing arithmetic operations on it directly?
   */

  // assert(0);
  union turn_float{
    struct {
      uint32_t mantissa : 23;
      uint32_t exp : 8;
      uint32_t sign : 1;
    };
    uint32_t val;
  };

  union turn_float f;
  f.val = *((uint32_t*)((void*)&a));

  FLOAT ret;

  int exp = f.exp - 127;
  if (exp < 0){
    return 0;
  }else{
    if (exp - 7 >= 0){
      ret = (f.mantissa | 0x100000) << (exp - 7);
    }else{
      ret = (f.mantissa | 0x100000) >> (7 - exp);
    }
  }

  if (f.sign != 0){
    ret = -ret;
  }

  return ret;
}

FLOAT Fabs(FLOAT a) {
  // assert(0);
  if ((a & 0x80000000) == 0)
    return a;
  else
    return -a;
}

/* Functions below are already implemented */

FLOAT Fsqrt(FLOAT x) {
  FLOAT dt, t = int2F(2);

  do {
    dt = F_div_int((F_div_F(x, t) - t), 2);
    t += dt;
  } while(Fabs(dt) > f2F(1e-4));

  return t;
}

FLOAT Fpow(FLOAT x, FLOAT y) {
  /* we only compute x^0.333 */
  FLOAT t2, dt, t = int2F(2);

  do {
    t2 = F_mul_F(t, t);
    dt = (F_div_F(x, t2) - t) / 3;
    t += dt;
  } while(Fabs(dt) > f2F(1e-4));

  return t;
}
