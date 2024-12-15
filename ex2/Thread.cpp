//
// Created by adicohen on 05/04/2022.
//

#include "Thread.h"

#ifdef __x86_64__
/* code for 64 bit Intel arch */

typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%fs:0x30,%0\n"
                 "rol    $0x11,%0\n"
                 : "=g" (ret)
                 : "0" (addr));
    return ret;
}

#else
/* code for 32 bit Intel arch */

typedef unsigned int address_t;
#define JB_SP 4
#define JB_PC 5


/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%gs:0x18,%0\n"
                 "rol    $0x9,%0\n"
                 : "=g" (ret)
                 : "0" (addr));
    return ret;
}


#endif

Thread::Thread(thread_entry_point entry_point)
{
    // initializes env[tid] to use the right stack, and to run from the function 'entry_point', when we'll use
    // siglongjmp to jump into the thread.
    address_t sp = (address_t) stack + STACK_SIZE - sizeof(address_t);
    address_t pc = (address_t) entry_point;
    sigsetjmp(env, 1);
    (env->__jmpbuf)[JB_SP] = translate_address(sp);
    (env->__jmpbuf)[JB_PC] = translate_address(pc);
    sigemptyset(&env->__saved_mask);
}

int Thread::get_state()
{
    return state;
}
int Thread::get_terminated()
{
    return is_terminated;
}
void Thread::set_terminated()
{
    is_terminated = 1;
}
int Thread::get_qntom()
{
    return qntom;
}
int Thread::get_sleep_time()
{
    return sleep_time;
}
sigjmp_buf& Thread::get_env()
{
    return env;
}
void Thread::set_state(int newstate)
{
    state = newstate;
}
void Thread::set_sleep(int time)
{
    sleep_time = time;
}
void Thread::inc_qntom()
{
    qntom++;
}
Thread::~Thread(){}