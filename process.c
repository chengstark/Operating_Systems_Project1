#include "process.h"

long int seed;
double Lambda;
int Upperbound;
double Alpha;
int t_cont_switch; 

//Deinitialize funtion for Process
//Might need to call free() after this Deinitialization
void Deinit_P(Process* a){
	if(a->CPU_burst_time != NULL){
		free(a->CPU_burst_time);
		a->CPU_burst_time = NULL;
	}
	if(a->IO_burst_time != NULL) {
		free(a->IO_burst_time);
		a->IO_burst_time = NULL;
	}
}

//Deinitialize funtion for Process Queue
void Deinit_PQ(Queue_Process* a){
	for(int i = 0; i < a->size; i++){
		if(a->processes[i] != NULL) Deinit_P(a->processes[i]);
		free(a->processes[i]);
	}

	if(a->processes != NULL){
		free(a->processes);
		a->processes = NULL;
	}

	a->size = 0;
}

//Deinitialization function for Job
//Might need to call free() after this Deinitialization
void Deinit_J(Job* a){
	if(a->CPU_burst_time != NULL){
		free(a->CPU_burst_time);
		a->CPU_burst_time = NULL;
	}
	if(a->IO_burst_time != NULL) {
		free(a->IO_burst_time);
		a->IO_burst_time = NULL;
	}
}

//Deinitialization function for Job Queue
void Deinit_JQ(Queue_Job* a){
	for(int i = 0; i < a->size; i++){
		if(a->jobs[i] != NULL) Deinit_J( a->jobs[i] );
		free( a->jobs[i] );
	}

	if(a->jobs != NULL){
		free(a->jobs);
		a->jobs = NULL;
	}

	a->size = 0;
}

//input:
//s -- seed
//l -- Lambda
//u -- Upperbound
void RNG48_set(long int s, double l, int u){
	seed = s;
	Lambda = l;
	Upperbound = u;
	
	srand48(seed);
}

//input:
//a -- Alpha
void set_Alpha(double a) { Alpha = a; }

//input:
//time -- the time slice value for RR algorithm
void set_Tcontent(int time) { t_cont_switch = time; }

//input:
//m -- multiple
//generated legal random number will be returned
//If 100 is set, exp_distributation will not be used
int RNG(int m){

	double result = 0;

	//Used for generating the number of # of processes
	if(m == 100){ 

#ifdef debug
		return 4;
#else
		return (int)( 100 * ( drand48() ) + 1); 
#endif
	
	}

	while(1){
		result = -log(drand48()) / Lambda;

		if(result > Upperbound) continue;
		else break;
	}

	result = (double)result * m;

	if(result == 0) result = 1;

	return (int)result;
}

