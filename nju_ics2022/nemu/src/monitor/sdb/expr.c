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

enum
{
    TK_NOTYPE = 256,
    TK_EQ = 1,  // ==

    // NOTE(linlianhui): Add element type
    TK_NEQ = 2, // !=
    TK_AND = 3, // &&
    TK_DEREF = 4,   // *
    TK_OCT = 5,
    TK_DEC = 6,
    TK_HEX = 7,
    TK_REG = 8,
};

static struct rule
{
    const char *regex;
    int token_type;

} rules[] = {

    /* TODO: Add more rules.
     * Pay attention to the precedence level of different rules.
     */

    {" +", TK_NOTYPE}, // spaces
    {"\\+", '+'},      // plus
    {"-", '-'},        // minus
    {"\\*", '*'},      // multiply
    {"/", '/'},        // divide
    {"\\(", '('},      // left parentheses
    {"\\)", ')'},      // right parentheses

    {"==", TK_EQ},          // equal
    {"^0x[0-9]+", TK_HEX},
    {"[0-9]+", TK_DEC}, // decimal
    {"\\$\\w+", TK_REG},
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex()
{
    int i;
    char error_msg[128];
    int ret;

    for (i = 0; i < NR_REGEX; i++)
    {
        ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
        if (ret != 0)
        {
            regerror(ret, &re[i], error_msg, 128);
            panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
        }
    }
}

typedef struct token
{
    int type;
    char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used)) = 0;

static bool make_token(char *e)
{
    int position = 0;
    int i;
    regmatch_t pmatch;

    nr_token = 0;

    while (e[position] != '\0')
    {
        /* Try all rules one by one. */
        for (i = 0; i < NR_REGEX; i++)
        {
            if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0)
            {
                char *substr_start = e + position;
                int substr_len = pmatch.rm_eo;

                Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
                    i, rules[i].regex, position, substr_len, substr_len, substr_start);

                position += substr_len;

                /* TODO: Now a new token is recognized with rules[i]. Add codes
                 * to record the token in the array `tokens'. For certain types
                 * of tokens, some extra actions should be performed.
                 */

                switch (rules[i].token_type)
                {
                case '+':
                case '-':
                case '*':
                case '/':
                case '(':
                case ')':
                    tokens[nr_token].type = rules[i].token_type;
                    nr_token++;
                    break;
                case TK_DEC:
                case TK_REG:
                    assert(nr_token < 32);
                    tokens[nr_token].type = rules[i].token_type;
                    strncpy(tokens[nr_token].str, substr_start, substr_len);
                    nr_token++;
                default:
                    break;
                }

                break;
            }
        }

        if (i == NR_REGEX)
        {
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
    return (op_type == '+' ||
            op_type == '-' ||
            op_type == '*' ||
            op_type == '/' ||
            op_type == TK_DEREF);
}

/**
 * Get the operator's priority, 
 * The smaller value is, the higher priority are.
*/
int get_priority(int op_type) {
    switch (op_type)
    {
    case -1:
        return -1;
    case TK_DEREF:
        return 1;
    case '*':
    case '/':
        return 2;
    case '+':
    case '-':
        return 3;
    default:
        Log("Don't support this operator %c", op_type);
        Log("Op Type: %d", op_type);
        assert(0);
    }
}

bool check_parentheses(int p, int q)
{
    return (tokens[p].type == '(' && tokens[q].type == ')');
}

int find_main_op(int p, int q)
{
    int main_op_type = -1;
    int main_op_idx = -1;
    int idx = p;
    while(idx <= q) {
        int op_type = tokens[idx].type;
        Log("idx: %d   Op type: %d", idx, op_type);
        if(op_type == '(') {
            int cnt = 0;
            while(idx <= q && cnt != 0) {
                op_type = tokens[idx].type;
                if(op_type == '(') cnt++;
                if(op_type == ')') cnt--;
                idx++;
            }
        }
        else {
            if(is_operator(op_type) && 
               get_priority(main_op_type) < get_priority(op_type)) {
                main_op_idx = idx;
                main_op_type = op_type;
                Log("main op idx: %d", main_op_idx);
            }
            idx++;
        }
    }
    return main_op_idx;
}

word_t eval(int p, int q)
{
    if (p > q)
    {
        return -1;
    }
    else if (p == q)
    {
        word_t num;

        if(tokens[p].type == TK_DEC) {
            sscanf(tokens[p].str, FMT_WORD_DEC, &num);
        }
        else if(tokens[p].type == TK_HEX) {
            sscanf(tokens[p].str, "0x%lx", &num);
        }
        else if(tokens[p].type == TK_REG) {
            bool success;
            if(strcmp(tokens[p].str, "$0") == 0)
                num = isa_reg_str2val(tokens[p].str, &success);
            else
                num = isa_reg_str2val(tokens[p].str + 1, &success);
        }

        return num;
    }
    else if (check_parentheses(p, q) == true)
    {
        return eval(p + 1, q - 1);
    }
    else
    {
        int32_t op = find_main_op(p, q);
        word_t val1 = eval(op + 1, q);

        if(tokens[op].type == TK_DEREF) {
            Log("val: "FMT_WORD, val1);
            word_t* value = (word_t*)guest_to_host(val1);
            return *value;
        }

        word_t val2 = eval(p, op - 1);

        switch (tokens[op].type)
        {
        case '+':
            return val2 + val1;
        case '-':
            return val2 - val1;
        case '*':
            return val2 * val1;
        case '/':
            return val2 / val1;
        default:
            assert(0);
        }
    }
}

bool is_deref(int p) {
    assert(tokens[p].type == '*');

    return (p == 0 || 
            tokens[p - 1].type == '(' || 
            tokens[p - 1].type == '+' || 
            tokens[p - 1].type == '-' ||
            tokens[p - 1].type == '*' ||
            tokens[p - 1].type == '/');
}

word_t expr(char *e, bool *success)
{
    // Make token
    if (!make_token(e))
    {
        *success = false;
        return 0;
    }

    // Check dereference tokens
    for (int i = 0; i < nr_token; i ++) {
        if (tokens[i].type == '*' && is_deref(i)) {
            tokens[i].type = TK_DEREF;
        }
    }

    // Evaluate expression
    word_t val = eval(0, nr_token - 1);

    return val;
}
