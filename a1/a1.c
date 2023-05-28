#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>

void transformPermissions(char *permissions, char *octalPermission)
{
	char binaryPermission[10] = "000000000";
	for(int i = 0; permissions[i] != '\0'; i++)
	{
		if(permissions[i] != '-')
		{
			binaryPermission[i] = '1';
		}
		else
		{
			binaryPermission[i] = '0';
		}
	}
	//am cautat pe internet aceasta functie pentru a converti sirul in numarul binar potrivit
	unsigned long decimal_num = strtoul(binaryPermission, NULL, 2); 
	sprintf(octalPermission, "%lo", decimal_num);
}

void listDir(const char *path, int recursive, char *permissions, int size_greater)
{
	DIR *dir = NULL;
	struct dirent *entry = NULL;
	struct stat statbuf;
	dir = opendir(path);
	if(dir == NULL)
	{
		perror("Could not open directory!");
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
		 		if(permissions == NULL)
		 		{
		 			if(size_greater == -1)
		 			{
		 				printf("%s\n", fullPath);
		 				if(recursive == 1)
		 				{
		 					if(S_ISDIR(statbuf.st_mode) == 1)
		 						listDir(fullPath, 1, permissions, size_greater);
		 				}
		 			}
		 			else
		 			{
		 				if(S_ISREG(statbuf.st_mode) == 1 && statbuf.st_size > size_greater)
		 					printf("%s\n", fullPath);
		 				if(recursive == 1)
		 				{
		 					if(S_ISDIR(statbuf.st_mode) == 1)
		 					{
		 						listDir(fullPath, 1, permissions, size_greater);
		 					}
		 				}
		 			}
		 		}
		 		else
		 		{
		 			int length = (strlen(permissions) / 3) + 1;
		 			char octalPermission[length], actualPermission[length];
		 			transformPermissions(permissions, octalPermission);
		 			sprintf(actualPermission, "%o", (statbuf.st_mode & 0777));
		 			if(strcmp(actualPermission, octalPermission) == 0)
		 			{
		 				printf("%s\n", fullPath);
		 				if(recursive == 1)
		 				{
		 					if(S_ISDIR(statbuf.st_mode) == 1)
		 					{
		 						listDir(fullPath, 1, permissions, size_greater);
		 					}
		 				}
		 			}
		 		}
		 	}
		 }
	}

	closedir(dir);
}

int parseSF(const char *path, bool *size_greater_1097, bool parse_command, int *header_length, int **sectionsSizes, int **sectionsOffsets, int *numberOfSections)
{
	*size_greater_1097 = false;
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
		if(parse_command == true)
		{
			printf("ERROR\nwrong magic\n");
		}
		close(fd1);
		return -1;
	}
	lseek(fd1, -3, SEEK_CUR);
	read(fd1, &headerSize, 2);
	lseek(fd1, -headerSize, SEEK_END);
	read(fd1, &version, 2);
	if(version < 36 || version > 102)
	{
		if(parse_command == true)
		{
			printf("ERROR\nwrong version\n");
		}
		close(fd1);
		return -1;
	}
	read(fd1, &nbOfSections, 1);
	if(nbOfSections < 5 || nbOfSections > 16)
	{
		if(parse_command == true)
		{
			printf("ERROR\nwrong sect_nr\n");
		}
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
		read(fd1, &sect_size[i], 4);
		if(sect_size[i] > 1097)
		{
			*size_greater_1097 = true;
		}
	}
	if(sect_type_ok == false)
	{
		if(parse_command == true)
		{
			printf("ERROR\nwrong sect_types\n");
		}
		close(fd1);
		return -1;
	}
	if(parse_command == true)
	{
		printf("SUCCESS\n");
		printf("version=%d\n", version);
		printf("nr_sections=%d\n", nbOfSections);
		for(int i = 0; i < nbOfSections; i++)
		{
			printf("section%d: %s %d %d\n", i+1, sect_name[i], sect_type[i], sect_size[i]);
		}
	}
	*header_length = headerSize;
	*sectionsSizes = sect_size;
	*sectionsOffsets = sect_offset;
	*numberOfSections = nbOfSections;
	close(fd1);
	return 0;
}

