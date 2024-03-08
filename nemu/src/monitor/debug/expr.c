#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */

/** note：
 * 从键盘上读入命令后（涉及到str表达式分析实现）, NEMU需要解析该命令——利用框架代码调用单步执行——打印寄存器——扫描内存
*/

#include <sys/types.h>
#include <regex.h>

enum
{
	TK_NOTYPE = 256,
	TK_EQ,
	TK_NEQ,
	TK_NUMBER,
	TK_HEXNUMBER,
	TK_REGISTER,
	TK_AND,
	TK_OR,
	TK_NOT,
	TK_MINUS_SIGN,
	TK_POINTER,

	/* TODO: Add more token types */

};

static struct rule
{
	char *regex;
	int token_type;
} rules[] = {

		/* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

		{" +", TK_NOTYPE}, // spaces
		{"\\+", '+'},			 // plus
		{"==", TK_EQ},		 // equal
		{"!=", TK_NEQ},
		{"!", TK_NOT},
		{"&&", TK_AND},
		{"\\|\\|", TK_OR},
		{"\\$[a-zA-z]+", TK_REGISTER},						 // register
		{"\\b0[xX][0-9a-fA-F]+\\b", TK_HEXNUMBER}, // hexnumber
		{"\\b[0-9]+\\b", TK_NUMBER},							 // number
		{"-", '-'},																 // sub
		{"\\*", '*'},															 // multiply
		{"/", '/'},																 // divide
		{"\\(", '('},															 // left bracket
		{"\\)", ')'},															 // right bracket
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]))

static regex_t re[NR_REGEX];

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
		ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED); // 这个函数把指定的正则表达式pattern编译成一种特定的数据格式compiled，这样可以使匹配更有效。
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

Token tokens[32];
int nr_token;

static bool make_token(char *e) // 把expr中的token保存在tokens数组中
{
	int position = 0;
	int i;
	regmatch_t pmatch;

	nr_token = 0; // token数量

	while (e[position] != '\0')
	{
		/* 逐个匹配所有的rules */
		for (i = 0; i < NR_REGEX; i++)
		{
			// 用regexec匹配目标文本串，pmatch.rm_so==0表示匹配串在目标串中的第一个位置。pmatch.rm_eo表示结束位置，position和substr_len表示读取完后的位置和读取长度。
			if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0)
			{
				char *substr_start = e + position;
				int substr_len = pmatch.rm_eo;

				Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
						i, rules[i].regex, position, substr_len, substr_len, substr_start);
				position += substr_len; // 后推到下一个位置

				/* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

				switch (rules[i].token_type) // 得到了一个token，判断type
				{
				case TK_NOTYPE:
					break;
				default:
				{
					tokens[nr_token].type = rules[i].token_type;
					if (rules[i].token_type == TK_REGISTER)
					{ // Register
						char *reg_start = e + (position - substr_len) + 1;
						strncpy(tokens[nr_token].str, reg_start, substr_len - 1);
						int t;
						for (t = 0; t <= strlen(tokens[nr_token].str); t++)
						{
							int x = tokens[nr_token].str[t];
							if (x >= 'A' && x <= 'Z')
								x += ('a' - 'A'); // lower大写转小写
							tokens[nr_token].str[t] = (char)x;
						}
					}
					else
						strncpy(tokens[nr_token].str, substr_start, substr_len);
					tokens[nr_token].str[substr_len] = '\0';
					++nr_token;
				}
				}
				break;
			}
		}
		// I found Assert() showed in instruction doc, so change to Assert()
		Assert(!(i == NR_REGEX), "Position not matched %d\n%s\n%*.s^\n", position, e, position, "");
	}

	return true;
}

int dominant_operator(int p, int q) // 找到并返回结合运算符的位置。
{
	/**
	 * 按照指导书所说，根据结合性, 最后被结合的运算符才是dominant operator.一个例子是1+2+3, 它的dominant operator 应该是右边的+. 
	 * 关键在于，最后一步的运算符优先级应该最低。因此，实现时，从右往左扫描表达式，并且设置优先级，遇到')'括号时要一直扫描到'('出现位置，没有括号就从右边第一个开始.
	 * 举例说明实现，表达式2*8+5，规定加减法的优先级是4，乘除法的优先级是3，维护一个初始优先级op_level，初值为0，从右往左第一次扫描到加法符号，
	 * 比较当前优先级op_level，如果当前值小于4（因为当前是加法，比较值为4），则pos 为加号的位置，优先级更新为4，
	 * 接下来继续扫描，遇到了乘法，优先级更高，值更小，因为op_level==4 > 3，不更新level 和pos，最后结合的位置仍然是+处，对应2*8+5 最后一步运算是+ 而不是*。
	 * 只要通过switch 语句列出各优先级运算符的情况即可。
	*/
	int i;
	int pos = p;
	int op_level = 0; // 越小越优先
	int b_num = 0;
	for (i = q; i >= p; i--)
	{
		if (tokens[i].type == ')')
			b_num++;
		if (tokens[i].type == '(')
			b_num--;
		if (b_num != 0)
			continue;
		
		switch (tokens[i].type)
		{
		case '+':
		{
			if (op_level < 4)
			{
				pos = i;
				op_level = 4;
			}
			break;
		}
		case '-':
		{
			if (op_level < 4)
			{
				pos = i;
				op_level = 4;
			}
			break;
		}
		case '*':
		{
			if (op_level < 3)
			{
				pos = i;
				op_level = 3;
			}
			break;
		}
		case '/':
		{
			if (op_level < 3)
			{
				pos = i;
				op_level = 3;
			}
			break;
		}
		case TK_NOT:
		{
			if (op_level < 2)
			{
				pos = i;
				op_level = 2;
			}
			break;
		}
		case TK_EQ:
		{
			if (op_level < 7)
			{
				pos = i;
				op_level = 7;
			}
			break;
		}
		case TK_NEQ:
		{
			if (op_level < 7)
			{
				pos = i;
				op_level = 7;
			}
			break;
		}
		case TK_AND:
		{
			if (op_level < 11)
			{
				pos = i;
				op_level = 11;
			}
			break;
		}
		case TK_OR:
		{
			if (op_level < 12)
			{
				pos = i;
				op_level = 12;
			}
			break;
		}
		case TK_MINUS_SIGN:
		{
			if (op_level < 2)
			{
				pos = i;
				op_level = 2;
			}
			break;
		}
		case TK_POINTER:
		{
			if (op_level < 2)
			{
				pos = i;
				op_level = 2;
			}
			break;
		}
		default:
			break;
		}
	}

	Assert(!(op_level == 0), "dominant_operator error!");
	if (op_level == 2)
	{
		op_level = 0;
		for (i = p; i <= q; i++)
		{
			if (tokens[i].type == '(')
				b_num++;
			if (tokens[i].type == ')')
				b_num--;
			if (b_num != 0)
				continue;
			switch (tokens[i].type)
			{
			case TK_MINUS_SIGN:
			{
				if (op_level < 2)
				{
					pos = i;
					op_level = 2;
				}
				break;
			}
			case TK_NOT:
			{
				if (op_level < 2)
				{
					pos = i;
					op_level = 2;
				}
				break;
			}
			case TK_POINTER:
			{
				if (op_level < 2)
				{
					pos = i;
					op_level = 2;
				}
				break;
			}
			}
		}
	}
	return pos;
}

