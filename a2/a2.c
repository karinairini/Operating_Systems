#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "a2_helper.h"
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>

typedef struct {
	int processNumber;
	int threadNumber;
} TH_STRUCT;

sem_t semThread3;
sem_t semThread4;

sem_t *semaphore1;
sem_t *semaphore2;

bool endingThread12 = false;
bool isRunningThread12 = false;

int runningThreads = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

void *thread_function_P7(void *arg)
{
	TH_STRUCT *s = (TH_STRUCT *)arg;
	if(s->threadNumber != 1 && s->threadNumber != 3 && s->threadNumber != 4)
	{
		info(BEGIN, s->processNumber, s->threadNumber);
		info(END, s->processNumber, s->threadNumber);
	}
	if(s->threadNumber == 1)
	{
		sem_wait(semaphore2);
		info(BEGIN, s->processNumber, s->threadNumber);
		info(END, s->processNumber, s->threadNumber);
		sem_post(semaphore1);
	}
	if(s->threadNumber == 3)
	{
		sem_wait(&semThread3);
		info(BEGIN, s->processNumber, s->threadNumber);
		info(END, s->processNumber, s->threadNumber);
		sem_post(&semThread4);
	}
	if(s->threadNumber == 4)
	{
		info(BEGIN, s->processNumber, s->threadNumber);
		sem_post(&semThread3);
		sem_wait(&semThread4);
		info(END, s->processNumber, s->threadNumber);
	}
	return NULL;
}

void *thread_function_P2(void *arg)
{
	TH_STRUCT *s = (TH_STRUCT *)arg;
	
	pthread_mutex_lock(&lock);
	
	while(runningThreads >= (s->threadNumber == 12 ? 6 : 5) || endingThread12)
	{
		pthread_cond_wait(&cond, &lock);
	}
	++runningThreads;
	
	pthread_mutex_unlock(&lock);
	
	if(s->threadNumber != 12)
	{
		info(BEGIN, s->processNumber, s->threadNumber);
		pthread_mutex_lock(&lock);
		
		info(END, s->processNumber, s->threadNumber);
		--runningThreads;
		
		pthread_mutex_unlock(&lock);
		pthread_cond_signal(&cond);
	}
	else
	{
		pthread_mutex_lock(&lock);
		
		endingThread12 = true;
		info(BEGIN, s->processNumber, s->threadNumber);
		endingThread12 = false;
		--runningThreads;
		
		pthread_mutex_unlock(&lock);
		pthread_cond_signal(&cond);
	}
	if(s->threadNumber == 12)
	{
		info(END, s->processNumber, s->threadNumber);
	}
	return NULL;
}

void *thread_function_P4(void *arg)
{
	TH_STRUCT *s = (TH_STRUCT *)arg;
	if(s->threadNumber != 3 && s->threadNumber != 4)
	{
		info(BEGIN, s->processNumber, s->threadNumber);
		info(END, s->processNumber, s->threadNumber);
	}
	if(s->threadNumber == 3)
	{
		info(BEGIN, s->processNumber, s->threadNumber);
		info(END, s->processNumber, s->threadNumber);
		sem_post(semaphore2);
	}
	if(s->threadNumber == 4)
	{
		sem_wait(semaphore1);
		info(BEGIN, s->processNumber, s->threadNumber);
		info(END, s->processNumber, s->threadNumber);
	}
	return NULL;
}

int main()
{
	init();
    	info(BEGIN, 1, 0);
    	sem_unlink("/semaphore1");
    	semaphore1 = sem_open("/semaphore1", O_CREAT, 0644, 0); 
    	if(semaphore1 == NULL) 
    	{
		perror("Could not aquire the semaphore");
		return -2;
    	}
    	sem_unlink("/semaphore2");
    	semaphore2 = sem_open("/semaphore2", O_CREAT, 0644, 0); 
    	if(semaphore2 == NULL) 
    	{
		perror("Could not aquire the semaphore");
		return -2;
    	}
    	pid_t p2 = -1, p3 = -1, p4 = -1, p5 = -1, p6 = -1, p7 = -1, p8 = -1; 
    	p2 = fork();
    	if(p2 == 0)
    	{
    		info(BEGIN, 2, 0);
    		pthread_t tids[48];
    		TH_STRUCT params[48];
    		int i;
    		for(i = 0; i < 48; i++)
    		{
    			params[i].threadNumber = i + 1;
    			params[i].processNumber = 2;
    			pthread_create(&tids[i], NULL, thread_function_P2, &params[i]);
    		}
    		for(i = 0; i < 48; i++)
    		{
    			pthread_join(tids[i], NULL);
    		}
    		p3 = fork();
    		if(p3 == 0)
    		{
    			info(BEGIN, 3, 0);
    			p7 = fork();
    			if(p7 == 0)
    			{
    				info(BEGIN, 7, 0);
    				pthread_t tids[5];
    				TH_STRUCT params[5];
    				int i;
    				for(i = 0; i < 5; i++)
    				{
    					params[i].threadNumber = i + 1;
    					params[i].processNumber = 7;
    					pthread_create(&tids[i], NULL, thread_function_P7, &params[i]);
    				}
    				for(i = 0; i < 5; i++)
    				{
    					pthread_join(tids[i], NULL);
    				}
    				info(END, 7, 0);
    				exit(7);
    			}	
    			else	
    			{
    				waitpid(p7, NULL, 0);
    				info(END, 3, 0);
    				exit(3);
    			}
    		}
    		else
    		{
    			waitpid(p3, NULL, 0);
    			info(END, 2, 0);
    			exit(2);
    		}
    	}
    	p4 = fork();
    	if(p4 == 0)
    	{
    		info(BEGIN, 4, 0);
    		pthread_t tids[5];
    		TH_STRUCT params[5];
    		int i;
    		semaphore1 = sem_open("/semaphore1", 0); 
    		semaphore2 = sem_open("/semaphore2", 0); 
    		for(i = 0; i < 5; i++)
    		{
    			params[i].threadNumber = i + 1;
    			params[i].processNumber = 4;
    			pthread_create(&tids[i], NULL, thread_function_P4, &params[i]);
    		}	
    		for(i = 0; i < 5; i++)
    		{
			pthread_join(tids[i], NULL);
    		}
    		sem_close(semaphore1);
    		sem_close(semaphore2);
    		info(END, 4, 0);
    		exit(4);
    	}	
    	p5 = fork();
    	if(p5 == 0)
    	{
    		info(BEGIN, 5, 0);
    		p6 = fork();
    		if(p6 == 0)
    		{
    			info(BEGIN, 6, 0);
    			info(END, 6, 0);
    			exit(6);
    		}
    		else
    		{
    			waitpid(p6, NULL, 0);
    			p8 = fork();
    			if(p8 == 0)
    			{
    				info(BEGIN, 8, 0);
    				info(END, 8, 0);
    				exit(8);
    			}
    			else
    			{
    				waitpid(p8, NULL, 0);
    				info(END, 5, 0);
    				exit(5);
    			}
    		}
    	}
   	waitpid(p2, NULL, 0);
   	waitpid(p4, NULL, 0); 		
   	waitpid(p5, NULL, 0);
   	info(END, 1, 0);
   	exit(1);
   	return 0;
}
