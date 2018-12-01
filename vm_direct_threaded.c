#include "stack.h"
#include "vm.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


def_ip_stack(ip_value_t);

struct ip_inst_internal {
  void *label;
  union { ip_value_t v; int i; size_t pos; ip_proc_ref_t p;} u;
};


enum ip_vm_mode {
                 IP_VM_COMPILE,
                 IP_VM_EXEC,
};
union ip_vm_arg {
  struct {
    struct ip_vm *vm;
    ip_proc_ref_t procref;
  } exec;
  struct {
    size_t ninsts;
    struct ip_inst *insts;
    struct ip_inst_internal *result;
  } compile;
};



int ip_vm_main(enum ip_vm_mode mode, union ip_vm_arg arg);



struct ip_proc {
  size_t nargs;
  size_t nlocals;
  size_t ninsts;
  struct ip_inst_internal *insts;
};


int
ip_proc_init(struct ip_proc *proc, size_t nargs, size_t nlocals, size_t ninsts, struct ip_inst *insts)
{
  union ip_vm_arg arg;

  proc->insts = malloc(ninsts * sizeof(struct ip_inst_internal));
  if (NULL == proc->insts) {
    return 1;
  }

  proc->nargs = nargs;
  proc->nlocals = nlocals;
  proc->ninsts = ninsts;

  arg.compile.ninsts = ninsts;
  arg.compile.insts = insts;
  arg.compile.result = proc->insts;

  return ip_vm_main(IP_VM_COMPILE, arg);
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


typedef struct ip_callinfo {
  size_t ip;
  size_t fp;
  struct ip_proc *proc;
} ip_callinfo_t;


def_ip_stack(ip_callinfo_t);


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
  ip_stack(ip_callinfo_t) callstack;
  size_t nprocs;
  struct ip_proc **procs;
};