bool check_bracket(int p, int q) // 检查左右括号匹配，p开始位置，q结束位置
{
	if (tokens[p].type != '(' || tokens[q].type != ')')
		return false;
	int b_num = 0;
	for (int i = p; i <= q; i++)
	{
		if (tokens[i].type == '(')
			b_num++;
		if (tokens[i].type == ')')
			b_num--;
		if (b_num == 0 && i != q) // 没到结束位置，失败
			return false;
	}
	if (b_num == 0)
		return true;
	return false;
}

uint32_t eval(int p, int q) // 转换字符串表达式为对应的数值或寄存器号
{
	Assert(!(p > q), "eval error!");
	if (p == q)
	{
		uint32_t num = 0;
		if (tokens[p].type == TK_NUMBER)
		{
			sscanf(tokens[p].str, "%d", &num);
		}
		else if (tokens[p].type == TK_HEXNUMBER)
		{
			sscanf(tokens[p].str, "%x", &num);
		}
		else if (tokens[p].type == TK_REGISTER)
		{
			if (strlen(tokens[p].str) == 3)
			{
				int i;
				for (i = R_EAX; i <= R_EDI; i++)
				{
					if (strcmp(tokens[p].str, regsl[i]) == 0)
					{
						break;
					}
				}
				if (i > R_EDI)
				{
					if (strcmp(tokens[p].str, "eip") == 0)
					{
						num = cpu.eip;
					}
					else
					{
						Assert(0, "eval error!");
					}
				}
				else
					return num = reg_l(i); // 返回寄存器号
			}
			else if (strlen(tokens[p].str) == 2)
			{
				int i;
				for (i = R_AX; i <= R_DI; i++)
				{
					if (strcmp(tokens[p].str, regsw[i]) == 0)
					{
						break;
					}
				}
				if (i <= R_DI)
					return num = reg_w(i);
				for (i = R_AL; i <= R_BH; i++)
				{
					if (strcmp(tokens[p].str, regsb[i]) == 0)
					{
						break;
					}
				}
				if (i <= R_BH)
					return num = reg_b(i);
				Assert(0, "eval error!");
			}
			else
			{
				Assert(0, "eval error!");
			}
		}
		else
		{
			Assert(0, "eval error!");
		}
		return num;
	}
	uint32_t ans = 0;
	if (check_bracket(p, q))
		return eval(p + 1, q - 1); // 含括号的递归计算表达式
	else
	{
		int pos = dominant_operator(p, q); // 找到表达式的最后结合点
		if (p == pos || tokens[pos].type == TK_NOT || tokens[pos].type == TK_MINUS_SIGN || tokens[pos].type == TK_POINTER)
		{
			uint32_t q_ans = eval(pos + 1, q); // ！ -  *右侧的子表达式转换
			switch (tokens[pos].type)
			{
			case TK_POINTER:
				return vaddr_read(q_ans, 4);
			case TK_NOT:
				return !q_ans;
			case TK_MINUS_SIGN:
				return -q_ans;
			default:
			{
				Assert(0, "eval error!");
			}
			}
		}
		uint32_t p_ans = eval(p, pos - 1), q_ans = eval(pos + 1, q); // 转换双目运算符左右两侧子表达式
		switch (tokens[pos].type)
		{
		case '+':
			ans = p_ans + q_ans;
			break;
		case '-':
			ans = p_ans - q_ans;
			break;
		case '*':
			ans = p_ans * q_ans;
			break;
		case '/':
			if (q_ans == 0)
			{
				Assert(0, "eval error!");
			}
			else
				ans = p_ans / q_ans;
			break;
		case TK_EQ:
			ans = p_ans == q_ans;
			break;
		case TK_NEQ:
			ans = p_ans != q_ans;
			break;
		case TK_AND:
			ans = p_ans && q_ans;
			break;
		case TK_OR:
			ans = p_ans || q_ans;
			break;
		default:
		{
			Assert(0, "eval error!");
		}
		}
	}
	return ans; // 返回表达式计算结果
}

