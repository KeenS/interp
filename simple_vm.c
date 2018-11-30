#include "inst.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct ip_empty_stack {
  size_t size;
  size_t sp;
  ip_value_t *data;
};

int
ip_empty_stack_init(struct ip_empty_stack *stack, size_t size)
{
  stack->data = malloc(size * sizeof(ip_value_t));
  if (NULL == stack->data) {
    return 1;
  }
  stack->size = size;
  stack->sp = 0;
  return 0;
}

int
ip_empty_stack_new(size_t size, struct ip_empty_stack **ret)
{
  *ret = malloc(sizeof(struct ip_empty_stack));
  if(NULL == *ret) {
    return 1;
  }

  return ip_empty_stack_init(*ret, size);
}

void
ip_empty_stack_dtor(struct ip_empty_stack *stack)
{
  free(stack->data);
}


int
ip_empty_stack_push(struct ip_empty_stack * stack, ip_value_t i)
{
  if (stack->sp < stack->size) {
    stack->data[stack->sp++] = i;
    return 0;
  } else {
    return 1;
  }
}

int
ip_empty_stack_pop(struct ip_empty_stack * stack, ip_value_t * i)
{
  if (0 < stack->sp) {
    *i = stack->data[--stack->sp];
    return 0;
  } else {
    return 1;
  }
}

int
ip_empty_stack_peek(struct ip_empty_stack * stack, ip_value_t * i)
{
  if (0 < stack->sp) {
    *i = stack->data[stack->sp - 1];
    return 0;
  } else {
    return 1;
  }
}


struct ip_proc {
  size_t nlocals;
  ip_value_t *locals;
  size_t ninsts;
  struct ip_inst *insts;
};


int
ip_proc_init(struct ip_proc *proc, size_t nlocals, ip_value_t *locals, size_t ninsts, struct ip_inst *insts)
{
  proc->locals = malloc(nlocals * sizeof(ip_value_t));
  if (NULL == proc->locals) {
    return 1;
  }

  proc->insts = malloc(ninsts * sizeof(struct ip_inst));
  if (NULL == proc->insts) {
    return 1;
  }

  proc->nlocals = nlocals;
  proc->ninsts = ninsts;
  memcpy(proc->locals, locals, nlocals * sizeof(ip_value_t));
  memcpy(proc->insts,   insts, ninsts  * sizeof(struct ip_inst));

  return 0;
}

int
ip_proc_new(size_t nlocals, ip_value_t *locals, size_t ninsts, struct ip_inst *insts, struct ip_proc **ret)
{
  *ret = malloc(sizeof(struct ip_proc));
  if(NULL == *ret) {
    return 1;
  }

  return ip_proc_init(*ret, nlocals, locals, ninsts, insts);
}

void
ip_proc_dtor(struct ip_proc *proc)
{
  free(proc->locals);
  free(proc->insts);
}


struct ip_vm {
  struct ip_empty_stack stack;
};

int
ip_vm_init(struct ip_vm *vm)
{
  return ip_empty_stack_init( &vm->stack, 1024);
}

int
ip_vm_new(struct ip_vm **vm)
{

  *vm = malloc(sizeof(struct ip_vm));
  if(NULL == *vm) {
    return 1;
  }

  return ip_vm_init(*vm);
}


