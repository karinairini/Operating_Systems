#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

void listDir(const char *path, int recursive)
{
	DIR *dir = NULL;
	struct dirent *entry = NULL;
	struct stat statbuf;
	dir = opendir(path);
	if(dir == NULL)
	{
		printf("Could not open directory!");
		return;
	}
	while((entry = readdir(dir)) != NULL)
	{
		if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) 
		{
			char fullPath[512];
		 	snprintf(fullPath, 512, "%s/%s", path, entry->d_name);
		 	if(lstat(fullPath, &statbuf) == 0)
		 	{
		 		//printf("%o\n", statbuf.st_mode & 0777);
		 		//int yeah = statbuf.st_mode & 0777;
		 		//if(yeah == permissions)
		 		//{
		 			printf("%s\n", fullPath);
		 			if(recursive == 1)
		 			{
		 				if(S_ISDIR(statbuf.st_mode) == 1)
		 					listDir(fullPath, 1);
		 			}
		 		//}
		 	}
		}
	}
	closedir(dir);
}

/*void listRec(const char *path)
{
	DIR *dir = NULL;
	struct dirent *entry = NULL;
	struct stat statbuf;
	dir = opendir(path);
	if(dir == NULL)
	{
		printf("Could not open directory!");
		return;
	}
	while((entry = readdir(dir)) != NULL)
	{
		if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) 
		{
			char fullPath[512];
		 	snprintf(fullPath, 512, "%s/%s", path, entry->d_name);
		 	if(lstat(fullPath, &statbuf) == 0)
		 	{
		 		printf("%s\n", fullPath);
		 		if(S_ISDIR(statbuf.st_mode) == 1)
		 			listRec(fullPath);
		 	}
		}
	}
	closedir(dir);
}*/

int main(int argc, char **argv)
{
	char *path;
	//int permissions;
	if(argc >= 2)
    	{
        	if(strcmp(argv[1], "variant") == 0)
        	{
            		printf("53721\n");
        	}
        	if(strcmp(argv[1], "list") == 0)
        	{
        		int doRecursive = 0;
        		for(int i = 2;i < argc; i++)
        		{
        			if(strncmp(argv[i], "path=", 5) == 0)
        			{
        				path = argv[i] + 5;
        				struct stat statbuf;
        				if(lstat(path, &statbuf) == 0 && S_ISDIR(statbuf.st_mode) == 1)
        				{
        					printf("SUCCESS\n");
        				}
        				else	
        				{
        					printf("ERROR\ninvalid directory path");
        					return -1;
        				}
        			}
        			if(strcmp(argv[i], "recursive") == 0)
    				{
    					doRecursive = 1;
    				}
    				//if(strncmp(argv[i], "permissions=", 12) == 0)
    				//{
    					//permissions = 0666;
    				//}
        		}
        		listDir(path, doRecursive);
        	}
    	}
    	return 0;
}
