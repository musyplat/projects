#define _GNU_SOURCE

#include "minispark.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sched.h>

typedef struct LinkedListNode
{
	void *ptr;
	struct LinkedListNode *next;
} LinkedListNode;

typedef struct List
{
	// operations on list: init, append, get, free
	// should be dynamically allocated
	// iteration: next, seek_to_start

	LinkedListNode *head;
	LinkedListNode *curr;
	LinkedListNode *tail;
	int size;
	int capacity;
	pthread_mutex_t linked_list_mutex;
} List;

List *list_init(int capacity)
{
	if (capacity <= 0) {
		printf("Invalid capacity\n");
		return NULL;
	}

	List *ret = malloc(sizeof(List));
	ret->linked_list_mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
	ret->size = 0;
	ret->capacity = capacity;

	LinkedListNode *head = malloc(sizeof(LinkedListNode));
	head->ptr = NULL;
	head->next = NULL;
	ret->head = head;
	ret->curr = head;

	LinkedListNode *current = head;
	for (int i = 1; i < capacity; i++) {
		LinkedListNode *newNode = malloc(sizeof(LinkedListNode));
		newNode->ptr = NULL;
		newNode->next = NULL;
		current->next = newNode;
		current = newNode;
	}

	ret->tail = current;
	return ret;
}

int list_capacity(List *l, int new_capacity)
{
	if (new_capacity <= l->capacity) {
		printf("New capacity must be larger\n");
		return -1;
	}

	LinkedListNode *current = l->tail;
	for (int i = l->capacity; i < new_capacity; i++) {
		LinkedListNode *newNode = malloc(sizeof(LinkedListNode));
		newNode->ptr = NULL;
		newNode->next = NULL;
		current->next = newNode;
		current = newNode;
	}

	if (l->curr == NULL) {
		l->curr = l->tail->next ? l->tail->next : l->tail;
	}

	l->tail = current;
	l->capacity = new_capacity;
	return 0;
}

int list_add_elem(List *l, void *elem)
{
	if (l->size >= l->capacity) {
		list_capacity(l, l->capacity * 2);
	}

	if (l->curr == NULL) {
		printf("Error: No available nodes\n");
		return -1;
	}

	l->curr->ptr = elem;
	l->size++;
	l->curr = l->curr->next;
	return 0;
}

int list_insert_at(List *l, void *elem, int index)
{
	if (index < 0 || index >= l->capacity) { // all precautionary measures, actual code should never have this happen
		printf("Invalid index\n");
		return -1;
	}

	if (l->size >= l->capacity) {
		printf("List capacity full\n");
		return -1;
	}

	LinkedListNode *current = l->head;
	for (int i = 0; i < index; i++) {
		current = current->next;
	}

	pthread_mutex_lock(&l->linked_list_mutex); // TODO added
	if (current->ptr != NULL) {
		printf("Error: Position already occupied\n");
		return -1;
	}

	current->ptr = elem;
	l->size++;
	pthread_mutex_unlock(&l->linked_list_mutex); // TODO added
	return 0;
}

void *get_nth_element(List *l, int n)
{
	if (n < 0 || n >= l->size) {
		return NULL;
	}

	LinkedListNode *current = l->head;
	for (int i = 0; i < n; i++) {
		current = current->next;
	}

	return current->ptr;
}

/* A special mapper */
void *identity(void *arg)
{
	return arg;
}

/* checks if all partitions have been materialized */
int contains_unmaterialized(int *ismaterialized, int num_partitions)
{
	for (int i = 0; i < num_partitions; i++) {
		if (ismaterialized[i] == 0) {
			return -1;
		}
	}
	return 0;
}

