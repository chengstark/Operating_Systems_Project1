#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "process.h"
//Global Variable
long int seed;
double Lambda;
int Upperbound;
int num_process;
int t_cont_switch;
double Alpha;
int t_slice;
char RR_behavior = END;
int timer;
char buffer[1000];
char queue[100];

Queue_Process process_q;
Queue_Job job_q;
Summary summarys[4];					//The order is the same as they execute

//Function prototype
void Init(int argc, char** argv);
void Deinit(void);
void Simulation(int type);
void Write_Summary_ToFile(void);

/* a1: seed of RNG
 * a2: parameter Lambda
 * a3: upper bound for vaild pseudo-random number
 * a4: # of process
 * a5: time to perform a context switch
 * a6: constant alpha to perform a pridiction
 * a7: time slice for RR
 * a8: RR add behavior
 */
int main(int argc, char** argv){

	if(argc != 8 && argc != 9){
		fprintf(stderr, "Usage: ./out [S] [L] [B] [N] [CS] [A] [Trr] [Brr]\n");
		return EXIT_FAILURE;
	}

	Init(argc, argv);
	
	Simulation(SJF);
	//Simulation(SRT);
	//Simulation(FCFS);
	//Simulation(RR);
	//
	
	Write_Summary_ToFile();
	
	Deinit();

	return EXIT_SUCCESS;
}

//Initial function to parse all arguments
void Init(int argc, char** argv){
	seed = strtol(argv[1], NULL, 10);
	Lambda = strtod(argv[2], NULL);
	Upperbound = atoi(argv[3]);
	num_process = atoi(argv[4]);

	if(num_process > 26){
		fprintf(stderr, "processes no more than 26\n");
		exit(EXIT_FAILURE);
	}

	t_cont_switch = atoi(argv[5]);
	Alpha = strtod(argv[6], NULL);
	t_slice = atoi(argv[7]);

	if(argc == 9){
		if(strcmp(argv[8], "BEGINNING") == 0) RR_behavior = BEGINNING;
		else if(strcmp(argv[8], "END") == 0) RR_behavior = END;
		else{
			fprintf(stderr, "ERROR: only BEGINNING or END can be specified\n");
			exit(EXIT_FAILURE);
		}
	}

	set_Alpha(Alpha);
	set_Tcontent(t_cont_switch);
}

//Deinitialize function
void Deinit(void){
	Deinit_PQ(&process_q);
	Deinit_JQ(&job_q);
}

