#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <stdbool.h>

#define RESP_PIPE "RESP_PIPE_53721"
#define REQ_PIPE "REQ_PIPE_53721"

int parseSF(char *path, int section_no)
{
	int fd1 = -1;
	fd1 = open(path, O_RDONLY);
	if(fd1 == -1)
	{
		printf("ERROR\ninvalid file\n");
		return -1;
	}
	lseek(fd1, -1, SEEK_END);
	char magic[2];
	int headerSize = 0, version = 0, nbOfSections = 0;
	read(fd1, magic, 1);
	magic[1]='\0';
	if(strncmp(magic, "L", 1) != 0)
	{
		close(fd1);
		return -1;
	}
	lseek(fd1, -3, SEEK_CUR);
	read(fd1, &headerSize, 2);
	lseek(fd1, -headerSize, SEEK_END);
	read(fd1, &version, 2);
	if(version < 36 || version > 102)
	{
		close(fd1);
		return -1;
	}
	read(fd1, &nbOfSections, 1);
	if(nbOfSections < 5 || nbOfSections > 16)
	{
		close(fd1);
		return -1;
	}
	char sect_name[nbOfSections][8];
	int sect_type[nbOfSections], sect_offset[nbOfSections], sect_size[nbOfSections];
	bool sect_type_ok = true;
	for(int i = 0; i < nbOfSections; i++)
	{
		sect_type[i] = 0;
	}
	for(int i = 0; i < nbOfSections; i++)
	{	
		char name[8];
		read(fd1, name, 7);
		sprintf(sect_name[i], "%s", name);
		read(fd1, &sect_type[i], 1);
		if(sect_type[i] != 17 && sect_type[i] != 29 && sect_type[i] != 98 && sect_type[i] != 43 && sect_type[i] != 89 && sect_type_ok == true)
		{	
			sect_type_ok = false;
		}
		read(fd1, &sect_offset[i], 4);
		if(i == section_no)
		{
			return sect_offset[i];
		}
		read(fd1, &sect_size[i], 4);
	}
	if(sect_type_ok == false)
	{
		close(fd1);
		return -1;
	}
	close(fd1);
	return 0;
}