void iter_list(Task *task) // jump
{
	RDD *rdd = task->rdd;
	int pnum = task->pnum;
	Transform trans = rdd->trans;
	void *transform_fn = rdd->fn;

	if (trans == MAP || trans == FILTER) {
		if (rdd->partitions == NULL) {
			rdd->partitions = list_init(rdd->dependencies[0]->partitions->capacity);
		}

		RDD *dep = rdd->dependencies[0];
		List *output_partition = list_init(10);

		if (dep->trans == MAP && dep->fn == identity) {
			pthread_mutex_lock(&dep->list_prot);
			FILE *fp = get_nth_element(dep->partitions, pnum);
			pthread_mutex_unlock(&dep->list_prot);
			void* line;
			while ((line = ((Mapper)transform_fn)(fp)) != NULL) {
				list_add_elem(output_partition, line);
			}
		} else { // Handle normal List case
			pthread_mutex_lock(&dep->list_prot);
			List *input_partition = get_nth_element(dep->partitions, pnum);
			pthread_mutex_unlock(&dep->list_prot);
			LinkedListNode *current = input_partition->head;
			while (current) {
				if (current->ptr == NULL) {
					current = current->next;
					continue;
				}
				void *result;
				if (trans == MAP) {
					result = ((Mapper)transform_fn)(current->ptr);
				} else if (trans == FILTER) {
					if (((Filter)transform_fn)(current->ptr, rdd->ctx)) {
						result = current->ptr;
					} else {
						result = NULL;
					}
				}

				if (result) {
					list_add_elem(output_partition, result);
				}
				current = current->next;
			}
		}

		// Store the result
		list_insert_at(rdd->partitions, output_partition, pnum);
	} else if (trans == JOIN) {
		if (rdd->partitions == NULL) {
			rdd->partitions = list_init(rdd->dependencies[0]->partitions->capacity);
		}

		RDD *dep1 = rdd->dependencies[0];
		RDD *dep2 = rdd->dependencies[1];
		pthread_mutex_lock(&dep1->list_prot);
		pthread_mutex_lock(&dep2->list_prot);
		List *part1 = get_nth_element(dep1->partitions, pnum);
		List *part2 = get_nth_element(dep2->partitions, pnum);
		pthread_mutex_unlock(&dep2->list_prot);
		pthread_mutex_unlock(&dep1->list_prot);

		List *output_partition = list_init(10); // arbitrary number, will be doubled as needed

		LinkedListNode *outer = part1->head;
		while (outer) {
			if (outer->ptr == NULL) {
				outer = outer->next;
				continue;
			}
			LinkedListNode *inner = part2->head;
			while (inner) {
				if (inner->ptr == NULL) {
					inner = inner->next;
					continue;
				}
				void *result = ((Joiner)transform_fn)(outer->ptr, inner->ptr, rdd->ctx);
				if (result) {
					list_add_elem(output_partition, result);
				}
				inner = inner->next;
			}
			outer = outer->next;
		}

		list_insert_at(rdd->partitions, output_partition, pnum);
	} else if (trans == PARTITIONBY) {
		if (rdd->partitions == NULL) {
			rdd->partitions = list_init(rdd->numpartitions);
		}

		RDD *dep = rdd->dependencies[0];

		// initializing new lists within each partition as needed
		for (int i = 0; i < rdd->numpartitions; i++) {
			if (get_nth_element(rdd->partitions, i) == NULL) {
				list_insert_at(rdd->partitions, list_init(10), i); // another arbitrary number, will increase if needed
			}
		}

		// process all input partitions, adds them to the correct partition (returned by Partitioner), only 1 task for this
		for (int i = 0; i < dep->partitions->capacity; i++) {
			pthread_mutex_lock(&dep->list_prot);
			List *input_part = get_nth_element(dep->partitions, i);
			pthread_mutex_unlock(&dep->list_prot);
			LinkedListNode *current = input_part->head;

			while (current) {
				if (current->ptr == NULL) {
					current = current->next;
					continue;
				}
				unsigned long target_part = ((Partitioner)transform_fn)(
					current->ptr,
					rdd->numpartitions,
					rdd->ctx);
				pthread_mutex_lock(&rdd->list_prot);
				List *target = get_nth_element(rdd->partitions, target_part);
				pthread_mutex_unlock(&rdd->list_prot);
				list_add_elem(target, current->ptr);
				current = current->next;
			}
		}
	}

	if (rdd->trans == PARTITIONBY) {
		pthread_mutex_lock(&rdd->list_prot);
		for (int i = 0; i < rdd->numpartitions; i++) {
			rdd->ismaterialized[i] = 1;
		}
		rdd->fullymaterialized = 1;
		pthread_mutex_unlock(&rdd->list_prot);
	} else {
		pthread_mutex_lock(&rdd->list_prot);
		rdd->ismaterialized[pnum] = 1;
		// check if all partitions are done
		if (contains_unmaterialized(rdd->ismaterialized, rdd->partitions->capacity) == 0) {
			rdd->fullymaterialized = 1;
		}
		pthread_mutex_unlock(&rdd->list_prot);
	}
}

