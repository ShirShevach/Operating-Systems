#include "uthreads.h"
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <setjmp.h>
#include <vector>
#include <algorithm>
#include <sys/time.h>
#include <stdbool.h>
#include <map>
#include <iostream>
#include "Thread.h"


#define EROR_SYS "system error:"
#define EROR_THREAD "thread library error:"
#define READY 1
#define RUNNING 2
#define BLOCKED 3
#define SLEEP 4
#define SECOND 1000000

static int curr_id;
static int total_qntom;

static std::map<const int,Thread *> threads_list;
static std::vector<int> ready_list;
static std::vector<int> blocked_list;
static std::vector<int> sleep_list;
static struct sigaction sa = {nullptr};
static struct itimerval timer;

void block()
{
    if (sigprocmask(SIG_BLOCK,&sa.sa_mask,NULL))
    {
        std::cerr << EROR_SYS << "Signal mask is not blocked" << std::endl;
        exit (1);
    }
}

void unblock()
{
    if (sigprocmask(SIG_UNBLOCK,&sa.sa_mask,NULL))
    {
        std::cerr << EROR_SYS << "Signal mask is not unblocked" << std::endl;
        exit (1);
    }
}


void timer_handler (int signum)
{
    auto curr_thread = threads_list.find(curr_id);
    if (curr_thread == threads_list.end())
    {
        std::cerr << EROR_THREAD  << "The thread id doesn't exist in the threads map" << std::endl;
        return;
    }
    if (!sleep_list.empty())
    {
        for (int it: sleep_list)
        {
            if (threads_list[it]->get_sleep_time()<=total_qntom)
            {
                auto id = find(sleep_list.begin(),sleep_list.end(),it);
                sleep_list.erase(id);
                ready_list.push_back(it);
                threads_list[it]->set_state(READY);
            }
        }
    }
    if (curr_thread->second->get_terminated() == 0){
        if (sigsetjmp(curr_thread->second->get_env(),1) != 0)
        {
            return;
        }
    }
    if (curr_thread->second->get_state() == RUNNING && curr_thread->second->get_terminated() == 0)
    {
        ready_list.push_back(curr_id);
        curr_thread->second->set_state(READY);
    }
    if (!ready_list.empty())    {
        if (curr_thread->second->get_terminated() == 1)
        {
            delete curr_thread->second;
            curr_thread->second = nullptr;
            threads_list.erase(curr_thread);
        }
        curr_id = ready_list[0];
        threads_list[curr_id]->set_state(RUNNING);
        ready_list.erase(ready_list.begin());
    }
    total_qntom++;
    auto new_thread = threads_list.find(curr_id);
    new_thread->second->inc_qntom();
    siglongjmp(threads_list[curr_id]->get_env(), 1);
}

void free_data()
{
    for (auto it : threads_list)
    {
        delete it.second;
    }

}

/**
 * @brief initializes the thread library.
 *
 * You may assume that this function is called before any other thread library function, and that it is called
 * exactly once.
 * The input to the function is the length of a quantum in micro-seconds.
 * It is an error to call this function with non-positive quantum_usecs.
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_init(int quantum_usecs)
{
    if (quantum_usecs <=0)
    {
        std::cerr << EROR_THREAD << "quantom usecs must be positive" << std::endl;
        return -1;
    }
    sa.sa_handler = timer_handler;
    /*inits the mask to empty*/
    if (sigemptyset(&sa.sa_mask)){
        std::cerr << EROR_THREAD << "***" << std::endl;
        exit(1);
    }
    /*adds the virtual time to the mask*/
    if(sigaddset(&sa.sa_mask,SIGVTALRM)){
        std::cerr << EROR_THREAD << "***" << std::endl;
        exit(1);
    }
    curr_id = 0;
    total_qntom = 1;
    Thread *main_thread = new Thread(nullptr);
    threads_list[0] = main_thread;
    threads_list[0]->set_state(RUNNING);
    threads_list[0]->inc_qntom();
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = quantum_usecs;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = quantum_usecs;

    if (sigaction(SIGVTALRM, &sa, NULL) < 0)
    {
        std::cerr << EROR_SYS << "sigaction error" << std::endl;
        return -1;
    }
    if (setitimer(ITIMER_VIRTUAL, &timer, NULL))
    {
        std::cerr << EROR_SYS << "setitimer error" << std::endl;
    }
    return 0;
}

