#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "a2_helper.h"
#include <pthread.h>
#include <semaphore.h>

void *thread_function(void *arg)
{
	int number = *((int *)arg);
	info(BEGIN, 7, number);
	info(END, 7, number);
	return NULL;
}

int main()
{
    init();
    info(BEGIN, 1, 0);
    pid_t p2 = -1, p3 = -1, p4 = -1, p5 = -1, p6 = -1, p7 = -1, p8 = -1; 
    p2 = fork();
    if(p2 == 0)
    {
    	info(BEGIN, 2, 0);
    	p3 = fork();
    	if(p3 == 0)
    	{
    		info(BEGIN, 3, 0);
    		p7 = fork();
    		if(p7 == 0)
    		{
    			info(BEGIN, 7, 0);
    			pthread_t tids[5];
    			int params[5], i;
    			for(i = 0; i < 5; i++)
    			{
    				params[i] = i + 1;
    				pthread_create(&tids[i], NULL, thread_function, &params[i]);
    			}
    			for(i = 0; i < 5; i++)
    			{
    				pthread_join(tids[i], NULL);
    			}
    			info(END, 7, 0);
    		}
    		else
    		{
    			waitpid(p7, NULL, 0);
    			info(END, 3, 0);
    		}
    	}
    	else
    	{
    		waitpid(p3, NULL, 0);
    		info(END, 2, 0);
    	}
    }
    else
    {
    	waitpid(p2, NULL, 0);
    	p4 = fork();
    	if(p4 == 0)
    	{
    		info(BEGIN, 4, 0);
    		info(END, 4, 0);
    	}
    	else
    	{
    		waitpid(4, NULL, 0);
    		p5 = fork();
    		if(p5 == 0)
    		{
    			info(BEGIN, 5, 0);
    			p6 = fork();
    			if(p6 == 0)
    			{
    				info(BEGIN, 6, 0);
    				info(END, 6, 0);
    			}
    			else
    			{
    				waitpid(p6, NULL, 0);
    				p8 = fork();
    				if(p8 == 0)
    				{
    					info(BEGIN, 8, 0);
    					info(END, 8, 0);
    				}
    				else
    				{
    					waitpid(p8, NULL, 0);
    					info(END, 5, 0);
    				}
    			}
    		}
    		else
    		{
    			waitpid(p5, NULL, 0);
    			info(END, 1, 0);
    		}
    	}
    }
    return 0;
}
