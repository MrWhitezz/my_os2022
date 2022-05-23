// cpu.c
struct cpu   *mycpu(void);
int           holding(spinlock_t *lk);

// os.c
void          add_task(task_t *task);
task_t       *os_tsk_alloc();
// void          del_task(task_t *task);

// kmt.c
int           get_new_pid();

// dsa.c
#include <dsa.h>

struct Queue* createQueue(unsigned capacity);
int           isFull     (struct Queue* queue);
int           isEmpty    (struct Queue* queue);
void          enqueue    (struct Queue* queue, void *item);
void         *dequeue    (struct Queue* queue);
void         *front      (struct Queue* queue);
void         *rear       (struct Queue* queue);

// irq.c
void          irq_init();

// ucreat.c
int          *ucreate_(task_t *task, const char *name, void *entry);