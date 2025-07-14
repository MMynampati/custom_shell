#include "icssh.h"
#include "dlinkedlist.h"
#include "helpers3.h"
#include <time.h>
#include <unistd.h>
#include <errno.h>

volatile sig_atomic_t chld_flag = 0;

volatile sig_atomic_t sigusr2_flag = 0;
volatile int program_count = 0;

//int p[2] = {0,0};
// Your helper functions need to be here.

int comparatorFunc(const void * a, const void * b){
    if(a == NULL || b == NULL){return 0;}

    //node_t * nodeA = (node_t*)a;
    //node_t * nodeB = (node_t*)b;
    //bgentry_t * entryA = (bgentry_t*)nodeA->data;
    //bgentry_t * entryB = (bgentry_t*)nodeB->data;
    
    bgentry_t * entryA = (bgentry_t*)a;
    bgentry_t * entryB = (bgentry_t*)b;
    return (entryA->seconds - entryB->seconds);

}

void printFunc(void * a, void * b){
    if(a==NULL || b==NULL){return;}
    FILE * fp = (FILE*)b;

    bgentry_t * entryA = (bgentry_t*)a;
    if(entryA->job == NULL){return;}
    fprintf(fp, "job:%s, pid:%d, time:%ld\n", entryA->job->line, entryA->pid, entryA->seconds);
}

/*
void deleteFunc(void * a){
    node_t* cur_node = (node_t*)a;
    node_t* cur_copy = cur_node;
    bgentry_t * cur_struct = (bgentry_t*)cur_node->data;
    if(cur_node->prev == NULL){
        if(cur_node->next){
            cur_node->next->prev = NULL;
        }
    }
    else{
        if(cur_node->next == NULL){
            cur_node->prev->next = NULL;
        }
        else{
            cur_node->prev->next = cur_node->next;
            cur_node->next->prev = cur_node->prev;
        }
    }
    if(cur_node->next){
        cur_node = cur_node->next;
    }

    free(cur_struct->job->procs);
    free(cur_struct->job);
    free(cur_struct);
    free(cur_copy);
    cur_copy = NULL;
    cur_struct = NULL;
}
*/

void deleteNode(void *a){
    if(a == NULL){return;}

    node_t * cur_node = (node_t*)a;
    bgentry_t * cur_struct = (bgentry_t*)cur_node->data;
    //free(cur_struct->job->procs);
    //free(cur_struct->job);
    free_job(cur_struct->job);
    free(cur_struct);
    free(cur_node);
    cur_struct = NULL;
    cur_node = NULL;
    return;
}

void rm_and_delete_node(dlist_t* bgjobs, void *a){
    if(a==NULL || bgjobs == NULL){return;}
    node_t * cur_node = (node_t*)a;
    bgentry_t *cur_struct = (bgentry_t*)cur_node->data;

    if(cur_node->prev != NULL){
        cur_node->prev->next = cur_node->next;
    }
    else{
        bgjobs->head = cur_node->next;
    }
    if(cur_node->next != NULL){
        cur_node->next->prev = cur_node->prev;
    }

    free_job(cur_struct->job);
    free(cur_struct);
    free(cur_node);
    cur_node = NULL;
    cur_struct = NULL;
    bgjobs->length--;
    return;
}


void add_background_job(job_info * job, int pid, dlist_t* bgjobs){
    bgentry_t* cur_bgentry = malloc(sizeof(bgentry_t));
    cur_bgentry->job = job;
    cur_bgentry->pid = pid;
    cur_bgentry->seconds = time(NULL);

    InsertAtTail(bgjobs, cur_bgentry);
    return;
}

void sigchld_handler(int sig){
    chld_flag = 1;
}

void sigusr2_handler(int sig){
    int olderrno = errno;
    
    //Sio_puts("Number of programs successfully completed ");
    //Sio_put1((long)program_count);    //might need to cast as int instead (and figure out corresponding sio)
    //Sio_puts("\n");
    //if(program_count > 0){
        write(2, "Number of programs successfully completed: ", 43);
        char s[12];
        snprintf(s, sizeof(s), "%d", program_count);  //change to snprintf if causing issues
        write(2, s, strlen(s));
        write(2, "\n", 1);
    //}
    errno = olderrno;
} 