typedef struct Node
{
	Task *task;
	struct Node *next;
} Node;

typedef struct Queue
{
	Node *head;
	Node *tail;
	pthread_mutex_t mutex; // for popping the head and updating into the new head
	pthread_cond_t cond;   // for waking up the threads when a task is added to the queue
} Queue;

typedef struct ThreadPool {
    Queue* work_queue;
    pthread_t *threads;
    int *thread_status; /* 0 = ready, 1 = working, -1 = terminate */
    pthread_mutex_t status_mutex;
	pthread_cond_t main_cond;
    int num_threads;
    int active_count;
} ThreadPool;

struct ThreadPool *threads;
struct MetricQueue *metric_queue;

// Working with metrics...
// Recording the current time in a `struct timespec`:
//    clock_gettime(CLOCK_MONOTONIC, &metric->created);
// Getting the elapsed time in microseconds between two timespecs:
//    duration = TIME_DIFF_MICROS(metric->created, metric->scheduled);
// Use `print_formatted_metric(...)` to write a metric to the logfile.
void print_formatted_metric(TaskMetric *metric, FILE *fp)
{
	fprintf(fp, "RDD %p Part %d Trans %d -- creation %10jd.%06ld, scheduled %10jd.%06ld, execution (usec) %ld\n",
			metric->rdd, metric->pnum, metric->rdd->trans,
			metric->created.tv_sec, metric->created.tv_nsec / 1000,
			metric->scheduled.tv_sec, metric->scheduled.tv_nsec / 1000,
			metric->duration);
}

typedef struct MetricNode {
    TaskMetric *metric;
    struct MetricNode *next;
} MetricNode;

typedef struct MetricQueue {
    MetricNode *head;
    MetricNode *tail;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
	pthread_t thread;
	int status; // 0 when the RDD is fully set
	FILE* fp;
} MetricQueue;

/* only 1 thread will be surveying this area */
void* metric_thread_function() {
	while (1) {
		pthread_mutex_lock(&metric_queue->mutex);
		while (metric_queue->head == NULL) {
			if (!metric_queue->status) {
				pthread_mutex_unlock(&metric_queue->mutex);
				pthread_exit(NULL);
			}
			pthread_cond_wait(&metric_queue->cond, &metric_queue->mutex);
		}
		MetricNode *node = metric_queue->head;
		metric_queue->head = metric_queue->head->next;
		if (metric_queue->head == NULL) {
			metric_queue->tail = NULL;
		}
		pthread_mutex_unlock(&metric_queue->mutex);
		print_formatted_metric(node->metric, metric_queue->fp);
		free(node->metric);
		free(node);
	}
	return NULL;
}

void metric_queue_init() {
    metric_queue = malloc(sizeof(MetricQueue));
	metric_queue->status = 1;
	metric_queue->fp = fopen("metrics.log", "w");
	if (metric_queue->fp == NULL) {
		printf("fopen");
		exit(-1);
	}
    metric_queue->head = metric_queue->tail = NULL;
    pthread_mutex_init(&metric_queue->mutex, NULL);
    pthread_cond_init(&metric_queue->cond, NULL);
	if (pthread_create(&metric_queue->thread, NULL, metric_thread_function, NULL) != 0) {
		printf("pthread_create");
		exit(-1);
	}
}

