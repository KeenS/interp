#ifndef IP_H_STACK
#define IP_H_STACK

/* pseudo generics. */
/* the stack is empty stack */
#define def_ip_stack(ty)                                        \
  typedef struct ip_stack_##ty {                                \
    size_t size;                                                \
    size_t sp;                                                  \
    ty *data;                                                   \
  } ip_stack_##ty##_t;                                          \
  static int                                                    \
  ip_stack_init_##ty(struct ip_stack_##ty *stack, size_t size)  \
  {                                                             \
    stack->data = malloc(size * sizeof(ty));                    \
    if (NULL == stack->data) {                                  \
      return 1;                                                 \
    }                                                           \
    stack->size = size;                                         \
    stack->sp = 0;                                              \
    return 0;                                                   \
  }                                                             \
                                                                \
  static int                                                    \
  ip_stack_new_##ty(size_t size, struct ip_stack_##ty **ret)    \
  {                                                             \
    *ret = malloc(sizeof(struct ip_stack_##ty));                \
    if(NULL == *ret) {                                          \
      return 1;                                                 \
    }                                                           \
                                                                \
    return ip_stack_init_##ty(*ret, size);                      \
  }                                                             \
                                                                \
  static void                                                   \
  ip_stack_dtor_##ty(struct ip_stack_##ty *stack)               \
  {                                                             \
    free(stack->data);                                          \
  }                                                             \
                                                                \
                                                                \
  static size_t                                                 \
  ip_stack_size_##ty(struct ip_stack_##ty * stack)              \
  {                                                             \
    return stack->sp;                                           \
  }                                                             \
                                                                \
                                                                \
  static int                                                    \
  ip_stack_push_##ty(struct ip_stack_##ty * stack, ty v)        \
  {                                                             \
    if (stack->sp < stack->size) {                              \
      stack->data[stack->sp++] = v;                             \
      return 0;                                                 \
    } else {                                                    \
      return 1;                                                 \
    }                                                           \
  }                                                             \
                                                                \
  static int                                                    \
  ip_stack_pop_##ty(struct ip_stack_##ty * stack, ty * v)       \
  {                                                             \
    if (0 < stack->sp) {                                        \
      *v = stack->data[--stack->sp];                            \
      return 0;                                                 \
    } else {                                                    \
      return 1;                                                 \
    }                                                           \
  }                                                             \


#define ip_stack(ty)                    ip_stack_##ty##_t
#define ip_stack_init(ty, stack, size)  ip_stack_init_##ty(stack, size)
#define ip_stack_new(ty, size, ret)     ip_stack_new_##ty(size, ret)
#define ip_stack_dtor(ty, stack)        ip_stack_dtor_##ty(stack)
#define ip_stack_size(ty, stack)        ip_stack_size_##ty(stack)
#define ip_stack_push(ty, stack, v)     ip_stack_push_##ty(stack, v)
#define ip_stack_pop(ty, stack, ref)    ip_stack_pop_##ty(stack, ref)
#define ip_stack_ref(ty, stack, i)      ((stack)->data[i])

#endif