void reap_bg_procs(dlist_t * bgjobs, int flag){
    int olderrno = errno;
    pid_t pid; 
    while( (pid = waitpid(-1, NULL, !flag ? WNOHANG: 0)) > 0){
        //Sio_puts? 
        int counter = 0;
        //printf("checks each pid that needs to be reaped");
        node_t* cur_node = bgjobs->head;  
        while(cur_node != NULL){
            bgentry_t *cur_struct = (bgentry_t*)cur_node->data;
            if(cur_struct->pid == pid){
                printf(BG_TERM, pid, cur_struct->job->line);
                
                if(cur_node->prev == NULL){
                    bgjobs->head = cur_node->next;
                    if(cur_node->next){
                       cur_node->next->prev = NULL;
                    }
                }
                else{
                    if(cur_node->next == NULL){
                        cur_node->prev->next = NULL;
                    }
                    else{
                        cur_node->prev->next = cur_node->next;
                        cur_node->next->prev = cur_node->prev;
                    }
                }
                bgjobs->length--;
                free_job(cur_struct->job);
                free(cur_struct);
                free(cur_node);
                cur_node = NULL;
                cur_struct = NULL;
            }
            else{
                cur_node = cur_node->next;
            }
        }
        //printf("Handler reaped child\n");
        program_count++;
    }
    errno = olderrno;
}

node_t * findTail(dlist_t * bgjobs){
    node_t * node = bgjobs->head;
    while(node!= NULL && node->next!=NULL){
        node = node->next;
    }
    return node;
}

void redirect_handler(job_info * job){
    if(job->in_file != NULL || job->out_file != NULL || (job->procs != NULL && job->procs->err_file != NULL)){
        /*
        if(job->in_file != NULL && job->out_file != NULL){
            if(strcmp(job->in_file, job->out_file) == 0){
                fprintf(stderr, RD_ERR);  //might want to change to an int output so cleanup can be done in main before quitting w/ error
                exit(EXIT_FAILURE);
            }
        }
        */
        if(job->procs->err_file != NULL && !job->outerr){
            int write_file = open(job->procs->err_file, O_WRONLY | O_CREAT | O_APPEND, 0664);
            if(write_file < 0){
                perror("write went wrong");
                exit(EXIT_FAILURE);
            }
            if(dup2(write_file, 2) < 0){
                perror("dup write failed");
                close(write_file);
                exit(EXIT_FAILURE);
            }
            close(write_file);
            //return;
        }

        if(job->in_file != NULL){
            int read_file = open(job->in_file, O_RDONLY, 0);
            if(read_file < 0){
                fprintf(stderr, RD_ERR);
                exit(EXIT_FAILURE);
            }

            if(dup2(read_file, 0) < 0){
                perror("dup failed for some reason");
                close(read_file);
                exit(EXIT_FAILURE);
            }
            close(read_file);
        }
        
        if(job->out_file != NULL){
            int write_file = -1;
            if(job->append){
                write_file = open(job->out_file, O_WRONLY | O_CREAT | O_APPEND, 0664);
            }
            else{
                write_file = open(job->out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644); 
            }

            if(write_file < 0){
                perror("write went wrong");//error handling? 
                exit(EXIT_FAILURE);
            }
            
            if(job->outerr){
                if(dup2(write_file, 2) < 0){
                    perror("dup failed");
                    close(write_file);
                    exit(EXIT_FAILURE);
                }
            }

            if(dup2(write_file, 1) < 0){
                perror("dup write failed for some reason");
                close(write_file);
                exit(EXIT_FAILURE);
            }
            close(write_file);
        }

    }
}

void piping_handler(job_info * job){
    //only deals with child pipe
    //pipes 1 to n-1
    int p[2] = {0,0};
    int exec_result = 0;
    if(pipe(p) < 0){
        perror("piping failed");
        exit(EXIT_FAILURE);
    }
   //before fork call 
    pid_t pid;
    if((pid = fork()) < 0){
        perror("fork failure");
        exit(EXIT_FAILURE);
    }

    if (pid == 0){   //child - after fork call
        close(p[0]);
        if(dup2(p[1], 1) < 0){
            perror("dup2 failed");
            exit(EXIT_FAILURE);
        }
        close(p[1]);
        
        proc_info* proc = job->procs;
        printf("%s", proc->cmd);
        exec_result = execvp(proc->cmd, proc->argv);
        if(exec_result < 0){
            printf(EXEC_ERR, proc->cmd);
            exit(EXIT_FAILURE);
        }
    }
    //parent - after fork call

    close(p[1]);
    if(dup2(p[0], 0) < 0){  //parent write
        perror("dup2 failed");
        exit(EXIT_FAILURE);
    }
    close(p[0]);
    return;

    //pipe n


}
