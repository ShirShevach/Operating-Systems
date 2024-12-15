#include "MapReduceFramework.h"
#include <pthread.h>
#include <atomic>
#include <iostream>
#include <semaphore.h>
#include "Barrier.h"
#include <vector>
#include <map>
#include <algorithm>
#include "JobContext.h"

#define SYS_ERROR "system error: "

typedef struct Context {
    int threadID;
    JobContext *jobContext;
} Context;

void DestroyAndDelete(pthread_mutex_t* handler_mutex)
{
    if (pthread_mutex_destroy(handler_mutex) != 0) {
        std::cerr << SYS_ERROR << "problem delete mutex" << std::endl;
        exit(1);
    }
    delete handler_mutex;
}
void mutex_lock(pthread_mutex_t* handler_mutex){
    if (pthread_mutex_lock(handler_mutex) != 0)
    {
        std::cerr << SYS_ERROR << "error in lock mutex" << std::endl;
        exit(1);
    }
}
void mutex_unlock(pthread_mutex_t* handler_mutex){
    if (pthread_mutex_unlock(handler_mutex) != 0)
    {
        std::cerr << SYS_ERROR << "error in unlock mutex" << std::endl;
        exit(1);
    }
}
void emit2 (K2* key, V2* value, void* context)
{
    JobContext *ctx = (JobContext*) context;
    IntermediatePair pair = std::make_pair(key, value);
    auto vec = ctx->get_interVec()->at(ctx->get_threadid());
    vec->push_back(pair);
}

void map_phase(JobContext* context)
{
    JobContext* jc = context;
    mutex_lock(jc[0].mutex5);
    unsigned long otom = (*(jc->get_atomicCounter()))++;
    mutex_unlock(jc[0].mutex5);
    while (otom < jc->get_inputVec()->size()){
        K1* key = jc->get_inputVec()->at(otom).first;
        V1* value = jc->get_inputVec()->at(otom).second;

        jc->get_client()->map(key,value,context);

        (*(jc->get_atomicCounter1()))++;
        otom = (*(jc->get_atomicCounter()))++;

    }
    (*(jc->get_atomicCounter()))--;
}

bool cmp(IntermediatePair p1, IntermediatePair p2) {
    return *p1.first < *p2.first;
}
void sort_phase(JobContext* handler)
{
    auto curr_vec = handler->get_interVec()->at(handler->get_threadid());
    std::sort(curr_vec->begin(), curr_vec->end(), cmp);
}

K2 *max_key(JobContext *jc) {
    K2* max_k = nullptr;
    for (int i=0;i<jc->get_numThreads();i++)
    {
        if (!(jc->get_interVec()->at(i)->empty()))
        {
            K2* key = jc->get_interVec()->at(i)->back().first;
            if (max_k == nullptr || (*max_k) < (*key))
            {
                max_k = key;
            }
        }
    }
    return max_k;
}

int count_pairs(std::vector<IntermediateVec*> *interVec) {
    int sum_p = 0;
    for (long unsigned int i =0; i<interVec->size(); i++)
    {
        sum_p += interVec->at(i)->size();
    }
    return sum_p;
}

void shuffle_phase(JobContext* context){
    if (context->get_threadid() == 0)
    {
        K2 * k_max = max_key(context);
        while (k_max != nullptr)
        {
            IntermediateVec *new_inter = new IntermediateVec;
            for (int i=0; i<context->get_numThreads(); i++) {
                auto curr_vec = context->get_interVec()->at(i);
                while ((!curr_vec->empty()) && !((*curr_vec->back().first) < (*k_max)) && !((*k_max) < (*curr_vec->back().first))) {
                    new_inter->push_back(curr_vec->back());
                    curr_vec->pop_back();
                }
            }
            context->push_shuffle(new_inter);
            context->inc_shuffle_pairs(new_inter->size());
            k_max = max_key(context);
        }
    }
}


