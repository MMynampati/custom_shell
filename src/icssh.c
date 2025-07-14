#include "icssh.h"
#include "dlinkedlist.h"
#include "helpers3.h"
#include "debug.h"
#include <readline/readline.h>
#include <unistd.h>

//volatile sig_atomic_t chld_flag = 0;

int main(int argc, char* argv[]) {
	int exec_result = 0;
	int exit_status = 0;
	pid_t pid;
	pid_t wait_result;
	char* line;

#ifdef GS  // *This compilation flag is for grading purposes. DO NOT REMOVE*
	rl_outstream = fopen("/dev/null", "w");
#endif

	// Setup segmentation fault handler
	if (signal(SIGSEGV, sigsegv_handler) == SIG_ERR) {
		perror("Failed to set signal handler");
		exit(EXIT_FAILURE);
	}

    /////
    if (signal(SIGUSR2, sigusr2_handler) == SIG_ERR){
        perror("signal error");
    }


    dlist_t* bgjobs = CreateList(comparatorFunc, printFunc, deleteNode);

    // print the prompt & wait for the user to enter commands string
	// readline returns a dynamically allocated buffer for the line
	while ((line = readline(SHELL_PROMPT)) != NULL) {
        if(chld_flag == 1){
            reap_bg_procs(bgjobs, 0);
            chld_flag = 0;
        }
    
        // Command string is parsed into a job struct
        // Will print out error message if command string is invalid
        job_info* job = validate_input(line);
        if (job == NULL) { // Command was empty string or invalid
            free(line);    // free the buffer allocated by readline
            continue;
        }
        //Prints out the job linked list struture for debugging
        
        #ifdef DEBUG   // *If DEBUG flag removed in makefile, this will not longer print*
            debug_print_job(job);
        #endif

		//exit
		if (strcmp(job->procs->cmd, "exit") == 0) {
			// Terminating the shell
            
            node_t * node = bgjobs->head;
            bgentry_t * bgentry = NULL;
            //printf("before while loop");

            while(node != NULL){
                //for every element in list kill then delete entry
                bgentry = (bgentry_t*)node->data;
              //  printf("before SIGKILL");
                node = node->next;
                kill(bgentry->pid, SIGKILL);
              //  printf("after SIGKILL");
                //node = node->next;
            }
            
            //printf("after while loop");
            reap_bg_procs(bgjobs, 1);
            node = NULL;
            bgentry = NULL;
            //free(bgentry);    //NEW ADD
            

            free(bgjobs);
            
            free(line);				// free the buffer allocated by readline 
			free_job(job);			// free the job_info struct created by validate_input for the line
            validate_input(NULL);   // calling validate_input with NULL to free its internal allocated memory
            return 0;
		}
        
        if (strcmp(job->procs->cmd, "cd") == 0){
            if(job->procs->argv[1] == NULL){
                //printf("no dir provided\n"); //what is the right error message to print?
                //perror("no dir provided");
                fprintf(stderr, DIR_ERR);                
            }
            else{
                if(chdir(job->procs->argv[1]) != 0){
                    fprintf(stderr, DIR_ERR);
                    //perror("cd");
                }
                else{
                    char s[100];
                    printf("%s\n", getcwd(s, 100));
                }
            }

            //cleanup
            free(line);
            free_job(job);
            continue;
        }
        
        if(strcmp(job->procs->cmd, "estatus") == 0){
            int status = 0;
            if(WIFEXITED(exit_status)){
                status = WEXITSTATUS(exit_status);
            }
            if(status == 0){
                printf("%d\n", status);
            }
            else{
                //printf("%s%d%s\n", KRED, exit_status, KNRM);                
                printf("%s%d%s\n", "\033[1;31;47m", status, "\033[0m");
            }
            //cleanup
            free(line);
            free_job(job);
            continue;
        }
    
       if(strcmp(job->procs->cmd, "bglist") == 0){
            node_t * node = bgjobs->head;
            bgentry_t * bgentry = NULL;
            while(node != NULL){
                bgentry = (bgentry_t*)node->data;
                if(bgentry != NULL){
                    print_bgentry(bgentry);
                }
                node = node->next;
            }

            //cleanup
            free(line);
            free_job(job);
            continue;
       }
       
       if(strcmp(job->procs->cmd, "fg") == 0){
            if(bgjobs->head == NULL){     //if bgjobs is empty
                printf(PID_ERR);
                continue;
            }
            
            //check number of args 
            //if pid is provided
            if(job->procs->argc == 2){
                //find node
                node_t * node = bgjobs->head;
                node_t * node_to_delete = NULL;
                int pid = atoi(job->procs->argv[1]);

                while(node!=NULL){
                    bgentry_t * bgentry = (bgentry_t*)node->data;
                    if(bgentry->pid == pid){
                        node_to_delete = node;
                        break;
                    }
                    node = node->next;
                }
                if(node_to_delete == NULL){
                    printf(PID_ERR);
                    free(line);
                    free_job(job);
                    continue;
                }
                //bring to fg 
                
                wait_result = waitpid(pid, &exit_status, 0);
                if (wait_result < 0) {
                    printf(WAIT_ERR);
                    exit(EXIT_FAILURE);
                }

                bgentry_t * this_entry = (bgentry_t*)node_to_delete->data;
                printf("%s\n", this_entry->job->line);
                rm_and_delete_node(bgjobs, node_to_delete);
                program_count++;
            }
            else if (job->procs->argc == 1){
                node_t * node = findTail(bgjobs);
                bgentry_t * bgentry = (bgentry_t*)node->data;
                
                wait_result = waitpid(bgentry->pid, &exit_status, 0);
                if (wait_result < 0){
                    printf(WAIT_ERR);
                    exit(EXIT_FAILURE);
                }
                bgentry_t * this_entry = (bgentry_t*)node->data;
                printf("%s\n", this_entry->job->line);

                rm_and_delete_node(bgjobs, node);
                program_count++;
            }
            //cleanup
            free(line);
            free_job(job);
            continue;
       }

       if(signal(SIGCHLD, sigchld_handler) == SIG_ERR){
            perror("signal error");
            //exit(EXIT_FAILURE);
       }

        //int p[2] = {0,0};
//        pipe(p);

        // create the child proccess
		if ((pid = fork()) < 0) {
			perror("fork error");
			exit(EXIT_FAILURE);
		}
		if (pid == 0) {  //If zero, then it's the child process
            //get the first command in the job list to execute
            proc_info * proc = NULL;
            if(job->nproc > 1){
                //call pipe func
                piping_handler(job);
                proc = job->procs->next_proc;
                printf("%s", proc->cmd);
                exec_result = execvp(proc->cmd, proc->argv);
            }
            else{
                redirect_handler(job);
            
                //redirect_handler(job);
                proc = job->procs;
			    printf("%s", proc->cmd);
                exec_result = execvp(proc->cmd, proc->argv);
			}
            if (exec_result < 0) {  //Error checking
				printf(EXEC_ERR, proc->cmd);
				
				// Cleaning up to make Valgrind happy 
				// (not technically necessary because resources will be released when reaped by parent)
				free(line);
				free_job(job);  
        		validate_input(NULL);
				exit(EXIT_FAILURE);
			}
		}

        //different handling for foreground and background

        if(job->bg == true){
            add_background_job(job, pid, bgjobs);  
            free(line);
            continue;
        }
        else{
            wait_result = waitpid(pid, &exit_status, 0);
		    if (wait_result < 0) {
			    printf(WAIT_ERR);
                //free(bgjobs);      //NEW ADD
			    exit(EXIT_FAILURE);
		    }
            else{
                program_count++;
            }
        }

		// As the parent, allocated command and line is no longer needed
        free(line);		
		free_job(job);  

	}

	validate_input(NULL); // calling validate_input with NULL to free its internal allocated memory

#ifndef GS // *This compilation flag is for grading purposes. DO NOT REMOVE*
	fclose(rl_outstream);
#endif

	return 0;
}
