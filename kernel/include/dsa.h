#ifndef __DSA_H__
#define __DSA_H__

typedef struct Queue queue_t;
// A structure to represent a queue
struct Queue {
	int front, rear, size;
	unsigned capacity;
	void **array;
};

#endif