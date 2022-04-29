// C program for array implementation of queue
#include <common.h>
#include <dsa.h>

// function to create a queue
// of given capacity.
// It initializes size of queue as 0

static void *dalloc(size_t size) {
  return pmm->alloc(size);
}

// static void dfree(void *ptr) {
//   pmm->free(ptr);
// }

struct Queue* createQueue(unsigned capacity)
{
	struct Queue* queue = (struct Queue*)dalloc(
		sizeof(struct Queue));
	queue->capacity = capacity;
	queue->front = queue->size = 0;

	// This is important, see the enqueue
	queue->rear = capacity - 1;
	queue->array = (void **)dalloc(
		queue->capacity * sizeof(void *));
	return queue;
}

// Queue is full when size becomes
// equal to the capacity
int isFull(struct Queue* queue)
{
	return (queue->size == queue->capacity);
}

// Queue is empty when size is 0
int isEmpty(struct Queue* queue)
{
	return (queue->size == 0);
}

// Function to add an item to the queue.
// It changes rear and size
void enqueue(struct Queue* queue, void *item)
{
	if (isFull(queue))
		return;
	queue->rear = (queue->rear + 1)
				% queue->capacity;
	queue->array[queue->rear] = item;
	queue->size = queue->size + 1;
}

// Function to remove an item from queue.
// It changes front and size
void *dequeue(struct Queue* queue)
{
	if (isEmpty(queue))
		return NULL;
	void *item = queue->array[queue->front];
	queue->front = (queue->front + 1)
				% queue->capacity;
	queue->size = queue->size - 1;
	return item;
}

// Function to get front of queue
void *front(struct Queue* queue)
{
	if (isEmpty(queue))
		return NULL;
	return queue->array[queue->front];
}

// Function to get rear of queue
void *rear(struct Queue* queue)
{
	if (isEmpty(queue))
		return NULL;
	return queue->array[queue->rear];
}
