#ifndef IP_H_VM
#define IP_H_VM

#include <stdlib.h>

typedef int    ip_value_t;
typedef int    ip_proc_ref_t;

#define IP_VALUE2INT(v) (v)
#define IP_INT2VALUE(i) (i)
#define IP_VALUE2PROCREF(v) (v)
#define IP_PROCREF2VALUE(p) (p)

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
              IP_CODE_CALL_INDIRECT,
              IP_CODE_RETURN,
              IP_CODE_EXIT,
};


struct ip_inst {
  enum ip_code code;
  union { ip_value_t v; int i; size_t pos; ip_proc_ref_t p;} u;
};


#define IP_INST_CONST(v)          {IP_CODE_CONST,         {v}}
#define IP_INST_GET_LOCAL(i)      {IP_CODE_GET_LOCAL,     {i}}
#define IP_INST_SET_LOCAL(i)      {IP_CODE_SET_LOCAL,     {i}}
#define IP_INST_ADD()             {IP_CODE_ADD,           {}}
#define IP_INST_SUB()             {IP_CODE_SUB,           {}}
#define IP_INST_JUMP(pos)         {IP_CODE_JUMP,          {pos}}
#define IP_INST_JUMP_IF_ZERO(pos) {IP_CODE_JUMP_IF_ZERO,  {pos}}
#define IP_INST_JUMP_IF_NEG(pos)  {IP_CODE_JUMP_IF_NEG,   {pos}}
#define IP_INST_CALL(p)           {IP_CODE_CALL,          {p}}
#define IP_INST_CALL_INDIRECT()   {IP_CODE_CALL_INDIRECT, {}}
#define IP_INST_RETURN()          {IP_CODE_RETURN,        {}}
#define IP_INST_EXIT()            {IP_CODE_EXIT,          {}}


struct ip_proc;
int ip_proc_init(struct ip_proc *proc, size_t nargs, size_t nlocals, size_t ninsts, struct ip_inst *insts);
int ip_proc_new(size_t nargs, size_t nlocals, size_t ninsts, struct ip_inst *insts, struct ip_proc **ret);
void ip_proc_dtor(struct ip_proc *proc);

struct ip_vm;

int ip_vm_init(struct ip_vm *vm);
int ip_vm_new(struct ip_vm **vm);
void ip_vm_dtor(struct ip_vm *vm);
ip_proc_ref_t ip_vm_reserve_proc(struct ip_vm *vm);
void ip_vm_register_proc_at(struct ip_vm *vm, struct ip_proc *proc, ip_proc_ref_t at);
ip_proc_ref_t ip_vm_register_proc(struct ip_vm *vm, struct ip_proc *proc);
int ip_vm_push_arg(struct ip_vm *vm, ip_value_t arg);
int ip_vm_get_result(struct ip_vm *vm, ip_value_t * result);


int ip_vm_exec(struct ip_vm *vm, ip_proc_ref_t procref);

#endif
