#include <stdlib.h>

typedef int    ip_value_t;
typedef int    ip_proc_ref_t;

#define IP_VALUE2INT(v) (v)
#define IP_INT2VALUE(i) (i)

enum ip_code {
              IP_CODE_CONST,
              IP_CODE_GET_LOCAL,
              IP_CODE_SET_LOCAL,
              IP_CODE_ADD,
              IP_CODE_SUB,
              IP_CODE_JUMP,
              IP_CODE_JUMP_IF_ZERO,
              IP_CODE_JUMP_IF_NEG,
              IP_CODE_CALL,
              IP_CODE_RETURN,
              IP_CODE_EXIT,
};


struct ip_inst {
  enum ip_code code;
  union { ip_value_t v; int i; size_t pos; ip_proc_ref_t p;} u;
};


#define IP_INST_CONST(v)          {IP_CODE_CONST,        {v}}
#define IP_INST_GET_LOCAL(i)      {IP_CODE_GET_LOCAL,    {i}}
#define IP_INST_SET_LOCAL(i)      {IP_CODE_SET_LOCAL,    {i}}
#define IP_INST_ADD()             {IP_CODE_ADD,          {}}
#define IP_INST_SUB()             {IP_CODE_SUB,          {}}
#define IP_INST_JUMP(pos)         {IP_CODE_JUMP,         {pos}}
#define IP_INST_JUMP_IF_ZERO(pos) {IP_CODE_JUMP_IF_ZERO, {pos}}
#define IP_INST_JUMP_IF_NEG(pos)  {IP_CODE_JUMP_IF_NEG,  {pos}}
#define IP_INST_CALL(p)           {IP_CODE_CALL,         {p}}
#define IP_INST_RETURN()          {IP_CODE_RETURN,       {}}
#define IP_INST_EXIT()            {IP_CODE_EXIT,         {}}
