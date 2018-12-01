#include "vm.h"
#include <stdio.h>


ip_proc_ref_t
ip_register_sum(struct ip_vm *vm)
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

  int ret;
  struct ip_proc *proc;

  ret = ip_proc_new(nargs, nlocals, sizeof(body)/sizeof(body[0]), body, &proc);
  if (ret) {
    return -1;
  }

  return ip_vm_register_proc(vm, proc);

}

ip_proc_ref_t
ip_register_fib(struct ip_vm *vm)
{


  ip_proc_ref_t fib;

  /* registering proc first to recursive call */
  /* creating en empty proc */


  fib = ip_vm_reserve_proc(vm);
  if (fib < 0) {
    return fib;
  }

  /* 0(arg)   - n */
  size_t nargs = 1;
  size_t nlocals = 0;
  #define n 0
  struct ip_inst body[] = {
                           /*  0 */ IP_INST_CONST(1),
                           /*  1 */ IP_INST_GET_LOCAL(n),
                           /*  2 */ IP_INST_SUB(),
                           /*  3 */ IP_INST_JUMP_IF_NEG(5/* else */),
                           /* then */
                           /*  4 */ IP_INST_CONST(1),
                           /*  5 */ IP_INST_RETURN(),
                           /* else */
                           /*  6 */ IP_INST_GET_LOCAL(n),
                           /*  7 */ IP_INST_CONST(1),
                           /*  8 */ IP_INST_SUB(),
                           /*  9 */ IP_INST_CALL(fib),
                           /* 10 */ IP_INST_GET_LOCAL(n),
                           /* 11 */ IP_INST_CONST(2),
                           /* 12 */ IP_INST_SUB(),
                           /* 13 */ IP_INST_CALL(fib),
                           /* 14 */ IP_INST_ADD(),
                           /* 15 */ IP_INST_RETURN(),
  };

  #undef n

  int ret;
  struct ip_proc *proc;

  ret = ip_proc_new(nargs, nlocals, sizeof(body)/sizeof(body[0]), body, &proc);
  if (ret) {
    return -1;
  }

  ip_vm_register_proc_at(vm, proc, fib);

  return fib;

}

ip_proc_ref_t
ip_register_entry(struct ip_vm *vm)
{
  size_t nargs = 2;
  size_t nlocals = 0;
  #define n 0
  #define callee 1
  struct ip_inst body[] = {
                           /*  0 */ IP_INST_GET_LOCAL(n),
                           /*  1 */ IP_INST_GET_LOCAL(callee),
                           /*  2 */ IP_INST_CALL_INDIRECT(),
                           /*  3 */ IP_INST_EXIT(),
  };

  #undef n
  #undef callee

  int ret;
  struct ip_proc *proc;

  ret = ip_proc_new(nargs, nlocals, sizeof(body)/sizeof(body[0]), body, &proc);
  if (ret) {
    return -1;
  }

  return ip_vm_register_proc(vm, proc);

}

int
main()
{

  struct ip_vm *vm;
  int ret;
  ip_proc_ref_t sum, fib, entry;
  ip_value_t result;

  ret = ip_vm_new(&vm);
  if (ret) {
    puts("initialization failed");
    return 1;
  }

  sum = ip_register_sum(vm);
  if (sum < 0) {
    puts("proc registration failed: sum");
    return 1;
  }

  fib = ip_register_fib(vm);
  if (fib < 0) {
    puts("proc registration failed: fib");
    return 1;
  }

  entry = ip_register_entry(vm);
  if (entry < 0) {
    puts("proc registration failed: main");
    return 1;
  }


  /* call fib */
  if (ip_vm_push_arg(vm, IP_INT2VALUE(33))) {
    puts("initialization failed");
    return 1;
  }

  if (ip_vm_push_arg(vm, IP_PROCREF2VALUE(fib))) {
    puts("initialization failed");
    return 1;
  }

  ret = ip_vm_exec(vm, entry);
  if (ret) {
    puts("vm returned an error");
    return 2;
  }

  ret = ip_vm_get_result(vm, &result);
  if (ret) {
    puts("getting result failed");
    return 3;
  }

  printf("result of fib: %d\n", IP_VALUE2INT(result));
  /* end fib */


  /* call sum */
  if (ip_vm_push_arg(vm, IP_INT2VALUE(10000000))) {
    puts("initialization failed");
    return 1;
  }

  if (ip_vm_push_arg(vm, IP_PROCREF2VALUE(sum))) {
    puts("initialization failed");
    return 1;
  }

  ret = ip_vm_exec(vm, entry);
  if (ret) {
    puts("vm returned an error");
    return 2;
  }

  ret = ip_vm_get_result(vm, &result);
  if (ret) {
    puts("getting result failed");
    return 3;
  }

  printf("result of sum: %lld\n", IP_VALUE2LLINT(result));
  /* end sum */

  ip_vm_dtor(vm);


  return 0;
}
