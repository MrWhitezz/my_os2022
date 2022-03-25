#include <stddef.h>
#define MAX_ALLOC (16 * (1 << 20))

// Spinlock
typedef int spinlock_t;
#define SPIN_INIT() 0

void spin_lock(spinlock_t *lk); 
void spin_unlock(spinlock_t *lk); 

enum {LK_ALLOC, LK_FREE, LK_SIZE};
typedef struct header_t
{
    void * start;
    int size;
}header_t;

typedef struct __node_t
{
    int              size;
    struct __node_t *next;
} __node_t;


void  list_init();
void* list_alloc(size_t size);
void  drag_node(__node_t *from, __node_t *to);
void  fill_header(header_t *header, void *start, int size);
void  list_free(void *ptr);
void  list_insert(__node_t *node);