void emit3 (K3* key, V3* value, void* context)
{
    JobContext* jc = (JobContext*) context;
    mutex_lock(jc[0].mutex2);
    JobContext *ctx = (JobContext*) context;
    OutputPair pair = std::make_pair(key,value);
    ctx->push_outputVec(pair);
    mutex_unlock(jc[0].mutex2);
}

void reduce_phase(JobContext* context) {

    JobContext* jc = context;

    while(!(jc->get_shuffle()->empty())) {
        mutex_lock(jc[0].mutex);
        if (jc->get_shuffle()->empty())
        {
            mutex_unlock(jc[0].mutex);
            return;
        }
        IntermediateVec *curr_vec = jc->get_shuffle()->back();
        jc->get_shuffle()->pop_back();
        mutex_unlock(jc[0].mutex);
        jc->get_client()->reduce(curr_vec, jc);
        *(jc->get_atomicCounter2()) += curr_vec->size();
        delete curr_vec;
    }
}
void *thread_func(void * context)
{
    auto cntx = (JobContext*) context;
    cntx->set_stage(MAP_STAGE);
    map_phase(cntx);
    sort_phase(cntx);
    cntx->activate_barrier();
    mutex_lock(cntx[0].mutex3);
    cntx->set_numpairs(count_pairs(cntx->get_interVec()));
    cntx->set_stage(SHUFFLE_STAGE);
    cntx->set_percentage(0);
    mutex_unlock(cntx[0].mutex3);
    shuffle_phase(cntx);
    cntx->activate_barrier();
    mutex_lock(cntx[0].mutex6);
    cntx->set_stage(REDUCE_STAGE);
    cntx->set_percentage(0);
    mutex_unlock(cntx[0].mutex6);
    reduce_phase(cntx);
    return nullptr;
}

JobHandle startMapReduceJob(const MapReduceClient& client,
                            const InputVec& inputVec, OutputVec& outputVec,
                            int multiThreadLevel)
                            {
    int num_threads = multiThreadLevel;
    pthread_t *threads = new pthread_t[num_threads];
    JobState *job_state = new JobState;
    job_state->stage = UNDEFINED_STAGE;
    job_state->percentage = 0;
    pthread_mutex_t *mutex = new pthread_mutex_t ();
    *mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t *mutex1 = new pthread_mutex_t ();
    *mutex1 = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t *mutex2 = new pthread_mutex_t ();
    *mutex2 = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t *mutex3 = new pthread_mutex_t ();
    *mutex3 = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t *mutex4 = new pthread_mutex_t ();
    *mutex4 = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t *mutex5 = new pthread_mutex_t ();
    *mutex5 = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t *mutex6 = new pthread_mutex_t ();
    *mutex6 = PTHREAD_MUTEX_INITIALIZER;

    std::vector<IntermediateVec *> *interVec = new std::vector<IntermediateVec *>;
    for (int i =0; i<num_threads; i++)
    {
        auto new_vec = new IntermediateVec;
        interVec->push_back(new_vec);
    }
    bool is_wait = false;
    std::atomic<uint32_t>* atomic_counter = new std::atomic<uint32_t>(0);
    std::atomic<uint32_t>* atomic_counter1 = new std::atomic<uint32_t>(0);
    std::atomic<uint32_t>* atomic_counter2 = new std::atomic<uint32_t>(0);

    Barrier *barrier = new Barrier(num_threads);
    std::vector<IntermediateVec *> *shuffle = new std::vector<IntermediateVec *>;
    JobContext* jc = new JobContext[num_threads];

    for (int i=0; i<num_threads; i++){
        jc[i].set_id(i);
        jc[i].set_num_thread(num_threads);
        jc[i].set_client(&client);
        jc[i].set_inputvec(&inputVec);
        jc[i].set_outputvec(&outputVec);
        jc[i].set_jobstate(job_state);
        jc[i].set_threads(threads);
        jc[i].set_atomic(atomic_counter);
        jc[i].set_atomic1(atomic_counter1);
        jc[i].set_atomic2(atomic_counter2);
        jc[i].set_barrier(barrier);
        jc[i].set_intervec(interVec);
        jc[i].set_shuffle(shuffle);
        jc[i].mutex = mutex;
        jc[i].mutex1 = mutex1;
        jc[i].mutex2 = mutex2;
        jc[i].mutex3 = mutex3;
        jc[i].mutex4 = mutex4;
        jc[i].mutex5 = mutex5;
        jc[i].mutex6 = mutex6;
    }

    for (int i = 0; i < num_threads; i++) {
        if (pthread_create(threads + i, nullptr, thread_func, jc + i) != 0)
        {
            std::cerr << SYS_ERROR << "Threads Error" << std::endl;
            exit(1);
        }
    }
    JobHandle jobhandle = static_cast <JobHandle> (jc);
    return jobhandle;
                            }