//Main simulation routine
void Simulation(int type){

	RNG48_set(seed, Lambda, Upperbound);
	Generate_processes(&process_q, num_process);
	Init_Job_Queue(&job_q, num_process, type, RR_behavior);
	update_AlgoName(type, &(summarys[type]) );
	
	Job* run_job = NULL;
	Job* switch_in = NULL;
	Job* switch_out = NULL;

	char switch_flag = 0;
	int countdown_in = 0;
	int countdown_out = 0;
	int countdown_RR = t_slice;

	char algo_name[5];

	translate_AlgoName(type, algo_name);

	for (timer = 0; ; timer++){
		int index = 0;

		if(timer == 0) {
			printf("time 0ms: Simulator started for %s [Q <empty>]\n", algo_name);
		}

		//Finish IO work
		do_IO_update(&job_q, timer);

		//Process all landed processes
		for( int i = 0; ; i++ ){
			
			index = check_arrival(&process_q, timer);
			
			if(index == -1) break;
			
			Job* temp_ptr = NULL;
			temp_ptr = create_job(process_q.processes[index]);
			process_q.processes[index] = NULL;

			update_CPUburst_job(temp_ptr);

			char if_preemption = 0;

			//Check if there is a need for preemption
			if(i == 0 && type == SRT){
				if(run_job->estimate_burst_time > temp_ptr->estimate_burst_time){
					
					switch_in = temp_ptr;
					countdown_in = t_cont_switch / 2; 
					switch_flag = 1;

					switch_out = run_job;
					countdown_out = t_cont_switch / 2;
					
					run_job = NULL;

					if_preemption = 1;
				}
			}
			
			//Time is up for RR
			else if(type == RR && countdown_RR <= 0){
			
				if(temp_ptr != NULL){
					
					switch_in = temp_ptr;
					countdown_in = t_cont_switch / 2; 
					switch_flag = 1;

					switch_out = run_job;
					countdown_out = t_cont_switch / 2;
					
					run_job = NULL;

					if_preemption = 1;
				}
			}

			//Add a job to a job queue
			else if (!if_preemption){
				
				update_arrival_job(temp_ptr, timer);
				add_job_to_queue(temp_ptr, &job_q);	
			
				get_Job_Queue(&job_q, queue);
				sprintf(buffer, "Process %c(tau %dms) arrived, added ot ready queue [Q %s]", 
					temp_ptr->PID, temp_ptr->estimate_burst_time, queue);
				printf("time %dms: %s\n", timer, buffer);
			
			}
		}

		//Find next process to run	
		if (run_job == NULL && !switch_flag) { 
			switch_in = get_next_job_inqueue(&job_q); 
			if(switch_in != NULL) { 
				countdown_in = t_cont_switch / 2; 
				switch_flag = 1;
			}
		}

		//Process move_in content, this operation may start only after move_out
		//opeation being complete
		if (switch_in != NULL && switch_out == NULL){

			//Check if the operation was done in the last second
			if(countdown_in <= 0){
				run_job = switch_in;
				switch_in = NULL;
				update_CPUburst_job(run_job);

				get_Job_Queue(&job_q, queue);
				sprintf(buffer, "Process %c started using the CPU for %dms burst [Q %s]", 
						run_job->PID, run_job->CPU_burst, queue);
				printf("time %dms: %s\n", timer, buffer);
				
				switch_flag = 0;
			}
			countdown_in--;
		}

		//Process move_out content
		if (countdown_out > 0){

			//Check if the operation was done in the last second
			if(switch_out != NULL){
				update_finish_job(switch_out, timer);
				update_avg_time( switch_out, &(summarys[type]) );

				//Also update tau
				IOBlock_job_update(switch_out, &job_q);
				switch_out = NULL;
			}

			countdown_out--;
		}

		//Run current process
		if (run_job != NULL) { 

			int state = run_a_job(run_job);
			countdown_RR--;
			
	
			//If finish CPU burst
			if(state == 1 || state == 2){

				//Set a flag to move out the content from cache
				countdown_out = t_cont_switch / 2;
			}

			//IO Blocked, add it back to the queue and do the IO part
			if(state == 1) {
				
				get_Job_Queue(&job_q, queue);
				sprintf(buffer, "Process %c completed a CPU burst; %d bursts to go [Q %s]", 
						run_job->PID, (run_job->num_CPU_burst - run_job->index), queue);
				printf("time %dms: %s\n", timer, buffer);
			
				estimate_CPU_burst(run_job);
				sprintf(buffer, "Recalculated tau = %dms for process %c [Q %s]", 
						run_job->estimate_burst_time, run_job->PID, queue);
				printf("time %dms: %s\n", timer, buffer);
				
				sprintf(buffer, "Process %c switching out of CPU; will block on I/O until time %dms [Q %s]", 
						run_job->PID, run_job->IO_burst_time[run_job->index], queue);
				printf("time %dms: %s\n", timer, buffer);

				switch_out = run_job;
				update_context_switch(&(summarys[type]));
				run_job = NULL;
			}

			//Finished housekeeping the finished job
			else if(state == 2) {
				
				get_Job_Queue(&job_q, queue);
				sprintf(buffer, "Process %c terminated [Q %s]", 
						run_job->PID, queue);
				printf("time %dms: %s\n", timer, buffer);

				Deinit_J(run_job);
				free(run_job);
				run_job = NULL;
				switch_out = NULL;
				//If all works are done
				if( check_all_job_done(&process_q, &job_q) && run_job == NULL && 
					switch_in == NULL && switch_out == NULL) {

					printf("time %dms: Simulator ended for %s [Q <empty>]\n", timer, algo_name);

					break;
				}
			}
		}
	}

	Deinit();
}

void Write_Summary_ToFile(void){
	FILE* wf = fopen("simout.txt", "w");

	for(int i = 0; i < 4; i++){
		writef_summary(&(summarys[i]), wf);
	}

	fclose(wf);
}