//input:
//QP -- address of Process Queue
//num_p -- # of processes generated
//This function generate the specific # of process and add these
//processes to the arrival queue
void Generate_processes(Queue_Process* QP, int num_p){
	char namelist[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	QP->size = num_p;
	QP->processes = calloc(num_p, sizeof(Process *));

	Process** ptr = (Process**)(QP->processes);

	for(int i = 0; i < num_p; i++){
		Process* p = calloc(1, sizeof(Process));

		//Predict step 1
		p->arrival = RNG(1);

		//Predict step 2
		p->num_CPU_burst = RNG(100);

		p->CPU_burst_time = calloc(p->num_CPU_burst, sizeof(int));
		p->IO_burst_time = calloc(p->num_CPU_burst, sizeof(int));

		//Predict step 3
		for(int a = 0; a < p->num_CPU_burst; a++){
			if(a == (p->num_CPU_burst - 1)){
				p->CPU_burst_time[a] = RNG(1);
				p->IO_burst_time[a] = 0;
			}
			else{
				p->CPU_burst_time[a] = RNG(1);
				p->IO_burst_time[a] = RNG(1);
			}

			if(i == 0){
				printf("A: %d\n", p->CPU_burst_time[a]);
			}
		}
		
		* ptr = (Process *)p;

		ptr++;
	}
	
	//Sort the arrival time in the beginning
	qsort(QP->processes, num_p, sizeof(Process *), compare_func_proc); 

	ptr = (Process**)(QP->processes);

	//Assign PID afterhand
	for(int i = 0; i < num_p; i++){
		ptr[i]->PID = namelist[i];
	}
}

//input:
//QJ -- address of Job Queue
//num_p -- # of maximum possible processes that could be added to
//the Ready queue
void Init_Job_Queue(Queue_Job* QJ, int num_p, char type){
	QJ->size = num_p;
	QJ->jobs = calloc(num_p, sizeof(Job *));
	QJ->type = type;

	for(int i = 0; i < QJ->size; i++) { QJ->jobs[i] = NULL; }
}

//This function is to sort the Processes by time of arrival
//Used by qsort()
int compare_func_proc(const void* a, const void* b){

	Process** pa = (Process **) a;
	Process** pb = (Process **) b;

	if(pa[0] == NULL) return 5;

	if(pb[0] == NULL) return -5;
	
	//Handle ties
	if(pa[0] -> arrival == pb[0]-> arrival){
		if(pa[0]->PID > pb[0]->PID) return 1;
		else if(pa[0]->PID < pb[0]->PID) return -1;
		else return 0;
	}

	else{
		if(pa[0]->arrival > pb[0]->arrival) return 2;
		else if(pa[0]->arrival < pb[0]->arrival) return -2;
		else return 0;
	}
}

//This function is to sort the process by estimate time of CPU burst
//Used mainly by qsort()
int compare_job_prior_1(const void* a, const void* b){
	Job** ja = (Job **)a;
	Job** jb = (Job **)b;

	//Move all ready job to the front
	if(ja[0] == NULL) return 10;
	if(jb[0] == NULL) return -10;

	if(ja[0]->state > jb[0]->state)	return 5;
	else if(ja[0]->state < jb[0]->state) return -5;

	//Sort by estimate burst time
	//If tie, sort by PID name
	if(ja[0]->estimate_burst_time == jb[0]->estimate_burst_time){
		if(ja[0]->PID > jb[0]->PID) return 1;
		else if(ja[0]->PID < jb[0]->PID) return -1;
		else return 0;
	}

	else{
		if(ja[0]->estimate_burst_time > jb[0]->estimate_burst_time) return 2;
		else if(ja[0]->estimate_burst_time < jb[0]->estimate_burst_time) return -2;
		else return 0;
	}
}

//Update next estimate CPU burst time after last CPU burst completed
void estimate_CPU_burst(Job* a){
	int temp = a->estimate_burst_time;
	a->estimate_burst_time = (double)Alpha * a->CPU_burst + (double)(1 - Alpha)* temp;
	printf("%c: %d est.\n", a->PID, a->estimate_burst_time);
}

//Check if next process is arrived
//The index of the arrival process will be returned
//If not found, -1 will be returned
int check_arrival(Queue_Process* QP, int time){
	
	for(int i = 0; i < QP->size; i++){
		if(QP->processes[i] == NULL) continue;
		else if(QP->processes[i]->arrival <= time) return i;
		else return -1;
	}

	return -1;
}


//Use Process info to generate the info of a new job
//The allocated and initialized Job will be returned
//Remember to reset p ptr to NULL
Job* create_job(Process* p){
	Job* j = NULL;

	j = calloc(1, sizeof(Job));
	
	j->PID = p->PID;
	j->state = READY;
	j->num_CPU_burst = p->num_CPU_burst;
	j->index = 0;
	j->estimate_burst_time = 1 / Lambda;

	int array_size = p->num_CPU_burst * sizeof(int);
	j->CPU_burst_time = calloc(j->num_CPU_burst, sizeof(int));
	memcpy(j->CPU_burst_time, p->CPU_burst_time, array_size);
	j->IO_burst_time = calloc(j->num_CPU_burst, sizeof(int));
	memcpy(j->IO_burst_time, p->IO_burst_time, array_size);

	j->arrival = p->arrival;

	Deinit_P(p);
	free(p);

	return j;
}

//Add a Job to the Ready Queue
void add_job_to_queue(Job* J, Queue_Job* QJ){

	for(int i = 0; i < QJ->size; i++){
		if(QJ->jobs[i] == NULL){ 
			QJ->jobs[i] = (Job *) J;
			break;
		}
	}
	
	//If the schedule scheme is SJF or SRT
	if(QJ->type == SJF || QJ->type == SRT){
		qsort(QJ->jobs, QJ->size, sizeof(Job *), compare_job_prior_1);		
	}

	//If is FCFS or RR
	else{
		//do not sort
	}
}


//Get next job available in the queue
//the address of Job will be returned
//if no Job available, return NULL
Job* get_next_job_inqueue(Queue_Job* QJ){
	Job* next = NULL;

	for(int i = 0; i < QJ->size; i++){
		if(QJ->jobs[i] == NULL) continue;

		if(QJ->jobs[i]->state != READY) continue;

		next = (Job *)QJ->jobs[i];
		
		next->state = RUNNING;

		QJ->jobs[i] = NULL;

		//If the schedule scheme is SJF or SRT
		if(QJ->type == SJF || QJ->type == SRT){
			qsort(QJ->jobs, QJ->size, sizeof(Job *), compare_job_prior_1);
		}

		return next;
	}
	
	return next;
}


//Simulate the process to run a job by one ms
//Return value:
//2 for finished job
//1 for I/O blocking job
//0 for not completed
int run_a_job(Job* J){
	
	J->CPU_burst_time[J->index] -= 1;

	//If CPU burst part is finished
	if(J->CPU_burst_time[J->index] < 0){
		if(J->index == (J->num_CPU_burst - 1)) {
			J->state = COMPLETE;
			return 2;
		}

		else {
			J->state = BLOCKED; 
			return 1;
		}
	}

	return 0;
}

//To is point, all blocked jobs and ready jobs are in the same queue
//Except that the blocked jobs are stored behind what is ready
void IOBlock_job_update(Job* J, Queue_Job* QJ){
	J->state = BLOCKED;

	//Update CPU burst guess
	estimate_CPU_burst(J);

	//Add it back to queue(fake) to finish its IO work
	add_job_to_queue(J, QJ);
}

//Check if there is no remain jobs or processes in the queue
//Return value:
//1 for sign of completion of simulation
//0 for not
int check_all_job_done(Queue_Process* QP, Queue_Job* QJ){
	
	for(int i = 0; i < QP->size; i++){
		if(QP->processes[i] != NULL) return 0;
	}

	for(int i = 0; i < QJ->size; i++){
		if(QJ->jobs[i] != NULL) return 0;
	}

	return 1;
}

//Let all IO-blocked jobs to do their IO by one s
void do_IO_update(Queue_Job* QJ, int time){
	for(int i = 0; i < QJ->size; i++){
		Job* ptr = (Job *)QJ->jobs[i];

		if(ptr == NULL) continue;
		if(ptr->state == BLOCKED){
				
			char buffer[100];
			
			ptr->IO_burst_time[ptr->index] -= 1;

			if(ptr->IO_burst_time[ptr->index] < 0){
				ptr->index++;
				ptr->state = READY;

				sprintf(buffer, "PID %c, process finishs performing I/O", ptr->PID);
				printf("time %dms: %s [Q <queue-contents>]\n", time, buffer);

				update_CPUburst_job(ptr);
			}
		}
	}

	//If the schedule scheme is SJF or SRT
	if(QJ->type == SJF || QJ->type == SRT){
		qsort(QJ->jobs, QJ->size, sizeof(Job *), compare_job_prior_1);		
	}
}

//Get the info of waiting queue
//UPDATE REQUIRED: Need to change to satisfy the project requirement
void get_Job_Queue(Queue_Job* QJ){
	printf("Queue info: ");
	for(int i = 0; i < QJ->size; i++) {
		if(QJ->jobs[i] != NULL){
			printf(" %c-%d state %d |", QJ->jobs[i]->PID, QJ->jobs[i]->estimate_burst_time,
					QJ->jobs[i]->state);
		}

		else{
			printf(" X-X |");
		}
	}
	printf("\n\n");
}

//Update the arrival time of current CPU burst for a job
void update_arrival_job(Job* J, int time) { J->arrival = time; }

//Update current CPU burst time for a job
void update_CPUburst_job(Job* J) { J->CPU_burst = J->CPU_burst_time[J->index]; }

//Update the finish time of current CPU burst for a job
void update_finish_job(Job* J, int time) { J->finish = time; }

//Update the number of preemption for a job
void update_preemption_job(Job* J) { J->preemption_time++; }

//Update avg time required by this project
//NOTICE: always update when one job finish its current CPU burst
void update_avg_time(Job* J, Summary* S) {
	//CPU burst time
	S->avg_CPU_burst += J->CPU_burst;
	S->avg_CPU_burst /= 2;

	//Turnaround time
	int turnaround = J->finish - J->arrival;
	S->avg_turnaround += turnaround;
	S->avg_turnaround /= 2;

	//Wait time
	int wait = turnaround - J->CPU_burst - 
		(1 + J->preemption_time) * t_cont_switch;

	J->preemption_time = 0;

	S->avg_wait_time += wait;
	S->avg_wait_time /= 2;
}

//Update the total # of preemptions
void update_preemptions(Summary* S) {
	S->num_preemptions++; 
	update_context_switch(S);
}

//Update the total # of content switch
void update_context_switch(Summary* S) {
	S->num_context_switch++;
}