void metric_queue_add(TaskMetric* taskmetric) {
	pthread_mutex_lock(&metric_queue->mutex);
	MetricNode* temp = malloc(sizeof(MetricNode));
	temp->metric = taskmetric;
	temp->next = NULL;
	MetricNode* tail = metric_queue->tail;
	if (tail == NULL) {
		metric_queue->head = temp;
		metric_queue->tail = temp;
	} else {
		metric_queue->tail->next = temp;
		metric_queue->tail = temp;
	}
	pthread_cond_signal(&metric_queue->cond);
	pthread_mutex_unlock(&metric_queue->mutex);
}

void metric_queue_clean() {
	pthread_mutex_lock(&metric_queue->mutex);
	metric_queue->status = 0;
	pthread_cond_signal(&metric_queue->cond);
	pthread_mutex_unlock(&metric_queue->mutex);

	pthread_join(metric_queue->thread, NULL);
	fclose(metric_queue->fp);
	pthread_mutex_destroy(&metric_queue->mutex);
	pthread_cond_destroy(&metric_queue->cond);
	MetricNode *current = metric_queue->head;
	while (current != NULL) {
		MetricNode *temp = current;
		current = current->next;
		free(temp->metric);
		free(temp);
	}
	free(metric_queue);
}

Task* pop_queue() // this function will only be called from thread_function, we are assuming we hold the mutex
{
	if (threads->work_queue->head == NULL) {
		return NULL;
	}
	Node *job = threads->work_queue->head;
	if (threads->work_queue->head == threads->work_queue->tail) {
		threads->work_queue->tail = NULL;
	}
	threads->work_queue->head = threads->work_queue->head->next;
	return job->task;
}

void *thread_function(void *arg) { // jump
    long thread_id = (long)arg;
    
    while (1) {
        pthread_mutex_lock(&threads->work_queue->mutex);
        
        // wait for work
        while (threads->work_queue->head == NULL) {
			if (threads->active_count == 0) {
				pthread_cond_signal(&threads->main_cond);
			}
            pthread_cond_wait(&threads->work_queue->cond, &threads->work_queue->mutex);
            
            // check for termination signal
            if (threads->thread_status[thread_id] == -1) {
                pthread_mutex_unlock(&threads->work_queue->mutex);
                pthread_exit(NULL);
            }
        }

		Task *task = pop_queue();
		// update status to working
		pthread_mutex_lock(&threads->status_mutex);
		threads->thread_status[thread_id] = 1;
		threads->active_count++;
		pthread_mutex_unlock(&threads->status_mutex);
		pthread_mutex_unlock(&threads->work_queue->mutex);

		if (task) {
            iter_list(task);
            
            // update metrics
            struct timespec end_time;
            clock_gettime(CLOCK_MONOTONIC, &end_time);
            task->metric->duration = TIME_DIFF_MICROS(task->metric->scheduled, end_time);
            metric_queue_add(task->metric);
			pthread_cond_signal(&metric_queue->cond);
        }
		pthread_cond_signal(&threads->main_cond);

		// update status to ready
		pthread_mutex_lock(&threads->status_mutex);
		threads->thread_status[thread_id] = 0;
		threads->active_count--;
		pthread_mutex_unlock(&threads->status_mutex);
	}
	return NULL;
}

/* Create the pool with numthreads threads. Do any necessary allocations. */
void thread_pool_init(int num_threads) {
    threads = malloc(sizeof(ThreadPool));
    threads->work_queue = malloc(sizeof(Queue));
    pthread_mutex_init(&threads->work_queue->mutex, NULL);
    pthread_cond_init(&threads->work_queue->cond, NULL);
    threads->work_queue->head = threads->work_queue->tail = NULL;

    threads->threads = malloc(num_threads * sizeof(pthread_t));
    threads->thread_status = calloc(num_threads, sizeof(int));
    pthread_mutex_init(&threads->status_mutex, NULL);
	pthread_cond_init(&threads->main_cond, NULL);
    threads->num_threads = num_threads;
    threads->active_count = 0;

    for (int i = 0; i < num_threads; i++) {
        if (pthread_create(&threads->threads[i], NULL, thread_function, (void*)(long)i) != 0) {
			printf("pthread_create");
            exit(-1);
        }
    }
}

