#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

// bool* flag; // I found Assert() showed in instruction doc, so change to Assert()

enum {
  TK_NOTYPE = 256, TK_EQ, TK_NEQ, TK_NUMBER, TK_HEXNUMBER, TK_REGISTER, TK_AND, TK_OR, TK_NOT, TK_SUB, TK_POINTER,

  /* TODO: Add more token types */

};

static struct rule {
  char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"==", TK_EQ},         // equal
  {"!=", TK_NEQ},
  {"!", TK_NOT},
  {"&&", TK_AND},
  {"\\|\\|", TK_OR},
  {"\\$[a-zA-z]+", TK_REGISTER},		// register
  {"\\b0[xX][0-9a-fA-F]+\\b", TK_HEXNUMBER}, // hexnumber
  {"\\b[0-9]+\\b", TK_NUMBER},				// number
  {"-", '-'},						// sub
  {"\\*", '*'},					// multiply
  {"/", '/'},						// divide
  {"\\(", '('},					// left bracket
  {"\\)", ')'},					// right bracket
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

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

Token tokens[32];
int nr_token;

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
        	case TK_NOTYPE: break;
			default: {
				tokens[nr_token].type = rules[i].token_type;				
				if (rules[i].token_type == TK_REGISTER) { // Register
					char* reg_start = e + (position - substr_len) + 1;
					strncpy(tokens[nr_token].str, reg_start, substr_len - 1);
					int t;
					for (t = 0; t <= strlen(tokens[nr_token].str); t++) { 
						int x = tokens[nr_token].str[t];
						if (x >= 'A' && x <= 'Z') x += ('a' - 'A');
						tokens[nr_token].str[t] = (char)x;
					}
				} else
					strncpy(tokens[nr_token].str, substr_start, substr_len);
				tokens[nr_token].str[substr_len] = '\0';
				++nr_token;
			}
		}
        break;
      }
    }
		// I found Assert() showed in instruction doc, so change to Assert()
		Assert(!(i == NR_REGEX),"Position not matched%d\n%s\n%*.s^\n", position, e, position, "");
    // if (i == NR_REGEX) { 
      // printf("Position not matched%d\n%s\n%*.s^\n", position, e, position, "");
      // return false;
			
    // }
  }

  return true;
}

