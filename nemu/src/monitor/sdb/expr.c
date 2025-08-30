/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
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

enum {
  TK_NOTYPE = 256, TK_EQ,
  TK_NUMBER, TK_NEGATIVE

  /* TODO: Add more token types */

};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"==", TK_EQ},        // equal
  {"\\-", '-'},         // sub
  {"\\(", '('},           // left parenthesis
  {"\\)", ')'},           // right parenthesis
  {"\\*", '*'},         // multiply
  {"/", '/'},           // division
  {"(0u?|[1-9][0-9]*u?)", TK_NUMBER}, // decimal integer
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

  for (i = 0; i < NR_REGEX; i ++) {
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

#define MAX_TOKENS 65536
static Token tokens[MAX_TOKENS] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
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

        switch (rules[i].token_type) {
          case TK_NOTYPE:
            break;
          case TK_NUMBER:
            if (nr_token >= MAX_TOKENS) {
              printf("too many tokens\n");
              return false;
            }
            //Assert(nr_token < 32, "The tokens array has insufficient storage space.");
            // Assert(nr_token < 65536, "The tokens array has insufficient storage space.");
            Assert(substr_len < 32, "token is too long");
            strncpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token].type = rules[i].token_type;
            tokens[nr_token].str[substr_len] = '\0';
            nr_token++;
            break;
          default:
            if (nr_token >= MAX_TOKENS) {
              printf("too many tokens\n");
              return false;
            }
            //Assert(nr_token < 32, "The tokens array has insufficient storage space.");
            // Assert(nr_token < 65536, "The tokens array has insufficient storage space.");
            tokens[nr_token].type = rules[i].token_type;
            nr_token++;
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

word_t eval(int p, int q, bool *success);
 
void markNegative() {
    int i;
 
    for (i = 0 ; i < nr_token; i++){
        if (tokens[i].type == TK_NUMBER) continue;
        if (tokens[i].type == (int)('-') && (i == 0 || 
            (tokens[i - 1].type != TK_NUMBER && tokens[i - 1].type != (int)(')')))){
                tokens[i].type = TK_NEGATIVE;
        }
    }
}

word_t expr(char *e, bool *success) {
  bool evalSuccess;
  word_t exprAns;

  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  markNegative();
  evalSuccess = true;
  exprAns = eval(0, nr_token - 1, &evalSuccess);
  *success = evalSuccess;
  if (evalSuccess){
    return exprAns;
  }
  return 0;
}

/*检查表达式的左右括号是否匹配*/
bool check_expr_parentheses(int p, int q){
  int i;
  int stack_top = -1;
  bool flag = true;

  for (i = p; i <= q ; i++){
    if (tokens[i].type == (int)('(')){
        stack_top++;
    } else if (tokens[i].type == (int)(')')){    
      if (stack_top >= 0){
        stack_top--;          
      } else {
        flag = false;
        break;
      }
    }
  }
  if (stack_top < 0 && flag == true){
    flag = true;
  } else {
    flag = false;
  }
  return flag;
}

bool check_parentheses(int p, int q){
    /*判断表达式是否被一对匹配的括号包围着*/
  if (tokens[p].type == (int)('(') && tokens[q].type == (int)(')')){
    return check_expr_parentheses(p + 1, q - 1);
  }
  return false;
}

int priority(int op_type){
    switch (op_type)
    {
    case '(':
    case ')':
        return 0;
        break;
    case '+':
    case '-':
        return 1;
        break;
    case '*':
    case '/':
        return 2;
        break;
    case TK_NEGATIVE:
        return 3;
        break;
    default:
        Assert(0, "No corresponding operator found");
    }
}

struct stack_node{
    int idx;
    int type;
};

word_t eval(int p, int q, bool *success){
  // printf("expr:");
  // for (int i = p; i <= q; i++){
  //   if (tokens[i].type == TK_NUMBER){
  //       printf("%s", tokens[i].str);
  //   } else {
  //       printf("%c", (char)tokens[i].type);
  //   }
  // }
  // printf("\n");

  word_t number;

  if (p > q){
    *success = false;
    return 0;
  } else if (p == q){
    /* TODO(human): 处理单个数字token的情况
     * 任务：将tokens[p].str转换为word_t类型的数字
     * 
     * 指导：
     * - 使用sscanf(tokens[p].str, "%u", &number)进行字符串到数字的转换
     * - 如果转换失败（返回值小于1），设置success为false并返回0
     * - 如果转换成功，返回转换后的数字
     * 
     * 示例：tokens[p].str = "123" -> 返回123
     */
    if (tokens[p].type != TK_NUMBER){
      *success = false;
      return 0;
    }
    if (sscanf(tokens[p].str, "%u", &number) < 1){
      *success = false;
      return 0;
    }
    return number;
  } else if (check_parentheses(p, q) == true){
    /* TODO(human): 处理被括号包围的整个表达式
     * 任务：优化处理被一对括号包围的表达式
     * 
     * 指导：
     * - 如果整个表达式被一对括号包围（如"(1+2)"），直接递归处理括号内部
     * - 调用eval(p + 1, q - 1, success)跳过外围括号
     * - 这是一种优化，可以减少不必要的运算步骤
     */
    return eval(p + 1, q - 1, success);
  } else {
    /* 主表达式求值逻辑 */
    if (check_expr_parentheses(p, q) == false){
      *success = false;
      return 0;
    }
    
    int i;
    int top = -1;
    int op_type;
    word_t val1;
    word_t val2;
    struct stack_node stack[1024];
    int main_op_idx = -1;
    // int main_op_pri = 1e9;
    
    /* TODO(human): 实现主运算符查找算法
     * 
     * 关键概念：主运算符是表达式中优先级最低且最后被结合的运算符
     * 规则：
     * 1. 括号内的运算符不能成为主运算符
     * 2. 优先级越低的运算符越有可能是主运算符  
     * 3. 相同优先级下，根据结合性选择最后结合的运算符（右结合）
     * 
     * 算法步骤：
     * 1. 使用栈结构遍历token数组
     * 2. 遇到左括号压栈，遇到右括号弹出栈内元素直到左括号
     * 3. 对于运算符，与栈顶元素比较优先级
     * 4. 优先级<=栈顶优先级时，弹出栈顶，当前运算符压栈
     * 5. 最终栈底元素即为主运算符
     * 
     * 数据结构：
     * - stack数组保存候选运算符的索引和类型
     * - top变量作为栈指针
     * 
     * 示例：表达式"1+2*3"
     * - +的优先级为1，*的优先级为2
     * - +是最低优先级，因此是主运算符
     */
    
    // int paren_depth = 0;
    for (i = p; i <= q; i++){
        Assert(top < 1024, "stack in eval function over overflow!");
        if (tokens[i].type == TK_NUMBER) continue;
        if (top < 0){
          top++;
          stack[top].idx = i;
          stack[top].type = tokens[i].type;
          continue;
        }
        if (tokens[i].type == (int)('(')){
          top++;
          stack[top].idx = i;
          stack[top].type = tokens[i].type;
          continue;
        }
        if (tokens[i].type == (int)(')')){
          // 就一直弹栈，直到遇到匹配的左括号
          // 这意味着括号内所有运算符都被排除了候选资格
          while (top >=0 && stack[top].type != '('){ 
            top--;
          }
          top--;
          continue;
        }
        while (top >= 0 && priority(tokens[i].type) <= priority(stack[top].type)){
          if (tokens[i].type == stack[top].type && tokens[i].type == TK_NEGATIVE){
            break;
        }
      }
      top++;
      stack[top].idx = i;
      stack[top].type = tokens[i].type;
    }

    Assert(top < 1024, "stack in eval function over overflow!");
    if (main_op_idx < 0){
      *success = false;
      return 0;
    } 
  
    /* TODO(human): 根据找到的主运算符进行递归求值
     * 
     * 任务：使用分治算法进行递归求值
     * 
     * 步骤：
     * 1. 根据主运算符位置将表达式分割为左右两部分
     * 2. 递归调用eval处理左子表达式（p到主运算符前）
     * 3. 递归调用eval处理右子表达式（主运算符后到q）
     * 4. 根据运算符类型执行相应的算术运算
     * 
     * 特殊情况：
     * - 一元负号（TK_NEGATIVE）：右操作数从主运算符后一位开始
     * - 二元运算符：左操作数从p到主运算符前一位，右操作数从主运算符后一位到q
     * - 除法需要检查除数为0的情况
     * 
     * 示例：表达式"1+2*3"
     * - 主运算符是+（索引1）
     * - 左子表达式："1"（p=0到主运算符前）
     * - 右子表达式："2*3"（主运算符后到q=3）
     */
    
    // 将找到的主运算符放入栈底，兼容下方参考框架
    stack[0].idx = main_op_idx;
    stack[0].type = tokens[main_op_idx].type;
    top = 0;

    if (stack[0].type == TK_NEGATIVE){
      val1 = (word_t)(-1);
      val2 = eval(stack[0].idx + 1, q, success);
      op_type = '*';
    } else {
      val1 = eval(p, stack[0].idx - 1, success);
      if (*success != true) return 0;
      val2 = eval(stack[0].idx + 1, q, success);
      op_type = stack[0].type;
    }

    if (*success != true){
      return 0; 
    }
    
    switch (op_type)
    {
    case '+':
      return val1 + val2;
    case '-':
      return val1 - val2;
    case '*':
      return val1 * val2;
    case '/':
      if (val2 == 0){
        *success = false;
        return 0;
      }
      return val1 / val2;
    default:
      Assert(0, "No corresponding operator found");
    }
    
    // 以下是实现参考框架，请根据你的理解完成
    /*
    if (stack[0].type == TK_NEGATIVE){
      // 处理一元负号：将其转换为乘以-1
      val1 = -1;
      val2 = eval(stack[0].idx + 1, q, success);
      op_type = '*';
    } else {
      // 处理二元运算符：分割表达式为左右两部分
      val1 = eval(p, stack[0].idx - 1, success);
      val2 = eval(stack[0].idx + 1, q, success);
      op_type = stack[0].type;
    }

    if (*success != true){
      return 0; 
    }
    
    switch (op_type)
    {
    case '+':
      return val1 + val2;
    case '-':
      return val1 - val2;
    case '*':
      return val1 * val2;
    case '/':
      if (val2 == 0){
        *success = false;
        return 0;
      }
      return val1 / val2;
    default:
      Assert(0, "No corresponding operator found");
    }
    */
  }
}