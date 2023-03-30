/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include "local-include/reg.h"

const char *regs[] = {
  "$0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
  "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
  "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
  "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

void isa_reg_display() {
  printf("pc        0x%016"PRIx64 "     " "%"PRIu64 "\n", cpu.pc, cpu.pc);
  for(int idx = 0; idx < 32; ++idx) {
    printf("%-10s" "0x%016"PRIx64 "     " "%"PRIu64 "\n", regs[idx], cpu.gpr[idx], cpu.gpr[idx]);
  }
}

word_t isa_reg_str2val(const char *s, bool *success) {
  *success = true;

  if(strcmp(s, "pc") == 0)
    return cpu.pc;

  int idx;
  for(idx = 0; idx < 32; ++idx) {
    Log("%s   %s", regs[idx], s);
    if(strcmp(regs[idx], s) == 0)
      break;
  }

  if(idx == 32) {
    *success = false;
    return -1;
  }

  return cpu.gpr[idx];
}
