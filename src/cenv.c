#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cenv.h"

int count_env;
char** envvars;

int count_rows(FILE* file, int* buffer_size_arg){
	
	int rows = 0;
	const int buffer_size = buffer_size_arg ? *buffer_size_arg : 1024;
	char* buffer = (char*) malloc(sizeof(char) * buffer_size);
	while(1){
		int read = fread(buffer, sizeof(char), buffer_size, file);
		
		if(read < buffer_size && ferror(file))
			return -1;
		
		for(int i=0; i<read; ++i)
			if(buffer[i] == '\n')
				++rows;

		if(read < buffer_size && feof(file))
			break;	
	}

	fseek(file, 0, SEEK_SET);
	free(buffer);
	return rows;
	
}

int load_env_vars(const char* fullpath){
		
	FILE* envfile;
	if(!(envfile = fopen(fullpath, "r"))){
		fprintf(stderr, "Couldn't open file at path %s\n", fullpath);
		return -1;
	}
	
	int row_count;
	if((row_count = count_rows(envfile, NULL)) < 0){
		fprintf(stderr, "Couldn't count the number of rows in file\n");
		return -1;
	}

	envvars = (char**) malloc(sizeof(char*) * row_count);

	int row_buffer_size = 256;
	char row_buffer[row_buffer_size];
	int successes = 0;
	while(fgets(row_buffer, row_buffer_size, envfile)){	
		char* name = strtok(row_buffer, "=");
		if(!name){
			fprintf(stderr, "Invalid format of variable in .env on line %d, missing =\n", successes + 1);
			for(int i=0; i<successes; ++i){
				unsetenv(envvars[i]);
				free(envvars[i]);
			}
			free(envvars);
			return -1;
		}

		char* value = strtok(NULL, "\n");
		if(setenv(name, value, 0) < 0)
			fprintf(stderr, "Error setting variable %s\n", name);
		
		else{
			envvars[successes] = (char*) malloc(sizeof(char) * strlen(name) + 1);
			strcpy(envvars[successes], name);
			++successes;
		}
	}

	for(int i=0; i<row_count; ++i){
		free(envvars[i]);
	}
	free(envvars);
	fclose(envfile);
	count_env = successes;
	return successes;

}

void unload_env_vars(){
	for(int i=0; i<count_env; ++i){
		unsetenv(envvars[i]);
		free(envvars[i]);
	}
	free(envvars);	
}
