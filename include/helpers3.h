#ifndef HELPERS3_H
#define HELPERS3_H

#include "icssh.h"
#include "debug.h"
#include "dlinkedlist.h"
// A header file for helpers.c
// Declare any additional functions in this file

extern volatile sig_atomic_t chld_flag;

extern volatile sig_atomic_t sigusr2_flag;
extern volatile int program_count;

extern int p[2];

int comparatorFunc(const void * a, const void * b);

void printFunc(void * a, void * b);

void deleteNode(void * a);

void rm_and_delete_node(dlist_t* bgjobs, void *a);

void add_background_job(job_info * job, int pid, dlist_t* bgjobs);

void sigchld_handler(int sig);

void sigusr2_handler(int sig);

void reap_bg_procs(dlist_t * bgjobs, int flag);

node_t * findTail(dlist_t * bgjobs);

void redirect_handler(job_info * job);

void piping_handler(job_info * job);

#endif