void extractSF(const char *path, int section, int line)
{
	int fd1 = -1;
	fd1 = open(path, O_RDONLY);
	if(fd1 == -1)
	{
		printf("ERROR\ninvalid file\n");
		return;
	}
	bool size_greater_1097 = true;
	int header_length = -1, numberOfSections = -1, *sectionsSizes = NULL, *sectionsOffsets = NULL;
	int SF_file = parseSF(path, &size_greater_1097, false, &header_length, &sectionsSizes, &sectionsOffsets, &numberOfSections);
	if(SF_file < 0)
	{
		printf("ERROR\ninvalid file\n");
		return;
	}
	if(numberOfSections < section)
	{
		printf("ERROR\ninvalid section\n");
		return;
	}
	int startSection = sectionsOffsets[section - 1];
	int endSection = sectionsSizes[section - 1];
	int numberOfLines = 0;
	lseek(fd1, startSection, SEEK_SET);
	for(int i = 0; i < endSection; i++)
	{
		char c = 0;
		read(fd1, &c, 1);
		if(c == '\n')
		{
			numberOfLines++;
		}
	}
	numberOfLines++;
	if(numberOfLines < line)
	{
		printf("ERROR\ninvalid line\n");
		return;
	}
	printf("SUCCESS\n");
	char lines[20][100000];
	lseek(fd1, startSection, SEEK_SET);
	int i = 0, j = 0;
	while(i < numberOfLines)
	{
		read(fd1, &lines[i][j], 1);
		if(lines[i][j] == '\n')
		{
			lines[i][j] = '\0';
			i++;
			j = 0;
		}
		else
		{
			j++;
		}
	}
	printf("%s\n", lines[numberOfLines - line]);
	close(fd1);
}

void findall(const char *path)
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
		 		bool size_greater_1097 = true;
		 		int header_length = -1, numberOfSections = -1, *sectionsSizes = NULL, *sectionsOffsets = NULL;
		 		int SF_file = parseSF(fullPath, &size_greater_1097, false, &header_length, &sectionsSizes, &sectionsOffsets, &numberOfSections);
		 		if(SF_file == 0 && size_greater_1097 != true)
		 		{
		 			printf("%s\n", fullPath);
		 		}
		 		if(S_ISDIR(statbuf.st_mode) == 1)
		 		{
		 			findall(fullPath);
		 		}
		 	}
		 }
	}
	closedir(dir);
}

int main(int argc, char **argv)
{
	char *path, *permissions;
	int size_greater = -1, section = -1, line = -1, header_length = -1, numberOfSections = -1;
	int *sectionsSizes = NULL, *sectionsOffsets = NULL;
	bool variable = false;
	if(argc >= 2)
    	{
        	if(strcmp(argv[1], "variant") == 0)
        	{
            		printf("53721\n");
        	}
        	for(int i = 1; i < argc; i++)
        	{
        		if(strcmp(argv[i], "list") == 0)
        		{
        			int doRecursive = 0;
        			for(int i = 1; i < argc; i++)
        			{
        				if(strncmp(argv[i], "path=", 5) == 0)
        				{
        					path = argv[i] + 5;
        				}
        				if(strcmp(argv[i], "recursive") == 0)
    					{
    						doRecursive = 1;
    					}
    					if(strncmp(argv[i], "permissions=", 12) == 0)
    					{
    						permissions = argv[i] + 12;
    					}
    					if(strncmp(argv[i], "size_greater=", 13) == 0)
    					{
    						size_greater = atoi(argv[i] + 13);
    					} 
        			}
        			struct stat statbuf;
        			if(lstat(path, &statbuf) == 0 && S_ISDIR(statbuf.st_mode) == 1)
        			{
        				printf("SUCCESS\n");
        			}
        			else	
        			{
        				printf("ERROR\ninvalid directory path\n");
        				return -1;
        			}
        			listDir(path, doRecursive, permissions, size_greater);
        		}
        		if(strcmp(argv[i], "parse") == 0)
        		{
        			for(int i = 1; i < argc; i++)
        			{
        				if(strncmp(argv[i], "path=", 5) == 0)
        				{
        					path = argv[i] + 5;
        				}
        			}
        			parseSF(path, &variable, true, &header_length, &sectionsSizes, &sectionsOffsets, &numberOfSections);
        		}
        		if(strcmp(argv[i], "extract") == 0)
        		{
        			for(int i = 1; i < argc; i++)
        			{
        				if(strncmp(argv[i], "path=", 5) == 0)
        				{
        					path = argv[i] + 5;
        				}
        				if(strncmp(argv[i], "section=", 8) == 0)
        				{
        					section = atoi(argv[i] + 8);
        				}
        				if(strncmp(argv[i], "line=", 5) == 0)
        				{
        					line = atoi(argv[i] + 5);
        				}
        			}
        			extractSF(path, section, line);
        		}
        		if(strcmp(argv[i], "findall") == 0)
        		{
        			for(int i = 1; i < argc; i++)
        			{
        				if(strncmp(argv[i], "path=", 5) == 0)
        				{
        					path = argv[i] + 5;
        				}
        			}
        			struct stat statbuf;
        			if(lstat(path, &statbuf) == 0 && S_ISDIR(statbuf.st_mode) == 1)
        			{
        				printf("SUCCESS\n");
        			}
        			else	
        			{
        				printf("ERROR\ninvalid directory path\n");
        				return -1;
        			}
        			findall(path);
        		}
    		}
    	}
    	return 0;
}