/* Join all the threads and deallocate any memory used by the pool. */
void thread_pool_destroy() {
    // signal all threads to terminate
    pthread_mutex_lock(&threads->status_mutex);
    for (int i = 0; i < threads->num_threads; i++) {
        threads->thread_status[i] = -1;
    }
	pthread_mutex_unlock(&threads->status_mutex);
    
    // wake all threads
    pthread_cond_broadcast(&threads->work_queue->cond);
    
    // join all threads
    for (int i = 0; i < threads->num_threads; i++) {
        pthread_join(threads->threads[i], NULL);
    }
    
    // cleanup
    pthread_mutex_destroy(&threads->work_queue->mutex);
    pthread_cond_destroy(&threads->work_queue->cond);
    pthread_mutex_destroy(&threads->status_mutex);
	pthread_cond_destroy(&threads->main_cond);
    free(threads->threads);
    free(threads->thread_status);
    free(threads->work_queue);
    free(threads);
}

/* Returns when the work queue is empty and all threads have finished their tasks. */
void thread_pool_wait()
{
	while (threads->work_queue->head != NULL) { } // wait for the queue to be empty
	// queue is now empty, must wait for all threads to finish their jobs
	while (threads->active_count > 0) { }
	return;
}

/* Adds a task to the work queue. */
void thread_pool_submit(Task *task)
{
	Node *temp = malloc(sizeof(Node));
	temp->task = task;
	temp->next = NULL;
	Node *tail = threads->work_queue->tail;
	if (tail == NULL) {
		threads->work_queue->head = temp;
		threads->work_queue->tail = temp;
	} else {
		threads->work_queue->tail->next = temp;
		threads->work_queue->tail = temp;
	}

	pthread_cond_signal(&threads->work_queue->cond);
	return;
}



int max(int a, int b)
{
	return a > b ? a : b;
}

RDD *create_rdd(int numdeps, Transform t, void *fn, ...)
{
	RDD *rdd = malloc(sizeof(RDD));
	if (rdd == NULL) {
		printf("error mallocing new rdd\n");
		exit(1);
	}

	va_list args;
	va_start(args, fn);

	int maxpartitions = 0;
	for (int i = 0; i < numdeps; i++) {
		RDD *dep = va_arg(args, RDD *);
		rdd->dependencies[i] = dep;
		maxpartitions = max(maxpartitions, dep->partitions->capacity);
	}
	va_end(args);

	rdd->numdependencies = numdeps;
	rdd->trans = t;
	rdd->fn = fn;
	rdd->partitions = NULL;
	rdd->list_prot = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;

	rdd->ismaterialized = calloc(maxpartitions, sizeof(int));
	if (rdd->ismaterialized == NULL) {
		printf("malloc error\n");
		exit(1);
	}

	rdd->addedtoqueue = calloc(maxpartitions, sizeof(int));
	if (rdd->addedtoqueue == NULL) {
		printf("malloc error\n");
		exit(1);
	}

	rdd->fullymaterialized = 0;

	return rdd;
}

/* RDD constructors */
RDD *map(RDD *dep, Mapper fn)
{
	RDD* rdd = create_rdd(1, MAP, fn, dep);
	rdd->partitions = list_init(dep->partitions->capacity);
	return rdd;
}

RDD *filter(RDD *dep, Filter fn, void *ctx)
{
	RDD *rdd = create_rdd(1, FILTER, fn, dep);
	rdd->partitions = list_init(dep->partitions->capacity);
	rdd->ctx = ctx;
	return rdd;
}

RDD *partitionBy(RDD *dep, Partitioner fn, int numpartitions, void *ctx)
{
	RDD *rdd = create_rdd(1, PARTITIONBY, fn, dep);
	rdd->partitions = list_init(numpartitions);
	rdd->numpartitions = numpartitions;
	rdd->ctx = ctx;
	free(rdd->ismaterialized);
	rdd->ismaterialized = calloc(numpartitions, sizeof(int));
	if (rdd->ismaterialized == NULL) {
		printf("malloc error\n");
		exit(1);
	}
	free(rdd->addedtoqueue);
	rdd->addedtoqueue = calloc(numpartitions, sizeof(int));
	if (rdd->addedtoqueue == NULL) {
		printf("malloc error\n");
		exit(1);
	}
	return rdd;
}

