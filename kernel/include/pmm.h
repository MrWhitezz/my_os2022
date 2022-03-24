// Spinlock
#include <am.h>
typedef int spinlock_t;
#define SPIN_INIT() 0

void spin_lock(spinlock_t *lk) { while (atomic_xchg(lk, 1))  ; }
void spin_unlock(spinlock_t *lk) { atomic_xchg(lk, 0); }

typedef struct header_t
{
    int size;
    int magic;
}header_t;

typedef struct __node_t
{
    int              size;
    struct __node_t *next;
} __node_t;


void  list_init();
void* list_alloc(size_t size);