int
ip_vm_exec(struct ip_vm *vm, struct ip_proc *proc)
{
  int ip = 0;

#define POP(ref)  do{if (ip_empty_stack_pop(&vm->stack, ref)) { return 1;}} while(0)
#define PUSH(v)  do{if (ip_empty_stack_push(&vm->stack, v)) { return 1;}} while(0)


  while (1) {
    struct ip_inst inst = proc->insts[ip];
    switch(inst.code) {
    case IP_CODE_CONST: {
      ip_empty_stack_push(&vm->stack, inst.u.v);
      break;
    }
    case IP_CODE_GET_LOCAL: {
      int i;
      ip_value_t v;

      i = inst.u.i;
      v = proc->locals[i];

      PUSH(v);
      break;
    }
    case IP_CODE_SET_LOCAL: {
      int i;
      ip_value_t v;

      i = inst.u.i;
      POP(&v);

      proc->locals[i] = v;

      break;
    }
    case IP_CODE_ADD: {
      ip_value_t v1, v2, ret;
      int x, y;

      POP(&v1);
      POP(&v2);
      y = IP_VALUE2INT(v1);
      x = IP_VALUE2INT(v2);

      ret = IP_INT2VALUE(x + y);

      PUSH(ret);

      break;
    }
    case IP_CODE_SUB: {
      ip_value_t v1, v2, ret;
      int x, y;

      POP(&v1);
      POP(&v2);
      y = IP_VALUE2INT(v1);
      x = IP_VALUE2INT(v2);

      ret = IP_INT2VALUE(x - y);

      PUSH(ret);

      break;
    }
    case IP_CODE_JUMP: {
      ip = inst.u.pos;
      break;
    }
    case IP_CODE_JUMP_IF_ZERO: {
      ip_value_t v;

      POP(&v);

      if (!IP_VALUE2INT(v)) {
        ip = inst.u.pos;
      }
      break;
    }
    case IP_CODE_JUMP_IF_NEG: {
      ip_value_t v;

      POP(&v);

      if (IP_VALUE2INT(v) < 0) {
        ip = inst.u.pos;
      }
      break;
    }
    case IP_CODE_RETURN: {
      return 0;
    }
    default: {
      printf("code: %d, u: %d", inst.code, inst.u.i);
      return 1;
    }
    }
    ip += 1;
  }

#undef POP
#undef PUSH
}

int
ip_vm_get_result(struct ip_vm *vm, ip_value_t * result)
{
  return ip_empty_stack_pop(&vm->stack, result);
}

int
ip_proc_new_sum(struct ip_proc **proc)
{
  /* 0(arg)   - n */
  /* 1(local) - i */
  /* 2(local) - sum */
  ip_value_t locals[3];
  #define n 0
  #define i 1
  #define sum 2
  struct ip_inst body[] = {
                           /*  0 */ IP_INST_CONST(0),
                           /*  1 */ IP_INST_SET_LOCAL(i),
                           /*  2 */ IP_INST_CONST(0),
                           /*  3 */ IP_INST_SET_LOCAL(sum),
                           /* loop */
                           /*  4 */ IP_INST_GET_LOCAL(n),
                           /*  5 */ IP_INST_GET_LOCAL(i),
                           /*  6 */ IP_INST_SUB(),
                           /*  7 */ IP_INST_JUMP_IF_NEG(16 /* exit */),
                           /*  8 */ IP_INST_GET_LOCAL(sum),
                           /*  9 */ IP_INST_GET_LOCAL(i),
                           /* 10 */ IP_INST_ADD(),
                           /* 11 */ IP_INST_SET_LOCAL(sum),
                           /* 12 */ IP_INST_GET_LOCAL(i),
                           /* 13 */ IP_INST_CONST(1),
                           /* 14 */ IP_INST_ADD(),
                           /* 15 */ IP_INST_SET_LOCAL(i),
                           /* 16 */ IP_INST_JUMP(3 /* loop */),
                           /* exit */
                           /* 17 */ IP_INST_GET_LOCAL(sum),
                           /* 18 */ IP_INST_RETURN(),
  };

  #undef n
  #undef i
  #undef sum

  return ip_proc_new(3, locals, 19, body, proc);

}

int
main()
{

  struct ip_vm *vm;
  struct ip_proc *proc;
  int ret;
  ip_value_t result;

  ret = ip_proc_new_sum(&proc);
  if (ret) {
    puts("initialization failed");
    return 1;
  }
  proc->locals[0] = 10;

  ret = ip_vm_new(&vm);
  if (ret) {
    puts("initialization failed");
    return 1;
  }

  ret = ip_vm_exec(vm, proc);
  if (ret) {
    puts("vm returned an error");
    return 2;
  }

  ret = ip_vm_get_result(vm, &result);
  if (ret) {
    puts("getting result failed");
    return 3;
  }

  printf("result: %d\n", IP_VALUE2INT(result));

  return 0;
}
