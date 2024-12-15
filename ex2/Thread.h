//
// Created by adicohen on 05/04/2022.
//

#ifndef UTHREADS_CPP_THREAD_H
#define UTHREADS_CPP_THREAD_H
#define STACK_SIZE 4096 /* stack size per thread (in bytes) */

#include <setjmp.h>
#include "uthreads.h"
#include <signal.h>


class Thread {

private:
    char stack[STACK_SIZE];
    int state =1;
    int qntom = 0;
    int sleep_time = 0;
    int is_terminated = 0;
    sigjmp_buf env;

public:
    Thread(thread_entry_point entry_point);
    int get_state();
    int get_qntom();
    int get_sleep_time();
    int get_terminated();
    void set_terminated();
    sigjmp_buf& get_env();
    void set_state(int);
    void set_sleep(int);
    void inc_qntom();
    ~Thread();






};


#endif //UTHREADS_CPP_THREAD_H
