#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>

//---------------DEFINING SOME PARAMETERS--------------------

#define INNERLOOP 1000 
#define OUTERLOOP 2000
#define NPERIODICTASKS 3 // number of periodic tasks
#define NAPERIODICTASKS 1 // number of aperiodic tasks
#define NTASKS NPERIODICTASKS + NAPERIODICTASKS

//------------INIZIALIZE FUNCTION OF PERIODIC TASKS--------------------
int task1_code();
int task2_code();
int task3_code();

//------------INIZIALIZE FUNCTION OF APERIODIC TASKS--------------------
int task4_code();

//-----------CHARACTERISTIC FUNCTION OF THE THREADS
void *task1(void *);
void *task2(void *);
void *task3(void *);
void *task4(void *);

//-----------INIZIALIZATION OF MUTEXES AND CONDITIONS (APERIODIC TASKS)-------------------
pthread_mutex_t mutex_task_4 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_task_4 = PTHREAD_COND_INITIALIZER;

//--------------------PRIORITY CEILING MUTEX--------------------------------------

// declaration of mutex
pthread_mutex_t mutex_sem = PTHREAD_MUTEX_INITIALIZER;

// mutex attribute
pthread_mutexattr_t mutexattr_sem;

//-----------ARRAY NEDEED FOR THE CODE------------------------------
long int periods[NTASKS];
struct timespec next_arrival_time[NTASKS];
double WCET[NTASKS];
pthread_attr_t attributes[NTASKS];
pthread_t thread_id[NTASKS];
struct sched_param parameters[NTASKS];

//------- THIS FUNCTION IS USED ONLY TO WASTE TIME DURING THE TASKS' EXECUTION------------

void wastingtime(){

	double wt;
	
	for (int i = 0; i < OUTERLOOP; i++){
 		
 		for (int j = 0; j < INNERLOOP; j++){
 		
			wt = rand() * rand() % 10;
		}
	}
}

