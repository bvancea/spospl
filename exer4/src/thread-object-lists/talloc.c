#include <stdlib.h>
#include <pthread.h>
#include "list.h"
#include "task.h"
#include "talloc.h"
#include "dbg.h"

#define INIT_LIST(l)		l = (list_t) malloc(sizeof(struct list)); 	\
							l->head = NULL;							 	\
							l->tail = NULL;								\
							l->size = 0;								\
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

	local_lists = (list_t*) malloc(nr_threads * sizeof(list_t));
	int i;
	for (i = 0; i < nr_threads; ++i) {
		//pthread_mutex_init(&local_lists[i]->lock, NULL);
		INIT_LIST(local_lists[i]);
	} 
	INIT_LIST(global_list);
}

void task_allocator_deinit() {
	int i = 0;
	for (i = 0; i < threads_nr; ++i) {
		list_free(local_lists[i]);
	}
	list_free(global_list);
}

task_t* task_malloc() {

#ifdef USE_MALLOC
	return task_malloc_new();
#else  
	int tid = sched_get()->id;

	if (tid >= threads_nr) {
		log_err("Malloc called from a wrong thread %d %d", tid, threads_nr);
	}

	task_t* new_task = (task_t*) list_pop_head(local_lists[tid]);
	if (new_task == NULL) {
		new_task = (task_t*) list_safe_pop_head(global_list);
	}
	if (new_task == NULL) {
		new_task = task_malloc_new();
	}

	return new_task;
#endif
	
}

void task_free(task_t* task) {
#ifdef USE_MALLOC
	task_destroy(task);
#else 
	scheduler_t* owner = sched_get();
	if (owner == NULL) {
		log_err("Scheduler is NULL");
	}
	int tid = owner->id;
	if (tid >= threads_nr) {
		log_err("Malloc called from a wrong thread");
	}
	list_t list = local_lists[tid];
	list_push_back(list,(void*) task);
	if (list->size > local_threshold) {
		task_move_to_gloabl_list(list, list->size / 2);
		//todo add lock here
		if (global_list->size > global_threshold) {
			task_free_from_global_list(global_threshold / threads_nr);
		}
	}
	
#endif
}
void task_move_to_gloabl_list(list_t from_list, int number) {	
	while (number > 0) {
		list_safe_push_back(global_list, list_pop_head(from_list));
		number--;
	}
}

void task_free_from_global_list(int number) {
	while (number > 0) {
		task_t* task = (task_t*) list_safe_pop_head(global_list);
		task_destroy(task);
		number--;
	}
}

task_t* task_malloc_new() {
	task_t* new_task;
	//ALLOCATE_TASK(new_task, task_counter, counter_lock);

	new_task = (task_t*) malloc(sizeof(task_t));
	new_task->context = (ucontext_t*) malloc(sizeof(ucontext_t)); 	
	ALLOCATE_STACK_UC(new_task->context);
	new_task->children = 0;											
	pthread_mutex_lock(&counter_lock);									
	new_task->id = ++task_counter;                                  
	pthread_mutex_unlock(&counter_lock);	
	pthread_mutex_init(&new_task->lock, NULL);
	pthread_mutex_init(&new_task->running_lock, NULL);
	return new_task;
}

void list_free(list_t list) {
	task_t* task = (task_t*) list_pop_head(list); 
	while(task) {
		task_destroy(task);
		task = (task_t*) list_pop_head(list);
	}
	pthread_mutex_destroy(&list->lock);
	free(list);
}