RDD *join(RDD *dep1, RDD *dep2, Joiner fn, void *ctx)
{
	RDD *rdd = create_rdd(2, JOIN, fn, dep1, dep2);
	rdd->partitions = list_init(dep1->partitions->capacity); // could be dep1 or dep2
	rdd->ctx = ctx;
	return rdd;
}

/* Special RDD constructor.
 * By convention, this is how we read from input files. */
RDD *RDDFromFiles(char **filenames, int numfiles)
{
	RDD *rdd = malloc(sizeof(RDD));
	rdd->partitions = list_init(numfiles);

	for (int i = 0; i < numfiles; i++) {
		FILE *fp = fopen(filenames[i], "r");
		if (fp == NULL) {
			perror("fopen");
			exit(1);
		}
		list_add_elem(rdd->partitions, fp);
	}

	rdd->numdependencies = 0;
	rdd->trans = MAP;
	rdd->fn = (void *)identity;
	rdd->list_prot = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;

	rdd->ismaterialized = calloc(numfiles, sizeof(int));
	if (rdd->ismaterialized == NULL) {
		printf("malloc error\n");
		exit(1);
	}

	for (int i = 0; i < numfiles; i++) {
		rdd->ismaterialized[i] = 1; // all partitions are materialized
	}

	rdd->addedtoqueue = calloc(numfiles, sizeof(int));
	if (rdd->addedtoqueue == NULL) {
		printf("malloc error\n");
		exit(1);
	}

	for (int i = 0; i < numfiles; i++) {
		rdd->addedtoqueue[i] = 1; // all partitions are not added to queue
	}

	rdd->fullymaterialized = 1;

	return rdd;
}

/* NEW INFO UNLOCKED: each RDD should have a Task PER partition to be added into queue */
Task *init_task(RDD *rdd, int pnum)
{
	Task *task = malloc(sizeof(Task));
	task->rdd = rdd;
	task->pnum = pnum;
	task->metric = malloc(sizeof(TaskMetric));
	clock_gettime(CLOCK_MONOTONIC, &task->metric->created);
	task->metric->rdd = rdd;
	task->metric->pnum = pnum;
	task->metric->duration = 0;
	return task;
}

/* queues all ready partitions (previous partition(s) have already been materialized AND not partitioner type) */
void queue_ready_partitions(RDD *rdd)
{
	if (rdd->fullymaterialized == 1) {
		return;
	}

	for (int i = 0; i < rdd->partitions->capacity; i++) {
		pthread_mutex_lock(&rdd->list_prot);
		if (rdd->ismaterialized[i] || rdd->addedtoqueue[i]) {
			pthread_mutex_unlock(&rdd->list_prot);
			continue;
		}
		pthread_mutex_unlock(&rdd->list_prot);

		int dependencies_ready = 1;
		for (int dep = 0; dep < rdd->numdependencies; dep++) {
			RDD *child = rdd->dependencies[dep];

			if (rdd->trans == MAP || rdd->trans == FILTER) {
				if (!child->ismaterialized[i]) {
					dependencies_ready = 0;
					break;
				}
			} else if (rdd->trans == JOIN) { // since JOIN requires both dependencies, the for loop doesn't matter, but the structure is already set so its inefficient ._.
				RDD* child1 = rdd->dependencies[0];
				RDD* child2 = rdd->dependencies[1];
				pthread_mutex_lock(&child1->list_prot);
				pthread_mutex_lock(&child2->list_prot);
				if (!child1->ismaterialized[i] || !child2->ismaterialized[i]) {
					dependencies_ready = 0;
					pthread_mutex_unlock(&child2->list_prot);
					pthread_mutex_unlock(&child1->list_prot);
					break;
				}
				pthread_mutex_unlock(&child2->list_prot);
				pthread_mutex_unlock(&child1->list_prot);
			} else { // PARTITIONBY only, which requires its (ONLY) dependency to be fully materialized
				if (!child->fullymaterialized) {
					dependencies_ready = 0;
					break;
				}
			}
		}

		if (dependencies_ready) {
			Task *task = init_task(rdd, i);
			thread_pool_submit(task);
			if (rdd->trans == PARTITIONBY) {
				for (int j = 0; j < rdd->partitions->capacity; j++) {
					rdd->addedtoqueue[j] = 1;
				}
			} else {
				rdd->addedtoqueue[i] = 1;
			}
			clock_gettime(CLOCK_MONOTONIC, &task->metric->scheduled);
		}
	}
}

