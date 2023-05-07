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

//semafoare anonime pentru sincronizarea thread-urilor 3, 4 din procesul 7
sem_t semThread3;
sem_t semThread4;

//semafoare cu nume pentru thread-urile 3, 4 din procesul 4 si thread-ul 1 din procesul 7
sem_t *semaphore1;
sem_t *semaphore2;

int runningThreads = 0, foundThread12 = 0, endedThread12 = 0, threadNumber12 = 12;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
sem_t semaphoreP2;

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
		sem_wait(semaphore2); // se asteapta terminarea thread-ului 3 din procesul 4
		info(BEGIN, s->processNumber, s->threadNumber);
		info(END, s->processNumber, s->threadNumber);
		sem_post(semaphore1); // se da drumul thread-ului 4 din procesul 4
	}
	//thread-ul 4 trebuie sa inceapa inaintea thread-ului 3 si sa se incheie dupa terminarea acestuia
	if(s->threadNumber == 3)
	{
		sem_wait(&semThread3); // se asteapta inceperea thread-ului 3
		info(BEGIN, s->processNumber, s->threadNumber);
		info(END, s->processNumber, s->threadNumber);
		sem_post(&semThread4); // se continua executia thread-ului 4
	}
	if(s->threadNumber == 4)
	{
		info(BEGIN, s->processNumber, s->threadNumber); // incepe thread-ul 4
		sem_post(&semThread3); // se da drumul thread-ului 3
		sem_wait(&semThread4); // se asteapta terminarea thread-ului 3, punand in asteptare thread-ul 4
		info(END, s->processNumber, s->threadNumber);
	}
	return NULL;
}

// cel mult 6 thread-uri din procesul 2 pot rula simultan
// thread-ul 12 nu are voie sa se incheie decat in timp ce 6 thread-uri, inclusiv thread-ul 12, ruleaza
void *thread_function_P2(void *arg)
{
	TH_STRUCT *s = (TH_STRUCT *)arg;
	
	// se protejeaza accesul la variabila runningThreads prin intermediul lacatului
	// runningThreads reprezinta numarul de thread-uri care ruleaza la un moment dat
	pthread_mutex_lock(&lock);
	
	// daca sunt deja 6 thread-uri care ruleaza, celelalte se pun in asteptare
	while(runningThreads >= 6)
	{
		pthread_cond_wait(&cond, &lock);
	}
	
    	++runningThreads;
    	pthread_mutex_unlock(&lock);

	// daca thread-ul este altul decat cel cu numarul 12, i se incepe executia
	// astfel se marcheaza ca s-a dat de el, pentru a-l putea gestiona conform cerintei
    	if(s->threadNumber != threadNumber12)
    	{
       		info(BEGIN, s->processNumber, s->threadNumber);
    	} 
    	else
        {
        	foundThread12 = 1;
        }
        
        // se foloseste un semafor pentru a putea intra doar un singur thread in regiunea critica
    	sem_wait(&semaphoreP2);
    
    	if(s->threadNumber != threadNumber12)
    	{
    		if(runningThreads == 6) // daca s-a atins numarul maxim de thread-uri care pot rula simultan
    		{
    			if(foundThread12 == 1 && endedThread12 == 0) // daca s-a dat de thread-ul cu numarul 12 si acesta nu a fost incheiat
    			{
        			info(BEGIN, s->processNumber, threadNumber12); // se incepe executia acestuia
        			info(END, s->processNumber, threadNumber12);  // se incheie executia acestuia cu conditia ca 6 thread-uri inclusiv el sa ruleze simultan
        			pthread_cond_signal(&cond); // se trezeste urmatorul thread in asteptare
        			endedThread12 = 1; // se seteaza variabila endedThread12 pentru a stii ca s-a procesat
    			}
    		}
        	info(END, s->processNumber, s->threadNumber); // se incheie executia celorlalte thread-uri
        	
        	// in regiunea critica se scade numarul thread-urilor care ruleaza
        	pthread_mutex_lock(&lock);
        	--runningThreads;
        	pthread_mutex_unlock(&lock);
        	
    		pthread_cond_signal(&cond); // se trezeste urmatorul thread in asteptare
    	}
    	
    	sem_post(&semaphoreP2);
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
	// thread-ul 3 din procesul 4 trebuie sa se incheie inainte ca thread-ul 1 din procesul 7 sa porneasca
	if(s->threadNumber == 3)
	{
		info(BEGIN, s->processNumber, s->threadNumber);
		info(END, s->processNumber, s->threadNumber); // se incheie thread-ul 3 din procesul 4 
		sem_post(semaphore2); // se porneste thread-ul 1 din procesul 7
	}
	// thread-ul 4 din procesul 4 nu poate incepe decat dupa ce thread-ul 1 din procesul 7 s-a terminat
	if(s->threadNumber == 4)
	{
		sem_wait(semaphore1); // se asteapta terminarea executiei thread-ului 1 din procesul 7
		info(BEGIN, s->processNumber, s->threadNumber);
		info(END, s->processNumber, s->threadNumber);
	}
	return NULL;
}

int main()
{
	init();
    	info(BEGIN, 1, 0);
    	// se initializeaza semafoarele cu 0, pentru a permite intrarea in regiunea critica a mai multor procese, la un moment dat
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
    	if(sem_init(&semaphoreP2, 0, 1) != 0) 
    	{
        	perror("Could not init the semaphore");
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
    		sem_destroy(&semaphoreP2);
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
