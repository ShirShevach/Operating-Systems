#ifndef MAPREDUCEFRAMEWORK_CPP_JOBCONTEXT_H
#define MAPREDUCEFRAMEWORK_CPP_JOBCONTEXT_H
#include "MapReduceClient.h"
#include "MapReduceFramework.h"
#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include "Barrier.h"
#include <atomic>


class JobContext{
public:
    JobContext() {};
    ~JobContext() {};
public:


    void set_id (int id){thread_id = id;}
    void set_num_thread(int num){num_threads = num;}
    void set_client(const MapReduceClient *cl){client=cl;}
    void set_inputvec(const InputVec* input){inputVec = input;}
    void set_outputvec(OutputVec * output){outputVec = output;}
    void set_jobstate(JobState* job){job_state = job;}
    void set_threads(pthread_t* allthreads){threads = allthreads;}
    void set_atomic(std::atomic<uint32_t> *atom){atomic_counter = atom;}
    void set_atomic1(std::atomic<uint32_t> *atom){atomic_counter1 = atom;}
    void set_atomic2(std::atomic<uint32_t> *atom){atomic_counter2 = atom;}
    void set_barrier(Barrier *barr){barrier = barr;}
    void set_intervec(std::vector<IntermediateVec*> *inter){interVec = inter;}
    void set_shuffle(std::vector<IntermediateVec*> *shuff){shuffle = shuff;}
    void set_wait(bool to_wait){is_wait = to_wait;}
    void set_stage(stage_t stage) {job_state->stage = stage;}
    void set_numpairs(int num){sum_pairs = num;}
    void set_percentage(float percentage){job_state->percentage = percentage;}
    void set_atom(unsigned long i){*(atomic_counter) += i;}

    pthread_t* get_threads(){ return threads;}
    stage_t get_stage(){return job_state->stage;}
    float get_percentage(){return job_state->percentage;}
    int get_numThreads(){return num_threads;}
    const InputVec* get_inputVec(){return inputVec;}
    std::atomic<uint32_t> *get_atomicCounter(){return atomic_counter;}
    std::atomic<uint32_t> *get_atomicCounter1(){return atomic_counter1;}
    std::atomic<uint32_t> *get_atomicCounter2(){return atomic_counter2;}
    const MapReduceClient *get_client(){return client;}
    std::vector<IntermediateVec*> *get_interVec(){return interVec;}
    std::vector<IntermediateVec*> *get_shuffle(){return shuffle;}
    bool iswait(){return is_wait;}
    pthread_mutex_t* get_mutex(){return mutex;}
    void inc_atom(){(*(atomic_counter)++);}
    void push_outputVec(OutputPair pair){outputVec->push_back(pair);}
    void activate_barrier(){barrier->barrier();}
    void push_shuffle(IntermediateVec *vec){shuffle->push_back(vec);}
    void zero_atomicCounter(){*(atomic_counter) = 0;}
    int get_threadid(){return thread_id;}
    int get_shuffle_pairs(){return shuffle_pairs;}
    int get_allpairs(){return sum_pairs;}
    JobState*  get_jobstate(){return job_state;}
    Barrier * get_barrier(){return barrier;}
    void inc_shuffle_pairs(int num_pairs){shuffle_pairs += num_pairs;}

    pthread_mutex_t* mutex;
    pthread_mutex_t* mutex1;
    pthread_mutex_t* mutex2;
    pthread_mutex_t* mutex3;
    pthread_mutex_t* mutex4;
    pthread_mutex_t* mutex5;
    pthread_mutex_t* mutex6;

private:

    int thread_id;
    int num_threads;
    const MapReduceClient *client;
    const InputVec* inputVec;
    OutputVec * outputVec;
    JobState* job_state;
    pthread_t* threads;
    bool is_wait = false;
    std::atomic<uint32_t> *atomic_counter ;
    std::atomic<uint32_t> *atomic_counter1 ;
    std::atomic<uint32_t> *atomic_counter2 ;
    Barrier *barrier;
    std::vector<IntermediateVec*> *interVec; //array of IntermediateVec!
    std::vector<IntermediateVec*> *shuffle;
    int sum_pairs = 0;
    int shuffle_pairs = 0;
};
#endif
