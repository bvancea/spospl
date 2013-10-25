/*
 * printer.c
 *
 *  Created on: Oct 25, 2013
 *      Author: bogdan
 */
#include <stdio.h>
#include <stdlib.h>
#include "task.h"

void print_greeting(void *args) {

	char* message = (char*) args;
	printf("%s\n", message);

	int dummy = 0;
	RETURN(&dummy,int);
}

int main(int argc, char **argv) {
	task_init();

	task_t* t[2];
	char* hello = (char*) malloc(10*sizeof(char));
	char* there = (char*) malloc(10*sizeof(char));
	sprintf(hello, "Hello");
	sprintf(there, "There!");

	int status;
	t[0] = task_spawn(&print_greeting, hello, &status);
	t[1] = task_spawn(&print_greeting, there, &status);
	task_sync(t,2);

	task_deinit();
	return 0;
}
