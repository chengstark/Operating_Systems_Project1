/*	Main logic:
 *		When a process arrive, a job will be generated and add it to the ready queue or
 *	run directly (SRT only). Upon arrival one burst, the job will be updated with it current 
 *	arrival time. Also upon finish of one burst, the job will be updated with it current 
 *	finish time. When a job is completely done with content switch, the average time will
 *	be update to the summary and the # of preemption will be reset. Therefore, please 
 *	remember to update the preemption time before reset (Recommend to do it every time 
 *	the event happens).
 */

#ifndef PROCESS
#define PROCESS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define READY		0
#define RUNNING		1
#define PENDING		2
#define BLOCKED		3
#define COMPLETE	4

#define SJF			0
#define SRT			1
#define FCFS		2
#define RR			3

#define BEGINNING	0
#define END			1

//Global setting function
//Set the parameter
void RNG48_set(long int seed, double Lambda, int Upperbound);
int RNG(int m);
void set_Alpha(double a);
void set_Tcontent(int time);

//Store the needed info of every element Process
typedef struct{
	char PID;							//PID
	int arrival;						//Arrival time
	int num_CPU_burst;					//Number of burst contained by this process
	int* CPU_burst_time;				//Array of CPU burst time
	int* IO_burst_time;					//Array of IO burst time
} Process;

void Deinit_P(Process* a);

//A queue to store Processes by initial arrival time
typedef struct {
	char size;
	Process** processes;
} Queue_Process;

void Deinit_PQ(Queue_Process* a);
void Generate_processes(Queue_Process* QP, int num_p);
int compare_func_proc(const void* a, const void* b);
int check_arrival(Queue_Process* QP, int time);


//Store the needed info of every element Job
//Before added to the ready queue in the first time, a job should be generated
//by calling create_job
typedef struct {
	char PID;							//PID
	char state;							//State of this job
	int num_CPU_burst;					//Number of burst contained by this job
	int index;							//Current running CPU burst
	int estimate_burst_time;			//Est. next CPU burst time
	int finish;							//Time when this IO burst time was finished
	int CPU_burst;						//Time need for current CPU burst
	int arrival;						//Time when this Job was add to the queue
	int preemption_time;				//Num of preemption
	int* CPU_burst_time;
	int* IO_burst_time;
} Job;

void Deinit_J(Job* a);
void estimate_CPU_burst(Job* a);
Job* create_job(Process* p);
int run_a_job(Job* J);
void update_arrival_job(Job* J, int time);
void update_CPUburst_job(Job* J);
void update_finish_job(Job* J, int time);
void update_preemption_job(Job* J);


//Queue to store Jobs, sorted according to specific algorithm
typedef struct {
	char size;
	Job** jobs;
	char type;
} Queue_Job;

void Init_Job_Queue(Queue_Job* QJ, int num_p, char type);
void Deinit_JQ(Queue_Job* a);

void add_job_to_queue(Job* J, Queue_Job* QJ);
int compare_job_prior_1(const void* a, const void* b);
int compare_job_prior_2(const void* a, const void* b);

void get_Job_Queue(Queue_Job* QJ);
Job* get_next_job_inqueue(Queue_Job* QJ);

void do_IO_update(Queue_Job* QJ, int time);
void IOBlock_job_update(Job* J, Queue_Job* QJ);

//Store summary info
typedef struct {
	int avg_CPU_burst;
	int avg_wait_time;
	int avg_turnaround;
	int num_context_switch;
	int num_preemptions;
} Summary;

void update_avg_time(Job* J, Summary* S);
void update_preemptions(Summary* S);
void update_context_switch(Summary* S);

int check_all_job_done(Queue_Process* QP, Queue_Job* QJ);



#endif