//---------MAIN OF THE PROGRAM---------------------------
int main () {

    // SET TASK'S PERIODS IN NANOSECONDS
  	periods[0]= 300000000; //in nanoseconds
  	periods[1]= 500000000; //in nanoseconds
  	periods[2]= 800000000; //in nanoseconds

    // FOR APERIODIC TASKS WE SET THE PERIOD EQUAL TO 0
    periods[3]= 0;

	// IT'S USEFULL TO ASSIGN A NAME TO THE MAXIMUM AND MINIMUM PRIORITY IN THE SYSTEM
  	struct sched_param prio_max;
 	prio_max.sched_priority = sched_get_priority_max(SCHED_FIFO);

  	struct sched_param prio_min;
  	prio_min.sched_priority = sched_get_priority_min(SCHED_FIFO);

  // WE HAVE TO SET THE MAXIMUM PRIORITY TO THE CURRENT THREAD
  // WE HAVE TO CHECK THAT THE MAIN THREAD IS EXECUTED WITH SUPERUSER PRIVILEGES
  // BEFORE ANYTHING ELSE.

	if (getuid() == 0){
	
		pthread_setschedparam(pthread_self(), SCHED_FIFO, &prio_max);
	}

	// OPEN THE FILE
    int fd;
    if ((fd = open("/dev/simple", O_RDWR)) == -1)
    {
        perror("Error opening file");
        return -1;
    }

    // ----------------PRIORITY CEILING--------------------------------
    
    // initialize mutex attributes
    pthread_mutexattr_init(&mutexattr_sem);

    // set mutex protocol to priority ceiling
    pthread_mutexattr_setprotocol(&mutexattr_sem, PTHREAD_PRIO_PROTECT);

    // set the priority ceiling of the mutex to the maximum priority
    pthread_mutexattr_setprioceiling(&mutexattr_sem, prio_min.sched_priority + NTASKS);

    // initialize the mutex
    pthread_mutex_init(&mutex_sem, &mutexattr_sem);

    // STRING WTITTEN TO THE FILE
    char str[100];

    // EXECUTE ALL TASKS IN STAND-ALONE MODALITY FOR EACH TIME, FINDING THE WORST
    // CASE EXECUTION TIME FOR EVERY TASK.

 	int i;
  	for (i =0; i < NTASKS; i++)
    	{
            // initialize time_1 and time_2 required to read the clock
            struct timespec time_1, time_2;
            clock_gettime(CLOCK_REALTIME, &time_1);

            // we should execute each task more than one for computing the WCET periodic tasks
                if (i==0)
                task1_code();
                if (i==1)
                task2_code();
                if (i==2)
                task3_code();
                
                //aperiodic tasks
                if (i==3)
                task4_code();

		    clock_gettime(CLOCK_REALTIME, &time_2);

            // compute the Worst Case Execution Time (in a real case, we should repeat this many times under
            // different conditions, in order to have reliable values

      		WCET[i]= 1000000000*(time_2.tv_sec - time_1.tv_sec)
			       +(time_2.tv_nsec-time_1.tv_nsec);
      		
			
			// write the WCET in the file
			sprintf(str, "Worst Case Execution Time %d=%f", i, WCET[i]);
			if (write(fd, str, strlen(str) + 1) != strlen(str) + 1)
			{
				perror("Error writing file");
				return -1;
        	}
    	}

	// compute U
    double U = WCET[0] / periods[0] + WCET[1] / periods[1] + WCET[2] / periods[2];

    // compute Ulub (no harmonic relationships)
    double Ulub = NPERIODICTASKS * (pow(2.0, (1.0 / NPERIODICTASKS)) - 1);

    // check the sufficient conditions: if they are not satisfied, exit
    if (U > Ulub)
    {
        sprintf(str, "U=%lf Ulub=%lf Non schedulable Task Set", U, Ulub);
        if (write(fd, str, strlen(str) + 1) != strlen(str) + 1)
        {
            perror("Error writing file");
            return -1;
        }
        return (-1);
    }

    sprintf(str, "U=%lf Ulub=%lf Schedulable Task Set", U, Ulub);
    if (write(fd, str, strlen(str) + 1) != strlen(str) + 1)
    {
        perror("Error writing file");
        return -1;
    }

    // close the special file
    close(fd);

    sleep(5);

	// set the minimum priority to the current thread: this is now required because 
	// we will assign higher priorities to periodic threads to be soon created
	// pthread_setschedparam

  	if (getuid() == 0)
    		pthread_setschedparam(pthread_self(),SCHED_FIFO,&prio_min);

  
  	// set the attributes of each task, including scheduling policy and priority
  	for (i =0; i < NPERIODICTASKS; i++)
    	{
			//initializa the attribute structure of task i
      		pthread_attr_init(&(attributes[i]));

			//set the attributes to tell the kernel that the priorities and policies are explicitly chosen,
			//not inherited from the main thread (pthread_attr_setinheritsched) 
      		pthread_attr_setinheritsched(&(attributes[i]), PTHREAD_EXPLICIT_SCHED);
      
			// set the attributes to set the SCHED_FIFO policy (pthread_attr_setschedpolicy)
			pthread_attr_setschedpolicy(&(attributes[i]), SCHED_FIFO);

			//properly set the parameters to assign the priority inversely proportional to the period
      		parameters[i].sched_priority = prio_min.sched_priority+NTASKS - i;
      		
		    // set the attributes and the parameters of the current thread (pthread_attr_setschedparam)
      		pthread_attr_setschedparam(&(attributes[i]), &(parameters[i]));
    	}
	
	// aperiodic tasks
    for (int i = NPERIODICTASKS; i < NTASKS; i++)
    {
        pthread_attr_init(&(attributes[i]));
        pthread_attr_setschedpolicy(&(attributes[i]), SCHED_FIFO);

        // set minimum priority (background scheduling)
        parameters[i].sched_priority = 0;
        pthread_attr_setschedparam(&(attributes[i]), &(parameters[i]));
    }

	// delare the variable to contain the return values of pthread_create	
  	int iret[NTASKS];

	//declare variables to read the current time
	struct timespec time_1;
	clock_gettime(CLOCK_REALTIME, &time_1);

  	// set the next arrival time for each task. This is not the beginning of the first
	// period, but the end of the first period and beginning of the next one. 
  	for (i = 0; i < NPERIODICTASKS; i++)
    	{
			long int next_arrival_nanoseconds = time_1.tv_nsec + periods[i];
			// then we compute the end of the first period and beginning of the next one
			next_arrival_time[i].tv_nsec= next_arrival_nanoseconds%1000000000;
			next_arrival_time[i].tv_sec= time_1.tv_sec + next_arrival_nanoseconds/1000000000;	
    	}

	// create all threads(pthread_create)
  	iret[0] = pthread_create( &(thread_id[0]), &(attributes[0]), task1, NULL);
  	iret[1] = pthread_create( &(thread_id[1]), &(attributes[1]), task2, NULL);
  	iret[2] = pthread_create( &(thread_id[2]), &(attributes[2]), task3, NULL);
   	iret[3] = pthread_create( &(thread_id[3]), &(attributes[3]), task4, NULL);

  	// join all threads (pthread_join)
  	pthread_join( thread_id[0], NULL);
  	pthread_join( thread_id[1], NULL);
  	pthread_join( thread_id[2], NULL);

    //-------------------PRIORITY CEILING---------------------------------
    
    // destroy the mutex
    pthread_mutex_destroy(&mutex_sem);

    exit(0);
}

