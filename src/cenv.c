#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cenv.h"

int* count_env = NULL;
int count_calls = 0;
char*** envvars = NULL;

static int count_rows(FILE* file, int* buffer_size_arg){
	
	int rows = 1;
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

static int check_references(const char* value, char** returner){
	char** retval = (char**) malloc(sizeof(char*));
	*returner = NULL;
	int count = 0;
	retval[count] = (char*) malloc(sizeof(char) * strlen(value) + 1);
	strcpy(retval[count], value);
	char* dollar_open_brace = strstr(retval[count], "${");
	char* closed_brace = strstr(retval[count], "}");
	int diff, broken = 0;
	while(dollar_open_brace && closed_brace && 
		 (diff = closed_brace - dollar_open_brace) > 0){
			if(diff - 2 <= 0)
				return -1;
			char ref[diff-1];
			strncpy(ref, dollar_open_brace + 2, diff-2);
			ref[diff-2] = '\0';
			char* var = getenv(ref);
			if(!var){		
				return -2;
			}
			int first_part = dollar_open_brace - retval[count];
			int second_part = retval[count] + strlen(retval[count]) - closed_brace;
			int var_part = strlen(var);
			retval = (char**) realloc(retval, sizeof(retval) + sizeof(char*));
			retval[++count] = (char*) malloc(sizeof(char) * (first_part + second_part + var_part));
			strncpy(retval[count], retval[count-1], dollar_open_brace - retval[count-1]);
			strcat(retval[count], var);
			strcat(retval[count], closed_brace + 1);
			dollar_open_brace = strstr(retval[count] + first_part + var_part, "${");
			closed_brace = strstr(retval[count] + first_part + var_part, "}");
	}

	*returner = strdup(retval[count]);
	for(int i=0; i<=count; ++i)
		free(retval[i]);
	free(retval);
	return 0;
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

	if(!envvars)
		envvars = (char***) malloc(sizeof(char**));
	else
		envvars = (char***) realloc(envvars, sizeof(envvars) + sizeof(char**));
	
	envvars[count_calls] = (char**) malloc(sizeof(char*) * row_count);

	int row_buffer_size = 256;
	char row_buffer[row_buffer_size];
	int successes = 0;
	while(fgets(row_buffer, row_buffer_size, envfile)){	
		char* name = strtok(row_buffer, "=");
		if(!name){
			fprintf(stderr, "Invalid format of variable in .env on line %d, missing =\n", successes + 1);
			for(int i=0; i<successes; ++i){
				unsetenv(envvars[count_calls][i]);
				free(envvars[i]);
			}
			free(envvars);
			return -1;
		}

		char* value = strtok(NULL, "\n");
		char* ref;
		int ref_checker = check_references(value, &ref);
		if(ref_checker < 0){
			fprintf(stderr, "Error populating %s\n", name);
			return -1;
		}
		if(ref)
			value = ref;
		if(setenv(name, value, 0) < 0)
			fprintf(stderr, "Error setting variable %s\n", name);
		
		else{
			envvars[count_calls][successes] = (char*) malloc(sizeof(char) * strlen(name) + 1);
			strcpy(envvars[count_calls][successes], name);
			++successes;
		}
		free(ref);
	}
	fclose(envfile);
	if(!count_env)
		count_env = (int*) malloc(sizeof(int));
	else
		count_env = (int*) realloc(count_env, sizeof(count_env) + sizeof(int));
	count_env[count_calls] = successes;
	++count_calls;
	return successes;

}

void unload_env_vars(){
	for(int i=0; i<count_calls; ++i){
		for(int j=0; j<count_env[i]; ++j){
			unsetenv(envvars[i][j]);
			free(envvars[i][j]);
		}
		free(envvars[i]);
	}
	free(count_env);
	free(envvars);
	envvars = NULL;
	count_env = NULL;
	count_calls = 0;
}