int dominant_operator(int p, int q) {
		int i;
		int pos = p;
		int op_level = 0;
		int b_num = 0;
		for (i = q; i >= p; i--) {
			if (tokens[i].type == ')') b_num++;
			if (tokens[i].type == '(') b_num--;
			if (b_num != 0) continue;
			switch (tokens[i].type) {
				case '+': {  
					if (op_level < 4) {
						pos = i;
						op_level = 4;
					}
					break;
				}
				case '-': {  
					if (op_level < 4) {
						pos = i;
						op_level = 4;
					}
					break;
				}
				case '*': {  
					if (op_level < 3) {
						pos = i; 
						op_level = 3;
					}
					break;
				}
				case '/': {  
					if (op_level < 3) {
						pos = i;
						op_level = 3;
					}
					break;
				}
				case TK_NOT: {  
					if (op_level < 2) {
						pos = i; 
						op_level = 2;
					}
					break;
				}
				case TK_EQ: {  
					if (op_level < 7) {
						pos = i; 
						op_level = 7;
					}
					break;
				}
				case TK_NEQ: {  
					if (op_level < 7) {
						pos = i;
						op_level = 7;
					}
					break;
				}
				case TK_AND: {  
					if (op_level < 11) {
						pos = i;
						op_level = 11;
					}
					break;
				}
				case TK_OR: {  
				if (op_level < 12) {
					pos = i;
					op_level = 12;
				}
				break;
				}
				case TK_SUB: {  
					if (op_level < 2) {
						pos = i;
						op_level = 2;
					}
					break;
				}
				case TK_POINTER: {  
					if (op_level < 2) {
						pos = i;
						op_level = 2;
					}
					break;
				}
				default: break;
			}
		}

		// if (op_level == 0) {
		// 	*flag = false; 
		// 	return 0;
		// }
		Assert(!(op_level == 0), "dominant_operator error!");
		if (op_level == 2) {
			op_level = 0;
			for (i = p; i <= q; i++) {
				if (tokens[i].type == '(') b_num++;
				if (tokens[i].type == ')') b_num--;
				if (b_num != 0) continue;
				switch (tokens[i].type) {
					case TK_SUB: {  
						if (op_level < 2) {
							pos = i;
							op_level = 2;
						}
						break;
					}
					case TK_NOT: {  
						if (op_level < 2) {
							pos = i;
							op_level = 2;
						}
						break;
					}
					case TK_POINTER: {  
						if (op_level < 2) {
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

bool check_bracket(int p, int q) {
	if (tokens[p].type != '(' || tokens[q].type != ')') return false;
	int b_num = 0;
	for (int i = p; i <= q; i++) {
		if (tokens[i].type == '(') b_num++;
		if (tokens[i].type == ')') b_num--;
		if (b_num == 0 && i != q) return false;
	}
	if (b_num == 0) return true;
	return false;
}

uint32_t eval(int p, int q) {
	Assert(!(p > q), "eval error!");
	if (p == q) {
		uint32_t num = 0;
		if (tokens[p].type == TK_NUMBER) {
			sscanf(tokens[p].str, "%d", &num);
		} 
		else if (tokens[p].type == TK_HEXNUMBER) {
			sscanf(tokens[p].str, "%x", &num);
		} 
		else if (tokens[p].type == TK_REGISTER) {
			if (strlen(tokens[p].str) == 3) {
				int i;
				for (i = R_EAX; i <= R_EDI ; i ++) {
					if (strcmp(tokens[p].str, regsl[i]) == 0) {
						break;
					}
				}
				if (i > R_EDI) {
					if (strcmp(tokens[p].str, "eip") == 0){
						num = cpu.eip;
					}
					else {
						Assert(0, "eval error!");
					}
				} 
				else return num = reg_l(i);
				
			}
			else if (strlen(tokens[p].str) == 2) {
				int i;
				for (i = R_AX; i <= R_DI; i ++) {
					if (strcmp(tokens[p].str, regsw[i]) == 0) {
						break;
					}
				}	
				if (i <= R_DI) return num = reg_w(i);
				for (i = R_AL; i <= R_BH; i ++) {
					if (strcmp(tokens[p].str, regsb[i]) == 0) {
						break;
					}
				}
				if (i <= R_BH) return num = reg_b(i);
				Assert(0, "eval error!");
			}
			else {
				Assert(0, "eval error!");
			}
		} 
		else {
			Assert(0, "eval error!");
		}
		return num;
	}
	uint32_t ans = 0;
	if (check_bracket(p, q)) return eval(p + 1, q - 1);
	else {
		int pos = dominant_operator(p, q);
		if (p == pos || tokens[pos].type == TK_NOT || tokens[pos].type == TK_SUB || tokens[pos].type == TK_POINTER) {
			uint32_t q_ans = eval(pos + 1, q);
			switch (tokens[pos].type) {
				case TK_POINTER: return vaddr_read(q_ans, 4);
				case TK_NOT: return !q_ans;
				case TK_SUB: return -q_ans;
				default: {
					Assert(0, "eval error!");
				}
			}
		}
		uint32_t p_ans = eval(p, pos - 1), q_ans =  eval(pos + 1, q);
		switch (tokens[pos].type) {
			case '+': ans = p_ans + q_ans; break;
			case '-': ans = p_ans - q_ans; break;
			case '*': ans = p_ans * q_ans; break;
			case '/': 
				if (q_ans == 0) {
					Assert(0, "eval error!");
				}
				else 
					ans = p_ans / q_ans; 
				break;
			case TK_EQ : ans = p_ans == q_ans; break;
			case TK_NEQ: ans = p_ans != q_ans; break;
			case TK_AND: ans = p_ans && q_ans; break;
			case TK_OR : ans = p_ans && q_ans; break;
			default: {
				Assert(0, "eval error!");
			}
		}
	}
	return ans;
}



uint32_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }
	/* TODO: Insert codes to evaluate the expression. */
	int i;
	int brack = 0;
	for (i = 0; i < nr_token ; i++) {
		if (tokens[i].type == '(') brack++;
		if (tokens[i].type == ')') brack--;
		if (brack < 0) {
			*success = false;
			return 0;
		}
		
		if (tokens[i].type == '-' && (i == 0 || (tokens[i - 1].type != ')' && tokens[i - 1].type != TK_NUMBER && tokens[i - 1].type != TK_HEXNUMBER && tokens[i - 1].type != TK_REGISTER))) {
			tokens[i].type = TK_SUB;
		}
								
		if (tokens[i].type == '*' && (i == 0 || (tokens[i - 1].type != ')' && tokens[i - 1].type != TK_NUMBER && tokens[i - 1].type != TK_HEXNUMBER && tokens[i - 1].type != TK_REGISTER))) {
			tokens[i].type = TK_POINTER;
		}
	}
	
	*success = true;
	return eval(0, nr_token - 1);
}