//-------------------------- APPLICATION OF TASK 1 ------------------------------------------

int task1_code()
{
	// strings to write on the file
    const char *str_init;
    const char *str_final;

    // strings length
    int len_init, len_final;

    // file descriptor
    int fd;

	// open the special file
    if ((fd = open("/dev/simple", O_RDWR)) == -1)
    {
        perror("open failed");
        return -1;
    }

	// write on the special file the id of the current task
    str_init = " 1[ ";
    len_init = strlen(str_init) + 1;
    if (write(fd, str_init, len_init) != len_init)
    {
        perror("write failed");
        return -1;
    }
    // close the file
    close(fd);

	wastingtime();

	/// open the file
    if ((fd = open("/dev/simple", O_RDWR)) == -1)
    {
        perror("open failed");
        return -1;
    }
    // write on the file the id of the current task
    str_final = " ]1 ";
    len_final = strlen(str_final) + 1;
    if (write(fd, str_final, len_final) != len_final)
    {
        perror("write failed");
        return -1;
    }
    // close the file
    close(fd);

    return 0;
}

// THREAD CODE FOR TASK 1 (USED ONLY FOR TEMPORIZATION)
void *task1( void *ptr)
{
	// SET THREAD AFFINITY
    cpu_set_t cset;
    CPU_ZERO(&cset);
    CPU_SET(0, &cset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cset);

    int i;
        for (i = 0; i < 100; i++)
        {
            //-----------------PRIORITY CEILING---------------------------
            
            // take the mutex
            pthread_mutex_lock(&mutex_sem);

            // execute application specific code
            if (task1_code())
            {
                printf("task1_code failed\n");
                fflush(stdout);
                // release the mutex
                pthread_mutex_unlock(&mutex_sem);
                return NULL;
            }

            //-----------------PRIORITY CEILING---------------------------
        
            // release the mutex
            pthread_mutex_unlock(&mutex_sem);

            // SLEEPING UNTIL THE END OF THE CURRENT PERIOD (START OF NEW PERIOD)
            clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &next_arrival_time[0], NULL);

            // THE THREAD IS READY
            long int next_arrival_nanoseconds = next_arrival_time[0].tv_nsec + periods[0];
            next_arrival_time[0].tv_nsec = next_arrival_nanoseconds % 1000000000;
            next_arrival_time[0].tv_sec = next_arrival_time[0].tv_sec + next_arrival_nanoseconds / 1000000000;
        }
}