int main()
{
	char *sharedData = NULL, *fileData = NULL;
	unsigned int size;
	int fd1 = -1, fd2 = -1, fd3 = -1;
	if(mkfifo(RESP_PIPE, 0600) != 0)
	{
		printf("ERROR\ncannot create the response pipe\n");
		return 1;
	}
	fd1 = open(REQ_PIPE, O_RDONLY);
	if(fd1 == -1)
	{
		printf("ERROR\ncannot open the request pipe\n");
		return 1;
	}
	fd2 = open(RESP_PIPE, O_WRONLY);
	if(fd2 == -1)
	{
		printf("ERROR\ncannot open the response pipe\n");
		return 1;
	}
	unsigned int string_length = 5;
	char start_message[5] = "START", success_message[7] = "SUCCESS", error_message[5] = "ERROR", value_message[5] = "VALUE", file_name[250];
	write(fd2, &string_length, 1);
	write(fd2, start_message, string_length);
	printf("SUCCESS\n");
	while(1)
	{
		char request_message[30];
		string_length = 10;
		read(fd1, &string_length, 1);
		read(fd1, request_message, string_length);
		request_message[string_length] = '\0';
		if(strncmp("VARIANT", request_message, 7) == 0)
		{
			write(fd2, &string_length, 1);
			write(fd2, request_message, string_length);
			unsigned int variantNumber = 53721;
			write(fd2, &variantNumber, sizeof(unsigned int));
			string_length = 5;
			write(fd2, &string_length, 1);
			write(fd2, value_message, string_length);
		}
		if(strncmp("CREATE_SHM", request_message, 10) == 0)
		{
			unsigned int shmFd = -1;
			const char *nameShm = "/zOzB9hE";
			read(fd1, &shmFd, sizeof(unsigned int));
			shmFd = shm_open(nameShm, O_CREAT | O_RDWR, 0664);
			ftruncate(shmFd, 4542036);
            		if(shmFd == -1)
            		{
                		string_length = 10;
                		write(fd2, &string_length, 1);
                		write(fd2, request_message, string_length);
                		string_length = 5;
                		write(fd2, &string_length, 1);
                		write(fd2, error_message, string_length);
            		}
			else
			{
                		sharedData = mmap(0, 4542036, PROT_WRITE, MAP_SHARED, shmFd, 0);
                		if(sharedData == (void *)-1)
                		{
                    			string_length = 10;
                    			write(fd2, &string_length, 1);
                    			write(fd2, request_message, string_length);
                    			string_length = 5;
                    			write(fd2, &string_length, 1);
                    			write(fd2, error_message, string_length);
                		}
                		string_length = 10;
                		write(fd2, &string_length, 1);
                		write(fd2, request_message, string_length);
                		string_length = 7;
                		write(fd2, &string_length, 1);
                		write(fd2, success_message, string_length);
            		}
		}
		if(strncmp("WRITE_TO_SHM", request_message, 12) == 0)
		{
			unsigned int offset, value;
			read(fd1, &offset, sizeof(unsigned int));
			read(fd1, &value, sizeof(unsigned int));
			if(offset >= 0 && offset <= 4542036 && (offset + 3) < 4542036)
			{
                		memcpy(sharedData + offset, &value, 4);
                		string_length = 12;
                		write(fd2, &string_length, 1);
                		write(fd2, request_message, string_length);
                		string_length = 7;
                		write(fd2, &string_length, 1);
                		write(fd2, success_message, string_length);
            		}
			else
			{ 
                		string_length = 12;
               			write(fd2, &string_length, 1);
                		write(fd2, request_message, string_length);
                		string_length = 5;
                		write(fd2, &string_length, 1);
                		write(fd2, error_message, string_length);
            		}
		}	
		if(strncmp("MAP_FILE", request_message, 8) == 0)
		{
			unsigned int file_size;
			read(fd1, &file_size, 1); 
            		read(fd1, file_name, file_size);
            		file_name[file_size] = '\0';
            		fd3 = open(file_name, O_RDONLY); 
			if(fd3 == -1)
			{
               			string_length = 8;
                		write(fd2, &string_length, 1);
                		write(fd2, request_message, string_length);
                		string_length = 5;
                		write(fd2, &string_length, 1);
                		write(fd2, error_message, string_length);
                		close(fd3);
                		continue;
            		}
            		size = lseek(fd3, 0, SEEK_END);
            		lseek(fd3, 0, SEEK_SET);
            		fileData = (char *)mmap(NULL, size, PROT_READ, MAP_SHARED, fd3, 0);
            		if(fileData == (void *)-1)
            		{
                		string_length = 8;
                		write(fd2, &string_length, 1);
                		write(fd2, request_message, string_length);
               			string_length = 5;
                		write(fd2, &string_length, 1);
                		write(fd2, error_message, string_length); 
                		close(fd3);
                		continue;
            		}
            		string_length = 8;
            		write(fd2, &string_length, 1);
            		write(fd2, request_message, string_length);
           		string_length = 7;
            		write(fd2, &string_length, 1);
            		write(fd2, success_message, string_length); 
            		close(fd3);
		}
		if(strncmp("READ_FROM_FILE_OFFSET", request_message, 21) == 0)
		{
    			unsigned int no_of_bytes, offset;
    			read(fd1, &offset, sizeof(unsigned int));
    			read(fd1, &no_of_bytes, sizeof(unsigned int));
    			int ok = 0;
    			if(sharedData != NULL && fileData != NULL)
   			{
        			if(offset + no_of_bytes <= size)
        			{
        				ok = 1;
            				memcpy(sharedData, fileData + offset, no_of_bytes);
            				string_length = 21;
            				write(fd2, &string_length, 1);
            				write(fd2, request_message, string_length);
            				string_length = 7;
            				write(fd2, &string_length, 1);
            				write(fd2, success_message, string_length);
            				write(fd2, &no_of_bytes, 1);
            				write(fd2, sharedData, no_of_bytes);
        			}
    			}
    			if(ok < 1)
   		 	{
   		 		string_length = 21;
    				write(fd2, &string_length, 1);
    				write(fd2, request_message, string_length);
    				string_length = 5;
    				write(fd2, &string_length, 1);
    				write(fd2, error_message, string_length);
    			}
		}
		if(strncmp("READ_FROM_FILE_SECTION", request_message, 22) == 0)
		{   
			unsigned int section_no, offset, no_of_bytes;
			read(fd1, &section_no, sizeof(unsigned int));
			read(fd1, &offset, sizeof(unsigned int));
    			read(fd1, &no_of_bytes, sizeof(unsigned int));
    			int sect_offset = parseSF(file_name, section_no - 1);
    			if(sect_offset > 0)
    			{
    				memcpy(sharedData, fileData + offset + sect_offset, no_of_bytes);
            			string_length = 22;
            			write(fd2, &string_length, 1);
            			write(fd2, request_message, string_length);
            			string_length = 7;
            			write(fd2, &string_length, 1);
           			write(fd2, success_message, string_length);
            			write(fd2, &no_of_bytes, 1);
            			write(fd2, sharedData, no_of_bytes);
            		}
            		else
            		{
            			string_length = 22;
            			write(fd2, &string_length, 1);
            			write(fd2, request_message, string_length);
            			string_length = 5;
            			write(fd2, &string_length, 1);
            			write(fd2, error_message, string_length);
            		}
        	}
        	if(strncmp("READ_FROM_LOGICAL_SPACE_OFFSET", request_message, 30) == 0)
        	{
        		unsigned int logical_offset, no_of_bytes;
			read(fd1, &logical_offset, sizeof(unsigned int));
			read(fd1, &no_of_bytes, sizeof(unsigned int));
            		string_length = 30;
            		write(fd2, &string_length, 1);
            		write(fd2, request_message, string_length);
            		string_length = 5;
            		write(fd2, &string_length, 1);
            		write(fd2, error_message, string_length);
            		close(fd3);
           		string_length = 7;
            		write(fd1, &string_length, 1);
            		write(fd1, success_message, string_length);
            		close(fd3);
        	}
		if(strncmp("EXIT", request_message, 4) == 0)
		{
			shmdt(sharedData);
			munmap(fileData, size);
    			sharedData = NULL;
    			fileData = NULL;
			close(fd1);
			close(fd2);
			unlink(RESP_PIPE);
			unlink(REQ_PIPE);
			return 0;
		}
	}
	shmdt(sharedData);
	munmap(fileData, size);
    	sharedData = NULL;
    	fileData = NULL;
	close(fd1);
	close(fd2);
	unlink(RESP_PIPE);
	unlink(REQ_PIPE);
	return 0;
}
