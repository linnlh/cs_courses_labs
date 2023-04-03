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
#include <memory/paddr.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

enum {
  TK_NOTYPE = 256, // Space

  TK_DEREF = 1,  // *
  TK_ADD   = 2,  // +
  TK_SUB   = 3,  // -
  TK_MUL   = 4,  // *
  TK_DIV   = 5,  // /
  TK_EQ    = 6,  // ==
  TK_NEQ   = 7,  // !=
  TK_AND   = 8,  // &&
  TK_BIN   = 9,  // Bin. number, start with 0b
  TK_DEC   = 10, // Dec. number
  TK_HEX   = 11, // Hex. number, start with 0x
  TK_REG   = 12, // Register, start with $
  TK_LPAREN = 13, // (
  TK_RPAREN = 14, // )
};

static struct rule {
  const char *regex;
  int token_type;

} rules[] = {

    /* TODO: Add more rules.
     * Pay attention to the precedence level of different rules.
     */

    {" +", TK_NOTYPE},
    {"\\+", TK_ADD},
    {"-", TK_SUB},
    {"\\*", TK_MUL},  // It also can be TK_DEREF. Should be checked later.s
    {"/", TK_DIV},
    {"==", TK_EQ},
    {"!=", TK_NEQ},
    {"&&", TK_AND},
    {"\\(", TK_LPAREN},
    {"\\)", TK_RPAREN},
    {"0b[01]+", TK_BIN},
    {"[0-9]+", TK_DEC},
    {"0x[0-9a-f]+", TK_HEX},
    {"\\$\\w+", TK_REG},
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used)) = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        if(rules[i].token_type != TK_NOTYPE)  assert(nr_token < 32);

        switch (rules[i].token_type) {
          case TK_ADD:
          case TK_SUB:
          case TK_MUL:
          case TK_DIV:
          case TK_EQ:
          case TK_NEQ:
          case TK_AND:
          case TK_LPAREN:
          case TK_RPAREN:
            tokens[nr_token].type = rules[i].token_type;
            nr_token++;
            break;
          case TK_BIN:
          case TK_DEC:
          case TK_HEX:
            tokens[nr_token].type = rules[i].token_type;
            strncpy(tokens[nr_token].str, substr_start, substr_len);
            nr_token++;
            break;
          case TK_REG:
            tokens[nr_token].type = rules[i].token_type;
            if(substr_len != 2 || strncmp(substr_start, "$0", 2) == 0)
              substr_start += 1;

            strncpy(tokens[nr_token].str, substr_start, substr_len);
            nr_token++;
            break;
          default:
            break;
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

/**
 * Check if op_type is a operator.
 */
bool is_operator(int op_type) {
  return (1 <= op_type && op_type <= 8);
}

/**
 * Check if '*' is dereference operator.
*/
bool is_deref(int p) {
  assert(tokens[p].type == TK_MUL);

  return (p == 0 || 
          tokens[p - 1].type == '(' ||
          (2 <= tokens[p - 1].type && tokens[p - 1].type <= 8));
}

/**
 * Get the operator's priority,
 * The smaller value is, the higher priority are.
 */
int get_priority(int op_type) {
  switch (op_type) {
    case -1:  return -1;
    case TK_DEREF:  return 1;
    case TK_MUL:
    case TK_DIV:  return 2;
    case TK_ADD:
    case TK_SUB:  return 3;
    case TK_EQ:
    case TK_NEQ:  return 4;
    case TK_AND:  return 5;
    default:
      Log("Don't support this operator %c", op_type);
      Log("Op Type: %d", op_type);
      assert(0);
  }
}

bool check_parentheses(int p, int q) {
  return (tokens[p].type == TK_LPAREN && tokens[q].type == TK_RPAREN);
}

int find_main_op(int p, int q) {
  int main_op_type = -1;
  int main_op_idx = -1;
  int idx = p;
  while (idx <= q) {
    int op_type = tokens[idx].type;
    if (op_type == TK_LPAREN) {
      int cnt = 1;
      while(idx <= q && cnt != 0) {
        idx++;
        if(tokens[idx].type == TK_LPAREN) cnt++;
        if(tokens[idx].type == TK_RPAREN) cnt--;
      }
      if(cnt != 0)
        return -1;
    }
    else if (is_operator(op_type) && get_priority(main_op_type) < get_priority(op_type)) {
      main_op_idx = idx;
      main_op_type = op_type;
    }
    
    idx++;
  }
  Log("p: %d  q: %d  main op idx: %d  main op: %d", p, q, main_op_idx, main_op_type);
  return main_op_idx;
}

word_t eval(int p, int q, bool * success) {
  if(success == false)
    return -1;

  if (p > q) {
    Log("Evaluation failure.");
    *success = false;
    return -1;
  }
  else if (p == q) {
    word_t num;

    switch(tokens[p].type) {
      case TK_BIN:  Assert(0, "Unimplement.");
      case TK_DEC:
        sscanf(tokens[p].str, WORD_DEC, &num);
        break;
      case TK_HEX:
        sscanf(tokens[p].str, WORD_HEX, &num);
        break;
      case TK_REG:
        num = isa_reg_str2val(tokens[p].str, success);
        break;
      default:
        Log("Unknow operand.");
        *success = false;
    }

    return num;
  }
  else if (check_parentheses(p, q) == true) {
    return eval(p + 1, q - 1, success);
  }
  else {
    int op = find_main_op(p, q);
    if(op == -1) {
      *success = false;
      return -1;
    }

    word_t val1 = eval(op + 1, q, success);
    if (tokens[op].type == TK_DEREF) {
      word_t * value = (word_t *)guest_to_host(val1);
      return *value;
    }

    word_t val2 = eval(p, op - 1, success);
    switch (tokens[op].type) {
      case TK_ADD:
        return val2 + val1;
      case TK_SUB:
        return val2 - val1;
      case TK_MUL:
        return val2 * val1;
      case TK_DIV:
        return val2 / val1;
      case TK_EQ:
        return val1 == val2;
      case TK_NEQ:
        return val1 != val2;
      case TK_AND:
        return val1 && val2;
      default:
        Log("Unknow operand.");
        *success = false;
        return -1;
    }
  }
}

word_t expr(char *e, bool *success) {
  // Make token
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  // Check dereference tokens
  for (int i = 0; i < nr_token; i++) {
    if (tokens[i].type == TK_MUL && is_deref(i)) {
      tokens[i].type = TK_DEREF;
    }
  }

  *success = true;
  // Evaluate expression
  word_t val = eval(0, nr_token - 1, success);

  return val;
}
