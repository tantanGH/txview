#include <stdint.h>
#include "fpcall.h"

//
//  FP#1: ASK68K set FP kana-kanji henkan mode
//
int32_t fpcall_set_fp_mode(int32_t mode) {

  register int32_t reg_d0 asm ("d0") = mode;

  asm volatile (
    "move.l   d0,-(sp)\n"
    "move.l   #1,-(sp)\n"
    "dc.w     __KNJCTRL\n"
    "addq.l   #8,sp\n"
    : "+r"  (reg_d0)    // output (&input) operand
    :
    :
  );

  return reg_d0;
}

//
//  FP#2: ASK68K get FP kana-kanji henkan mode
//
int32_t fpcall_get_fp_mode() {

  register int32_t reg_d0 asm ("d0") = 0;

  asm volatile (
    "move.l   #2,-(sp)\n"
    "dc.w     __KNJCTRL\n"
    "addq.l   #4,sp\n"
    : "=r"  (reg_d0)    // output only operand
    :
    :
  );

  return reg_d0;
}