//-------------------------- APPLICATION OF TASK 2 ------------------------------------------

int task2_code()
{
    // strings to write on the file
    const char *str_init;
    const char *str_final;

    // strings length
    int len_init, len_final;

    // file descriptor
    int fd;

	// open the file
    if ((fd = open("/dev/simple", O_RDWR)) == -1)
    {
        perror("open failed");
        return -1;
    }

	// write on the file the id of the current task
    str_init = " 2[ ";
    len_init = strlen(str_init) + 1;
    if (write(fd, str_init, len_init) != len_init)
    {
        perror("write failed");
        return -1;
    }

    // close the file
    close(fd);

    int i, j;
    double wt;
    for (i = 0; i < OUTERLOOP; i++)
    {
        for (j = 0; j < INNERLOOP; j++)
        {
            wt = rand() * rand() % 10;
        }
    }

    // open the file
    if ((fd = open("/dev/simple", O_RDWR)) == -1)
    {
        perror("open failed");
        return -1;
    }

    // when the random variable uno=0, then aperiodic task 4 must be executed
    if (wt == 0)
    {
        const char *str_4 = ":ex(4)";
        int len_4 = strlen(str_4) + 1;
        if (write(fd, str_4, len_4) != len_4)
        {
            perror("write failed");
            return -1;
        }

        // In theory, we should protect conditions using mutexes. However, in a real-time application, something undesirable may happen.
        // Indeed, when task2 takes the mutex and sends the condition, task4 is executed and is given the mutex by the kernel. Which means
        // that task2 (higher priority) would be blocked waiting for task4 to finish (lower priority). This is of course unacceptable,
        // as it would produced a priority inversion. For this reason, we are not putting mutexes here.

        // pthread_mutex_lock(&mutex_task_4);
        pthread_cond_signal(&cond_task_4);
        // pthread_mutex_unlock(&mutex_task_4);
    }

    // write on the file the id of the current task
    str_final = " ]2 ";
    len_final = strlen(str_final) + 1;
    if (write(fd, str_final, len_final) != len_final)
    {
        perror("write failed");
        return -1;
    }

    // close the file
    close(fd);

    return 0;
}

// THREAD CODE FOR TASK 2 (USED ONLY FOR TEMPORIZATION)
void *task2( void *ptr)
{
	// SET THREAD AFFINITY
    cpu_set_t cset;
    CPU_ZERO(&cset);
    CPU_SET(0, &cset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cset);

    int i = 0;
        for (i = 0; i < 60; i++)
        {
            //------------------PRIORITY CEILING--------------------------

            // take the mutex
            pthread_mutex_lock(&mutex_sem);

            // execute application specific code
            if (task2_code())
            {
                printf("task2_code failed\n");
                fflush(stdout);
                // release the mutex
                pthread_mutex_unlock(&mutex_sem);
                return NULL;
            }

            //------------------PRIORITY CEILING--------------------------
            
            // release the mutex
            pthread_mutex_unlock(&mutex_sem);

            clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &next_arrival_time[1], NULL);
            long int next_arrival_nanoseconds = next_arrival_time[1].tv_nsec + periods[1];
            next_arrival_time[1].tv_nsec = next_arrival_nanoseconds % 1000000000;
            next_arrival_time[1].tv_sec = next_arrival_time[1].tv_sec + next_arrival_nanoseconds / 1000000000;
        }

}

//-------------------------- APPLICATION OF TASK 3 ------------------------------------------