uint32_t expr(char *e, bool *success) // 检查表达式合法性并转换字符串表达式为值
{
	if (!make_token(e))
	{
		*success = false;
		return 0;
	}
	/* TODO: Insert codes to evaluate the expression. */
	int i;
	int brack = 0;
	for (i = 0; i < nr_token; i++)
	{
		// 括号匹配检查
		if (tokens[i].type == '(')
			brack++;
		if (tokens[i].type == ')')
			brack--;
		if (brack < 0)
		{
			*success = false;
			return 0;
		}

		// 判断并修改‘-’是负号而不是减号。满足if条件说明是负号
		if (tokens[i].type == '-' && (i == 0 || (tokens[i - 1].type != ')' && tokens[i - 1].type != TK_NUMBER && tokens[i - 1].type != TK_HEXNUMBER && tokens[i - 1].type != TK_REGISTER)))
		{
			tokens[i].type = TK_MINUS_SIGN;
		}

		// 判断并修改‘*’是指针符号而不是乘号
		if (tokens[i].type == '*' && (i == 0 || (tokens[i - 1].type != ')' && tokens[i - 1].type != TK_NUMBER && tokens[i - 1].type != TK_HEXNUMBER && tokens[i - 1].type != TK_REGISTER)))
		{
			tokens[i].type = TK_POINTER;
		}
	}

	*success = true;
	return eval(0, nr_token - 1);
}
