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

    // NOTE(linlianhui): Add priority to token for eval.
    int priority;
} rules[] = {

    /* TODO: Add more rules.
     * Pay attention to the precedence level of different rules.
     */

    {" +", TK_NOTYPE, 0}, // spaces
    {"\\+", '+', 1},      // plus
    {"-", '-', 1},        // minus
    {"\\*", '*', 2},      // multiply
    {"/", '/', 2},        // divide
    {"\\(", '(', 0},      // left parentheses
    {"\\)", ')', 0},      // right parentheses

    {"==", TK_EQ, 0},          // equal
    {"[0-9]+", TK_DEC, 0}, // decimal
    {"0x", TK_HEX, 0},
    {"\\$\\w+", TK_REG, 0},
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

bool check_parentheses(int p, int q)
{
    return (tokens[p].type == '(' && tokens[q].type == ')');
}

int find_main_op(int p, int q)
{
    int cur_op = -1;
    for(int idx = p; idx <= q; ++idx) {
        switch (tokens[idx].type)
        {
        case '+':
        case '-':
            if(cur_op == -1 || tokens[cur_op].type == '+' || tokens[cur_op].type == '-')
            cur_op = idx;
            break;
        case '*':
        case '/':
            cur_op = idx;
            break;
        default:
            break;
        }
    }

    return cur_op;
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
            sscanf(tokens[p].str, "%lu", &num);
        }
        else if(tokens[p].type == TK_HEX) {
            sscanf(tokens[p].str, "%lx", &num);
        }
        else if(tokens[p].type == TK_REG) {
            bool success;
            num = isa_reg_str2val(tokens[p].str, &success);
        }

        return num;
    }
    else if (check_parentheses(p, q) == true)
    {
        /* The expression is surrounded by a matched pair of parentheses.
         * If that is the case, just throw away the parentheses.
         */
        return eval(p + 1, q - 1);
    }
    else
    {
        int32_t op = find_main_op(p, q);

        word_t val1 = eval(p, op - 1);
        word_t val2 = eval(op + 1, q);

        switch (tokens[op].type)
        {
        case '+':
            return val1 + val2;
        case '-':
            return val1 - val2;
        case '*':
            return val1 * val2;
        case '/':
            return val1 / val2;

        default:
            assert(0);
        }
    }
}

word_t expr(char *e, bool *success)
{
    if (!make_token(e))
    {
        *success = false;
        return 0;
    }

    word_t val = eval(0, nr_token - 1);

    return val;
}