int task3_code()
{
	// strings to write on the file
    const char *str_init;
    const char *str_final;

    // strings length
    int len_init, len_final;

    // file descriptor
    int fd;

	// open the file
    if ((fd = open("/dev/simple", O_RDWR)) == -1)
    {
        perror("open failed");
        return -1;
    }

	// write on the file the id of the current task
    str_init = " 3[ ";
    len_init = strlen(str_init) + 1;
    if (write(fd, str_init, len_init) != len_init)
    {
        perror("write failed");
        return -1;
    }
    // close the file
    close(fd);

	wastingtime();

	// open the file
    if ((fd = open("/dev/simple", O_RDWR)) == -1)
    {
        perror("open failed");
        return -1;
    }

    // write on the file the id of the current task
    str_final = " ]3 ";
    len_final = strlen(str_final) + 1;
    if (write(fd, str_final, len_final) != len_final)
    {
        perror("write failed");
        return -1;
    }
    // close the file
    close(fd);

    return 0;
}

// THREAD CODE FOR TASK 3 (USED ONLY FOR TEMPORIZATION)
void *task3( void *ptr)
{
	// SET THREAD AFFINITY
    cpu_set_t cset;
    CPU_ZERO(&cset);
    CPU_SET(0, &cset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cset);

    int i = 0;
        for (i = 0; i < 40; i++)
        {
            //------------------PRIORITY CEILING--------------------------

            // take the mutex
            pthread_mutex_lock(&mutex_sem);

            // execute application specific code
            if (task3_code())
            {
                printf("task3_code failed\n");
                fflush(stdout);
                // release the mutex
                pthread_mutex_unlock(&mutex_sem);
                return NULL;
            }

            //------------------PRIORITY CEILING--------------------------
            
            // release the mutex
            pthread_mutex_unlock(&mutex_sem);
            
            clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &next_arrival_time[2], NULL);
            long int next_arrival_nanoseconds = next_arrival_time[2].tv_nsec + periods[2];
            next_arrival_time[2].tv_nsec = next_arrival_nanoseconds % 1000000000;
            next_arrival_time[2].tv_sec = next_arrival_time[2].tv_sec + next_arrival_nanoseconds / 1000000000;
        }
}

//-------------------------- APPLICATION OF TASK 4 ------------------------------------------

int task4_code()
{
	// strings to write on the file
    const char *str_init;
    const char *str_final;

    // strings length
    int len_init, len_final;

    // file descriptor
    int fd;

	// open the file
    if ((fd = open("/dev/simple", O_RDWR)) == -1)
    {
        perror("open failed");
        return -1;
    }

	// write on the file the id of the current task
    str_init = " 4[ ";
    len_init = strlen(str_init) + 1;
    if (write(fd, str_init, len_init) != len_init)
    {
        perror("write failed");
        return -1;
    }
    // close the file
    close(fd);

	wastingtime();

	// open the file
    if ((fd = open("/dev/simple", O_RDWR)) == -1)
    {
        perror("open failed");
        return -1;
    }

    // write on the file the id of the current task
    str_final = " ]4 ";
    len_final = strlen(str_final) + 1;
    if (write(fd, str_final, len_final) != len_final)
    {
        perror("write failed");
        return -1;
    }

    // close the file
    close(fd);

    return 0;
}

// THREAD CODE FOR TASK 4 (USED ONLY FOR TEMPORIZATION)
void *task4 (void *ptr)
{
	// SET THREAD AFFINITY
    cpu_set_t cset;
    CPU_ZERO(&cset);
    CPU_SET(0, &cset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cset);

    // Infinite loop
        while (1)
        {
            // wait for the proper condition to be signaled
            pthread_cond_wait(&cond_task_4, &mutex_task_4);

            //------------------PRIORITY CEILING--------------------------
        
            // take the mutex
            pthread_mutex_lock(&mutex_sem);

            // execute the task code
            if (task4_code())
            {
                printf("task4_code failed\n");
                fflush(stdout);
                // release the mutex
                pthread_mutex_unlock(&mutex_sem);
                return NULL;
            }

            //------------------PRIORITY CEILING--------------------------
            
            // release the mutex
            pthread_mutex_unlock(&mutex_sem);
            
        }
}