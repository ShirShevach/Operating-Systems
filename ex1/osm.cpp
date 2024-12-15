
#include <iostream>
#include <sys/time.h>
#include "osm.h"
#define TO_USEC_TO_NSEC 1000
#define TO_SEC_TO_NSEC 1000000000



/* Time measurement function for a simple arithmetic operation.
   returns time in nano-seconds upon success,
   and -1 upon failure.
   */
double osm_operation_time(unsigned int iterations) {
    size_t a;
    a=0;
    int res;
    struct timeval begin, end;
    res = gettimeofday(&begin, NULL);
    if (res == -1 || iterations == 0) {
        return -1;
    }
    for (size_t i = 0; i < iterations; i+=5) {
        a +=1;
        a +=1;
        a +=1;
        a +=1;
        a +=1;
    }
    res = gettimeofday(&end, NULL);
    if (res == -1) {
        return -1;
    }
    double time = (end.tv_usec - begin.tv_usec)*TO_USEC_TO_NSEC
                  + (end.tv_sec - begin.tv_sec)*TO_SEC_TO_NSEC;

    return time;
}


void empty_func() {
}

/* Time measurement function for an empty function call.
   returns time in nano-seconds upon success,
   and -1 upon failure.
   */
double osm_function_time(unsigned int iterations){
    struct timeval begin, end;
    int res = gettimeofday(&begin, NULL);
    if (res == -1 || iterations == 0) {
        return -1;
    }
    for (size_t i = 0; i<iterations; i+=5) {
        empty_func();
        empty_func();
        empty_func();
        empty_func();
        empty_func();
    }
    res = gettimeofday(&end, NULL);
    if (res == -1) {
        return -1;
    }
    double time = (end.tv_usec - begin.tv_usec)*TO_USEC_TO_NSEC
                  + (end.tv_sec - begin.tv_sec)*TO_SEC_TO_NSEC;
    return time;
}


/* Time measurement function for an empty trap into the operating system.
   returns time in nano-seconds upon success,
   and -1 upon failure.
   */
double osm_syscall_time(unsigned int iterations){
    struct timeval begin, end;
    int res = gettimeofday(&begin, NULL);
    if (res == -1 || iterations == 0) {
        return -1;
    }
    for (size_t i = 0; i<iterations; i+=5) {
        OSM_NULLSYSCALL;
        OSM_NULLSYSCALL;
        OSM_NULLSYSCALL;
        OSM_NULLSYSCALL;
        OSM_NULLSYSCALL;
    }
    res = gettimeofday(&end, NULL);
    if (res == -1) {
        return -1;
    }
    double time = (end.tv_usec - begin.tv_usec)*TO_USEC_TO_NSEC
                  + (end.tv_sec - begin.tv_sec)*TO_SEC_TO_NSEC;
    return time;
}
