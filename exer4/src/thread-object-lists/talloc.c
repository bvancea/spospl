#include <stdlib.h>
#include <pthread.h>
#include "list.h"
#include "task.h"
#include "talloc.h"
#include "dbg.h"

#define INIT_LIST(l)		l = (list_t) malloc(sizeof(struct list)); 	\
					 	 	pthread_mutex_init(&l->lock, NULL)	

static pthread_mutex_t counter_lock = PTHREAD_MUTEX_INITIALIZER;;
static int global_threshold;
static int local_threshold;
static int task_counter = 0;
list_t* local_lists;
list_t global_list;

int threads_nr;

void task_allocator_init(int nr_threads, int global_bound, int local_bound) {
	threads_nr = nr_threads;
	global_threshold = global_bound;
	local_threshold = local_bound;

	list_t* local_lists = (list_t*) malloc(nr_threads * sizeof(list_t));
	for (int i = 0; i < nr_threads; ++i) {
		INIT_LIST(local_lists[i]);
	} 
	INIT_LIST(global_list);
}

task_t* task_malloc(int tid) {
	if (tid >= threads_nr) {
		log_err("Malloc called from a wrong thread");
	}

	task_t* new_task = (task_t*) list_pop_head(local_lists[tid]);
	if (new_task == NULL) {
		new_task = (task_t*) list_safe_pop_head(global_list);
	}
	if (new_task == NULL) {
		new_task = task_malloc_new();
	}

	return NULL;
}

void task_free(task_t* task, int tid) {
	if (tid >= threads_nr) {
		log_err("Malloc called from a wrong thread");
	}

	list_push_back(local_lists[tid],(void*) task);
}

task_t* task_malloc_new() {
	task_t* new_task;
	ALLOCATE_TASK(new_task, task_counter, counter_lock);
	pthread_mutex_init(&new_task->lock, NULL);
	pthread_mutex_init(&new_task->running_lock, NULL);
	return new_task;
}