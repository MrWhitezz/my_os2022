// cpu.c
struct cpu*     mycpu(void);
int     holding(spinlock_t *lk);

// dsa.c
#include <dsa.h>

struct Queue* createQueue(unsigned capacity);
int           isFull     (struct Queue* queue);
int           isEmpty    (struct Queue* queue);
void          enqueue    (struct Queue* queue, void *item);
void         *dequeue    (struct Queue* queue);
void         *front      (struct Queue* queue);
void         *rear       (struct Queue* queue);
