#include "inst.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* pseudo generics. */
/* the stack is empty stack */
#define def_ip_stack(ty)                                                \
  typedef struct ip_stack_##ty {                                        \
    size_t size;                                                        \
    size_t sp;                                                          \
    ty *data;                                                           \
  } ip_stack_##ty##_t;                                                  \
    int                                                                 \
    ip_stack_init_##ty(struct ip_stack_##ty *stack, size_t size)        \
    {                                                                   \
      stack->data = malloc(size * sizeof(ty));                          \
      if (NULL == stack->data) {                                        \
        return 1;                                                       \
      }                                                                 \
      stack->size = size;                                               \
      stack->sp = 0;                                                    \
      return 0;                                                         \
    }                                                                   \
                                                                        \
    int                                                                 \
    ip_stack_new_##ty(size_t size, struct ip_stack_##ty **ret)          \
    {                                                                   \
      *ret = malloc(sizeof(struct ip_stack_##ty));                      \
      if(NULL == *ret) {                                                \
        return 1;                                                       \
      }                                                                 \
                                                                        \
      return ip_stack_init_##ty(*ret, size);                            \
    }                                                                   \
                                                                        \
    void                                                                \
    ip_stack_dtor_##ty(struct ip_stack_##ty *stack)                     \
    {                                                                   \
      free(stack->data);                                                \
    }                                                                   \
                                                                        \
                                                                        \
    size_t                                                              \
    ip_stack_size_##ty(struct ip_stack_##ty * stack)                    \
    {                                                                   \
      return stack->sp;                                                 \
    }                                                                   \
                                                                        \
                                                                        \
    int                                                                 \
    ip_stack_push_##ty(struct ip_stack_##ty * stack, ty v)              \
    {                                                                   \
      if (stack->sp < stack->size) {                                    \
        stack->data[stack->sp++] = v;                                   \
        return 0;                                                       \
      } else {                                                          \
        return 1;                                                       \
      }                                                                 \
    }                                                                   \
                                                                        \
    int                                                                 \
    ip_stack_pop_##ty(struct ip_stack_##ty * stack, ty * v)             \
    {                                                                   \
      if (0 < stack->sp) {                                              \
        *v = stack->data[--stack->sp];                                  \
        return 0;                                                       \
      } else {                                                          \
        return 1;                                                       \
      }                                                                 \
    }                                                                   \


#define ip_stack(ty)                    ip_stack_##ty##_t
#define ip_stack_init(ty, stack, size)  ip_stack_init_##ty(stack, size)
#define ip_stack_new(ty, size, ret)     ip_stack_new_##ty(size, ret)
#define ip_stack_dtor(ty, stack)        ip_stack_dtor_##ty(stack)
#define ip_stack_size(ty, stack)        ip_stack_size_##ty(stack)
#define ip_stack_push(ty, stack, v)     ip_stack_push_##ty(stack, v)
#define ip_stack_pop(ty, stack, ref)    ip_stack_pop_##ty(stack, ref)
#define ip_stack_ref(ty, stack, i)      ((stack)->data[i])

def_ip_stack(ip_value_t);


struct ip_proc {
  size_t nargs;
  size_t nlocals;
  size_t ninsts;
  struct ip_inst *insts;
};


int
ip_proc_init(struct ip_proc *proc, size_t nargs, size_t nlocals, size_t ninsts, struct ip_inst *insts)
{
  proc->insts = malloc(ninsts * sizeof(struct ip_inst));
  if (NULL == proc->insts) {
    return 1;
  }

  proc->nargs = nargs;
  proc->nlocals = nlocals;
  proc->ninsts = ninsts;
  memcpy(proc->insts,   insts, ninsts  * sizeof(struct ip_inst));

  return 0;
}

int
ip_proc_new(size_t nargs, size_t nlocals, size_t ninsts, struct ip_inst *insts, struct ip_proc **ret)
{
  *ret = malloc(sizeof(struct ip_proc));
  if(NULL == *ret) {
    return 1;
  }

  return ip_proc_init(*ret, nargs, nlocals, ninsts, insts);
}

void
ip_proc_dtor(struct ip_proc *proc)
{
  free(proc->insts);
}


/**
 * stack usage
 *               previous sp        fp             sp
 *                     v            v              v
 *        --+----------+------------+--------------
 * bottom <-| args ... | locals ... | data | data | -> top
 *        --+----------+------------+--------------
 */
struct ip_vm {
  ip_stack(ip_value_t) stack;
};

int
ip_vm_init(struct ip_vm *vm)
{
  return ip_stack_init(ip_value_t, &vm->stack, 1024);
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
  size_t ip = 0;
  size_t fp;

  for (size_t i = 0; i < proc->nlocals; i++) {
    if (ip_stack_push(ip_value_t, &vm->stack, IP_INT2VALUE(0))) {
      return 1;
    }
  }
  fp = ip_stack_size(ip_value_t, &vm->stack);


#define LOCAL(i)  ip_stack_ref(ip_value_t, &vm->stack, fp - (proc->nargs + proc->nlocals) + i)
#define POP(ref)  do{if (ip_stack_pop(ip_value_t, &vm->stack, ref)) { return 1;}} while(0)
#define PUSH(v)   do{if (ip_stack_push(ip_value_t, &vm->stack, v)) { return 1;}} while(0)


  while (1) {
    struct ip_inst inst = proc->insts[ip];
    switch(inst.code) {
    case IP_CODE_CONST: {
      ip_stack_push(ip_value_t, &vm->stack, inst.u.v);
      break;
    }
    case IP_CODE_GET_LOCAL: {
      int i;
      ip_value_t v;

      i = inst.u.i;
      v = LOCAL(i);

      PUSH(v);
      break;
    }
    case IP_CODE_SET_LOCAL: {
      int i;
      ip_value_t v;

      i = inst.u.i;
      POP(&v);

      LOCAL(i) = v;

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
      ip_value_t v;
      ip_value_t ignore;
      POP(&v);

      for (size_t i = 0; i < proc->nlocals + proc->nargs; i++) {
        POP(&ignore);
      }

      PUSH(v);

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
ip_vm_push_arg(struct ip_vm *vm, ip_value_t arg)
{
  return ip_stack_push(ip_value_t, &vm->stack, arg);
}

int
ip_vm_get_result(struct ip_vm *vm, ip_value_t * result)
{
  return ip_stack_pop(ip_value_t, &vm->stack, result);
}

int
ip_proc_new_sum(struct ip_proc **proc)
{
  /* 0(arg)   - n */
  /* 1(local) - i */
  /* 2(local) - sum */
  size_t nargs = 1;
  size_t nlocals = 2;
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

  return ip_proc_new(nargs, nlocals, 19, body, proc);

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

  ret = ip_vm_new(&vm);
  if (ret) {
    puts("initialization failed");
    return 1;
  }

  if (ip_vm_push_arg(vm, IP_INT2VALUE(10))) {
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