int get_id()
{
    for (int i =1; i < MAX_THREAD_NUM; i++)
    {
        auto it = threads_list.find(i);
        if (it == threads_list.end())
        {
            return i;
        }
    }
    return -1;
}


/**
 * @brief Creates a new thread, whose entry point is the function entry_point with the signature
 * void entry_point(void).
 *
 * The thread is added to the end of the READY threads list.
 * The uthread_spawn function should fail if it would cause the number of concurrent threads to exceed the
 * limit (MAX_THREAD_NUM).
 * Each thread should be allocated with a stack of size STACK_SIZE bytes.
 *
 * @return On success, return the ID of the created thread. On failure, return -1.
*/
int uthread_spawn(thread_entry_point entry_point)
{
    block();
    if (entry_point == nullptr)
    {
        std::cerr<<EROR_SYS<<" entry point can't be null"<<std::endl;
        unblock();
        return -1;
    }

    if (threads_list.size() >= MAX_THREAD_NUM)
    {
        std::cerr << EROR_THREAD << "Exceed the limit MAX_THREAD_NUM" <<
                  std::endl;
        unblock();
        return -1;
    }

    Thread *new_thread = new (std::nothrow) Thread(entry_point);
    if (new_thread == NULL)
    {
        std::cerr << EROR_THREAD << "Error memory allocation" <<
                  std::endl;
        unblock();
        return -1;
    }
    int id = get_id();
    ready_list.push_back(id);
    threads_list[id] = new_thread;
    unblock();
    return id;
}


/**
 * @brief Terminates the thread with ID tid and deletes it from all relevant control structures.
 *
 * All the resources allocated by the library for this thread should be released. If no thread with ID tid exists it
 * is considered an error. Terminating the main thread (tid == 0) will result in the termination of the entire
 * process using exit(0) (after releasing the assigned library memory).
 *
 * @return The function returns 0 if the thread was successfully terminated and -1 otherwise. If a thread terminates
 * itself or the main thread is terminated, the function does not return.
*/
int uthread_terminate(int tid)
{
    block();
    if (tid == 0)
    {
        free_data();
        unblock();
        exit(0);
    }
    if (tid >= MAX_THREAD_NUM || tid < 0){
        std::cerr << EROR_THREAD << "The thread id is invalid" <<
                  std::endl;
        unblock();
        return -1;
    }
    auto it = threads_list.find(tid);
    if (it == threads_list.end()){
        std::cerr << EROR_THREAD << "The thread id doesn't exist in the threads map" <<
                  std::endl;
        unblock();
        return -1;
    }

    it->second->set_terminated();

    if (it->second->get_state()== RUNNING)
    {
        unblock();
        timer_handler(0);
        return 0;
    }
    else if (it->second->get_state() == READY)
    {
        auto it_id = find(ready_list.begin(),ready_list.end(),it->first);
        ready_list.erase(it_id);
    }
    else if (it->second->get_state() == BLOCKED)
    {
        auto it_id = find(blocked_list.begin(),blocked_list.end(),it->first);
        blocked_list.erase(it_id);
    }
    else if (it->second->get_state() == SLEEP)
    {
        auto it_id = find(sleep_list.begin(),sleep_list.end(),it->first);
        sleep_list.erase(it_id);
    }
    delete it->second;
    it->second = nullptr;
    threads_list.erase(it);
    unblock();
    return 0;
}


