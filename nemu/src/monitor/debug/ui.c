#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

/** 
 * note: 
 * 用 readline 读取输入的命令之后，用 strtok()分解第一个字符串（以空格分开），然后与cmd_table[]中的 name 比较，执行对应的函数。
*/


void cpu_exec(uint64_t);

/* We use the `readline' library to provide more flexibility to read from stdin. */
char *rl_gets()
{
  static char *line_read = NULL;

  if (line_read)
  {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read)
  {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args)
{
  cpu_exec(-1);
  return 0;
}

static int cmd_q(char *args)
{
  return -1;
}

static int cmd_help(char *args);

static int cmd_si(char *args) // 单步执行num步
{
  int num = 0;
  if (args == NULL)
    num = 1;
  else
    sscanf(args, "%d", &num); 
  cpu_exec(num);
  return 0;
}

static int cmd_info(char *args)
{
  int i;
  if (args[0] == 'r') // 输入info r 打印寄存器状态
  {
    for (i = R_EAX; i <= R_EDI; i++)
    {
      printf("%s\t0x%08x\n", regsl[i], reg_l(i));
    }
    printf("eip\t0x%08x\n", cpu.eip);
  }
  else if (args[0] == 'w') // 输入 info w 打印监视点信息
  {
    print_w();
  }
  else
  {
    printf("Invalid Args!");
    assert(0);
  }
  return 0;
}

static int cmd_p(char *args)
{
  Assert(!(args == NULL), "Needs a expr!");
  uint32_t ans;
  bool success;
  ans = expr(args, &success);
  Assert(success, "failure error!");
  printf("answer: %d\n", ans);
  return 0;
}

static int cmd_x(char *args) // 扫描内存，格式为x N EXPR，求出表达式EXPR的值, 将结果作为起始内存地址, 以十六进制形式输出连续的N个4字节.
{
  if (args == NULL)
  {
    printf("Invalid Args!\n");
    return 0;
  }
  char *tokens = strtok(args, " "); // 分割 "N EXPR"
  int N, exprs;
  sscanf(tokens, "%d", &N);
  char *eps = tokens + strlen(tokens) + 1;  // 获取"EXPR"
  bool flag = true;
  exprs = expr(eps, &flag); // 返回EXPR表示的值
  if (!flag)
  {
    printf("Invalid !\n");
    return 0;
  }
  for (int i = 0; i < N; i++)
  {
    printf("0x%08x\t0x%08x\n", exprs + i * 4, vaddr_read(exprs + i * 4, 4)); // 格式控制 %08x 表示输出8个16进制表示的字符
  }
  return 0;
}

static int cmd_w(char *args) // 命令格式 w EXPR ，创建一个监视点。（当表达式EXPR的值发生变化时, 暂停程序执行）
{
  Assert(!(args == NULL), "Need more args!");
  bool flag = true;
  uint32_t val = expr(args, &flag); // 输入表达式求值
  Assert(flag, "Need more args!");
  WP *wp = new_wp(); // 创建一个监视点
  Assert(!(wp == NULL), "No free space to add watchpoints!");
  strcpy(wp->exprs, args);
  wp->val = val; // 记录当前表达式和它的值
  printf("Succefully add watchpoint %d\n", wp->NO);
  return 0;
}

static int cmd_d(char *args)
{
  Assert(!(args == NULL), "Need more args!");
  int id;
  sscanf(args, "%d", &id);
  bool del_success = true;
  WP *wp = delete_wp(id, &del_success);
  Assert(del_success, "No such watchpoint!");
  free_wp(wp);
  printf("Succefully delete!\n");
  return 0;
}

static struct
{
  char *name;
  char *description;
  int (*handler)(char *);
} cmd_table[] = {
    {"help", "Display informations about all supported commands", cmd_help},
    {"c", "Continue the execution of the program", cmd_c},
    {"q", "Exit NEMU", cmd_q},

    /* TODO: Add more commands */
    {"si", "Step into implementation of N instructions after the suspension of execution.When N is notgiven,the default is 1.", cmd_si},
    {"info", "r for print register state\nw for print watchpoint information", cmd_info},
    {"p", "Calculate expressions", cmd_p},
    {"x", "Calculate the value of the expression and regard the result as the starting memory address.", cmd_x},
    {"w", "Add watchpoint", cmd_w},
    {"d", "Delete watchpoint", cmd_d},
};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args)
{
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL)
  {
    /* no argument given */
    for (i = 0; i < NR_CMD; i++)
    {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else
  {
    for (i = 0; i < NR_CMD; i++)
    {
      if (strcmp(arg, cmd_table[i].name) == 0)
      {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

// main函数
void ui_mainloop(int is_batch_mode)
{
  if (is_batch_mode)
  {
    cmd_c(NULL);
    return;
  }

  while (1)
  {
    char *str = rl_gets();
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL)
    {
      continue;
    }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end)
    {
      args = NULL;
    }

#ifdef HAS_IOE
    extern void sdl_clear_event_queue(void);
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i++)
    {
      if (strcmp(cmd, cmd_table[i].name) == 0)
      {
        if (cmd_table[i].handler(args) < 0)
        {
          return;
        }
        break;
      }
    }

    if (i == NR_CMD)
    {
      printf("Unknown command '%s'\n", cmd);
    }
  }
}