void waitForJob(JobHandle job)
{
    JobContext* handler = (JobContext*) job;
    mutex_lock(handler[0].mutex1);
    if (handler->iswait())
    {
        mutex_unlock(handler[0].mutex1);
        return;
    }

    if (!handler->iswait())
    {
        mutex_unlock(handler[0].mutex1);
        for (int i=0;i< handler->get_numThreads();i++)
        {
            if (pthread_join(handler[0].get_threads()[i], nullptr) != 0)
            {
                std::cerr << SYS_ERROR << "Threads error join" << std::endl;
                exit(1);
            }
        }
        handler->set_wait(true);
    }
}

void getJobState(JobHandle job, JobState* state){

    JobContext* handler = (JobContext*) job;

    mutex_lock(handler[0].mutex4);
    mutex_lock(handler[0].mutex1);
    mutex_lock(handler[0].mutex2);
    mutex_lock(handler[0].mutex6);
    mutex_lock(handler[0].mutex3);
    mutex_lock(handler[0].mutex5);
    mutex_lock(handler[0].mutex);

    if( handler->get_stage() == UNDEFINED_STAGE )
    {
        handler->set_percentage(0);
    }
    else if( handler->get_stage() == MAP_STAGE )
    {
        float new_per = 100.0 * (*handler->get_atomicCounter1()) / handler->get_inputVec()->size();
        handler->set_percentage(new_per);
    }

    else if (handler->get_stage() == SHUFFLE_STAGE && handler->get_allpairs() != 0)
    {
        float new_per = 100.0 * (handler->get_shuffle_pairs()/ handler->get_allpairs());
        handler->set_percentage(new_per);
    }
    else if (handler->get_stage() == REDUCE_STAGE && handler->get_allpairs() != 0)
    {
    float new_per = 100.0 * (*(handler->get_atomicCounter2()) / handler->get_allpairs());
        handler->set_percentage(new_per);

    }
    state->percentage = handler->get_percentage();
    state->stage = handler->get_stage();

    mutex_unlock(handler[0].mutex);
    mutex_unlock(handler[0].mutex5);
    mutex_unlock(handler[0].mutex3);
    mutex_unlock(handler[0].mutex6);
    mutex_unlock(handler[0].mutex2);
    mutex_unlock(handler[0].mutex1);
    mutex_unlock(handler[0].mutex4);
}


void closeJobHandle(JobHandle job)
{
    waitForJob(job);
    JobContext* handler = (JobContext*) job;
    delete handler[0].get_jobstate();
    delete[] handler[0].get_threads();
    delete handler[0].get_atomicCounter();
    delete handler[0].get_atomicCounter1();
    delete handler[0].get_atomicCounter2();
    delete handler[0].get_barrier();
    for (long unsigned int  i=0; i < handler[0].get_interVec()->size(); i++) {
        delete handler[0].get_interVec()->at(i);
    }
    delete handler[0].get_interVec();
    delete handler[0].get_shuffle();

    DestroyAndDelete(handler[0].mutex);
    DestroyAndDelete(handler[0].mutex2);
    DestroyAndDelete(handler[0].mutex3);
    DestroyAndDelete(handler[0].mutex4);
    DestroyAndDelete(handler[0].mutex1);
    DestroyAndDelete(handler[0].mutex5);
    DestroyAndDelete(handler[0].mutex6);
    delete[] handler;
}