/**
 * @brief Blocks the thread with ID tid. The thread may be resumed later using uthread_resume.
 *
 * If no thread with ID tid exists it is considered as an error. In addition, it is an error to try blocking the
 * main thread (tid == 0). If a thread blocks itself, a scheduling decision should be made. Blocking a thread in
 * BLOCKED state has no effect and is not considered an error.
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_block(int tid)
{
    block();
    auto it = threads_list.find(tid);
    if (tid >= MAX_THREAD_NUM || tid < 0){
        std::cerr << EROR_THREAD << "The thread id is invalid" <<
                  std::endl;
        unblock();
        return -1;
    }
    if (it == threads_list.end()){
        std::cerr << EROR_THREAD << "The thread id doesn't exist in the threads map block" <<
                  std::endl;
        unblock();
        return -1;
    }
    if (it->first == 0)
    {
        std::cerr << EROR_THREAD << "It's illegal to block the main thread" <<  std::endl;
        unblock();
        return -1;
    }
    if (it->second->get_state() == READY)
    {
        auto it_id = find(ready_list.begin(),ready_list.end(),it->first);
        ready_list.erase(it_id);
        blocked_list.push_back(tid);
        it->second->set_state(BLOCKED); //check not to put twice the same!!!
    }
    if (it->second->get_state() == RUNNING)
    {
        blocked_list.push_back(tid);
        it->second->set_state(BLOCKED); //check not to put twice the same!!!
        timer_handler(0);
    }
    unblock();
    return 0;
}


/**
 * @brief Resumes a blocked thread with ID tid and moves it to the READY state.
 *
 * Resuming a thread in a RUNNING or READY state has no effect and is not considered as an error. If no thread with
 * ID tid exists it is considered an error.
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_resume(int tid)
{
    block();
    auto it = threads_list.find(tid);
    if (tid >= MAX_THREAD_NUM || tid < 0){
        std::cerr << EROR_THREAD << "The thread id is invalid" <<
                  std::endl;
        unblock();
        return -1;
    }
    if (it == threads_list.end()){
        std::cerr << EROR_THREAD << "The thread id doesn't exist in the threads map resume" <<
                  std::endl;
        unblock();
        return -1;
    }
    if (threads_list[tid]->get_state() == BLOCKED)
    {
        auto it_id = find(blocked_list.begin(),blocked_list.end(),it->first);
        blocked_list.erase(it_id);
        ready_list.push_back(tid);
        it->second->set_state(READY);
        unblock();
        return 0;
    }
    if (threads_list[tid]->get_state() == READY || threads_list[tid]->get_state() == RUNNING)
    {
        unblock();
        return 0;
    }
    unblock();
    return -1;
}


/**
 * @brief Blocks the RUNNING thread for num_quantums quantums.
 *
 * Immediately after the RUNNING thread transitions to the BLOCKED state a scheduling decision should be made.
 * After the sleeping time is over, the thread should go back to the end of the READY threads list.
 * The number of quantums refers to the number of times a new quantum starts, regardless of the reason. Specifically,
 * the quantum of the thread which has made the call to uthread_sleep isn’t counted.
 * It is considered an error if the main thread (tid==0) calls this function.
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_sleep(int num_quantums)
{
    block();
    if (curr_id == 0)
    {
        std::cerr << EROR_SYS << "Can't call the main thread" << std::endl;
        unblock();
        return  -1;
    }
    threads_list[curr_id]->set_state(SLEEP);
    threads_list[curr_id]->set_sleep(num_quantums + total_qntom);
    sleep_list.push_back(curr_id);
    timer_handler(0);
    unblock();
    return 0;
}


/**
 * @brief Returns the thread ID of the calling thread.
 *
 * @return The ID of the calling thread.
*/
int uthread_get_tid()
{
    return curr_id;
}


/**
 * @brief Returns the total number of quantums since the library was initialized, including the current quantum.
 *
 * Right after the call to uthread_init, the value should be 1.
 * Each time a new quantum starts, regardless of the reason, this number should be increased by 1.
 *
 * @return The total number of quantums.
*/
int uthread_get_total_quantums()
{
    return  total_qntom;
}


/**
 * @brief Returns the number of quantums the thread with ID tid was in RUNNING state.
 *
 * On the first time a thread runs, the function should return 1. Every additional quantum that the thread starts should
 * increase this value by 1 (so if the thread with ID tid is in RUNNING state when this function is called, include
 * also the current quantum). If no thread with ID tid exists it is considered an error.
 *
 * @return On success, return the number of quantums of the thread with ID tid. On failure, return -1.
*/
int uthread_get_quantums(int tid)
{
    auto it = threads_list.find(tid);
    if (tid >= MAX_THREAD_NUM || tid < 0){
        std::cerr << EROR_THREAD << "The thread id is invalid" <<
                  std::endl;
        unblock();
        return -1;
    }
    if (it == threads_list.end()){
        std::cerr << EROR_THREAD << "The thread id doesn't exist in the threads map uthread_get_quantums" <<
                  std::endl;
        fflush (stdout);
        unblock();
        return -1;
    }
    return it->second->get_qntom();
}