/* recursively iterates through given RDD, adding ready partitions into Task queue */
void iterate_rdd(RDD *rdd)
{
	if (rdd->fullymaterialized) {
		return;
	}

	for (int i = 0; i < rdd->numdependencies; i++) {
		iterate_rdd(rdd->dependencies[i]);
	}

	queue_ready_partitions(rdd);

	if (contains_unmaterialized(rdd->ismaterialized, rdd->partitions->capacity) == 0) {
		rdd->fullymaterialized = 1;
	}
}

void main_thread_fcn(RDD *rdd) // jump
{
    pthread_mutex_lock(&threads->work_queue->mutex);
    while (rdd->fullymaterialized != 1) {
        // Submit all ready tasks first
        iterate_rdd(rdd);

        // Wait only if there are active workers or pending tasks
        if (threads->active_count > 0 || threads->work_queue->head != NULL) {
            pthread_cond_wait(&threads->main_cond, &threads->work_queue->mutex);
        }
    }
    pthread_mutex_unlock(&threads->work_queue->mutex);
}

void execute(RDD *rdd)
{
	main_thread_fcn(rdd);
	/*
	 how will the locking go ?
	 once we call threads on all the leaf nodes (reading from files), and threads with dependencies show up, then we can start placing them in condition variables (if we go from order we add them,
	 then it is guaranteed that once our index lands on a certain index, all of its dependencies are already being called upon at the moment, so we can safely place in a CV to be sleeping)

	 we can just add to the head of the list every single time with a helper function
	 */

	/*
	algorithm: taking the list that's returned from iterate_rdd, start at the end (all the root nodes)
	different algorithm: every run through, check the rdd, if it's all fulfilled on dependencies, add it to the queue
	with this algorithm, we declare a second CV (solely for this thread) which is called once the Task queue is empty
	when it's empty, we can restart the loop (maybe make this an infinite loop?) which allows us to call iterate_rdd again
	we will also change iterate_rdd to now just add all RDDs that have their dependencies filled to the Task queue
	*/

	return;
}

void MS_Run()
{
	// Create a thread pool, work queue, and worker threads
	
	cpu_set_t set;
	CPU_ZERO(&set);
	if (sched_getaffinity(0, sizeof(set), &set) == -1) {
		perror("sched_getaffinity");
		exit(1);
	}

	int cores_available = CPU_COUNT(&set);
	thread_pool_init(cores_available - 1); // 1 for the metric thread

	// Create the task metric queue and start the metrics monitor thread
	metric_queue_init();
	return;
}

void MS_TearDown()
{
	// Destroy the thread pool.
	// Wait for the metrics thread to finish and join it.
	// Free any allocations (thread pools, queues, RDDs).
	metric_queue_clean();
	thread_pool_wait();
	thread_pool_destroy();
	// handle freeing allocatings in thread_pool_destroy
}

int count(RDD *rdd)
{
	execute(rdd);

	int count = 0;
	// count all the items in rdd
	for (int i = 0; i < rdd->partitions->capacity; i++) {
		List *part = get_nth_element(rdd->partitions, i);
		LinkedListNode *current = part->head;
		while (current) {
			if (current->ptr != NULL) {
				count++;
			}
			current = current->next;
		}
	}
	return count;
}

void print(RDD *rdd, Printer p)
{
	execute(rdd);
	// print all the items in rdd
	// aka... `p(item)` for all items in rdd
	for (int i = 0; i < rdd->partitions->capacity; i++) {
		List *part = get_nth_element(rdd->partitions, i);
		LinkedListNode *current = part->head;
		while (current) {
			if (current->ptr != NULL) {
				p(current->ptr);
			}
			current = current->next;
		}
	}
}