int
ip_vm_init(struct ip_vm *vm)
{
  int ret;

  ret = ip_stack_init(ip_value_t, &vm->stack, 1024);
  if (ret) {
    return 1;
  }

  ret = ip_stack_init(ip_callinfo_t, &vm->callstack, 1024);
  if (ret) {
    return 1;
  }

  vm->nprocs = 0;
  vm->procs = NULL;

  return 0;
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


ip_proc_ref_t
ip_vm_reserve_proc(struct ip_vm *vm)
{
  vm->nprocs += 1;
  vm->procs = realloc(vm->procs, vm->nprocs * sizeof(struct ip_proc *));
  if (NULL == vm->procs) {
    return -1;
  }

  return vm->nprocs - 1;
}

void
ip_vm_register_proc_at(struct ip_vm *vm, struct ip_proc *proc, ip_proc_ref_t at)
{
  vm->procs[at] = proc;
}



ip_proc_ref_t
ip_vm_register_proc(struct ip_vm *vm, struct ip_proc *proc)
{

  ip_proc_ref_t ret;

  ret = ip_vm_reserve_proc(vm);

  if (ret < 0) {
    return -1;
  }

  ip_vm_register_proc_at(vm, proc, ret);

  return ret;
}

void
ip_vm_dtor(struct ip_vm *vm)
{

  ip_stack_dtor(ip_value_t, &vm->stack);
  ip_stack_dtor(ip_callinfo_t, &vm->callstack);
}

int
ip_vm_exec(struct ip_vm *vm, ip_proc_ref_t procref)
{
  union ip_vm_arg arg;

  arg.exec.vm = vm;
  arg.exec.procref = procref;

  return ip_vm_main(IP_VM_EXEC, arg);
}

int
ip_vm_main(enum ip_vm_mode mode, union ip_vm_arg arg)
{
  static void *labels[] = {
                         &&L_CONST,
                         &&L_GET_LOCAL,
                         &&L_SET_LOCAL,
                         &&L_ADD,
                         &&L_SUB,
                         &&L_JUMP,
                         &&L_JUMP_IF_ZERO,
                         &&L_JUMP_IF_NEG,
                         &&L_CALL,
                         &&L_CALL_INDIRECT,
                         &&L_RETURN,
                         &&L_EXIT,
  };

  if (IP_VM_COMPILE == mode) {
    size_t i, ninsts = arg.compile.ninsts;
    struct ip_inst *insts = arg.compile.insts;
    struct ip_inst_internal * result = arg.compile.result;
    for(i = 0; i < ninsts; i++) {
      struct ip_inst_internal inst;
      inst.label = labels[insts[i].code];
      inst.u.v = insts[i].u.v;
      inst.u.i = insts[i].u.i;
      inst.u.pos = insts[i].u.pos;
      inst.u.p = insts[i].u.p;
      result[i] = inst;
    }
    return 0;
  }
  /* else exec */
  size_t ip = 0;
  size_t fp;
  struct ip_proc  *proc;
  struct ip_inst_internal inst;
  struct ip_vm *vm = arg.exec.vm;
  ip_proc_ref_t procref = arg.exec.procref;


#define LOCAL(i)      ip_stack_ref(ip_value_t, &vm->stack, fp - (proc->nargs + proc->nlocals) + i)
#define POP(ref)      do{if (ip_stack_pop(ip_value_t, &vm->stack, ref)) { return 1;}} while(0)
#define PUSH(v)       do{if (ip_stack_push(ip_value_t, &vm->stack, v)) { return 1;}} while(0)
#define POPN(n, ref)  do{size_t i; for (i = 0; i < (n); i++) POP(ref);} while(0)
#define PUSHN(n, v)   do{size_t i; for (i = 0; i < (n); i++) PUSH(v); } while(0)

  proc = vm->procs[procref];

  PUSHN(proc->nlocals, IP_LLINT2VALUE(0));
  fp = ip_stack_size(ip_value_t, &vm->stack);


#define JUMP()  do{inst = proc->insts[++ip]; goto *inst.label;} while(0);

  inst = proc->insts[ip];
  goto *inst.label;

 L_CONST: {
      ip_stack_push(ip_value_t, &vm->stack, inst.u.v);
      JUMP();
    }
 L_GET_LOCAL: {
      int i;
      ip_value_t v;

      i = inst.u.i;
      v = LOCAL(i);

      PUSH(v);
      JUMP();
    }
 L_SET_LOCAL: {
      int i;
      ip_value_t v;

      i = inst.u.i;
      POP(&v);

      LOCAL(i) = v;

      JUMP();
    }
 L_ADD: {
      ip_value_t v1, v2, ret;
      long long int x, y;

      POP(&v1);
      POP(&v2);
      y = IP_VALUE2LLINT(v1);
      x = IP_VALUE2LLINT(v2);

      ret = IP_INT2VALUE(x + y);

      PUSH(ret);

      JUMP();
    }
 L_SUB: {
      ip_value_t v1, v2, ret;
      long long int x, y;

      POP(&v1);
      POP(&v2);
      y = IP_VALUE2LLINT(v1);
      x = IP_VALUE2LLINT(v2);

      ret = IP_LLINT2VALUE(x - y);

      PUSH(ret);

      JUMP();
    }
 L_JUMP: {
      ip = inst.u.pos;
      JUMP();
    }
 L_JUMP_IF_ZERO: {
      ip_value_t v;

      POP(&v);

      if (!IP_VALUE2LLINT(v)) {
        ip = inst.u.pos;
      }
      JUMP();
    }
 L_JUMP_IF_NEG: {
      ip_value_t v;

      POP(&v);

      if (IP_VALUE2LLINT(v) < 0) {
        ip = inst.u.pos;
      }
      JUMP();
    }
 L_CALL: {
      int ret;
      ip_callinfo_t ci = {.ip = ip, .fp = fp, .proc = proc};

      ret = ip_stack_push(ip_callinfo_t, &vm->callstack, ci);
      if (ret) {
        return 1;
      }

      proc = vm->procs[inst.u.p];

      PUSHN(proc->nlocals, IP_LLINT2VALUE(0));

      ip = -1;
      fp = ip_stack_size(ip_value_t, &vm->stack);


      JUMP();
    }
 L_CALL_INDIRECT: {
      int ret;
      ip_value_t p;
      ip_callinfo_t ci = {.ip = ip, .fp = fp, .proc = proc};

      POP(&p);

      ret = ip_stack_push(ip_callinfo_t, &vm->callstack, ci);
      if (ret) {
        return 1;
      }

      proc = vm->procs[IP_VALUE2PROCREF(p)];

      PUSHN(proc->nlocals, IP_INT2VALUE(0));

      ip = -1;
      fp = ip_stack_size(ip_value_t, &vm->stack);


      JUMP();
    }
 L_RETURN: {
      int ret;
      ip_value_t v;
      ip_value_t ignore;
      ip_callinfo_t ci;

      POP(&v);

      POPN(proc->nlocals + proc->nargs, &ignore);

      PUSH(v);

      ret = ip_stack_pop(ip_callinfo_t, &vm->callstack, &ci);
      if (ret) {
        return 1;
      }

      ip = ci.ip;
      fp = ci.fp;
      proc = ci.proc;

      JUMP();
    }
 L_EXIT: {
      ip_value_t v;
      ip_value_t ignore;
      POP(&v);

      POPN(proc->nlocals + proc->nargs, &ignore);

      PUSH(v);

      return 0;
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

