#include<stdbool.h>

#define MAX_ALLOC (16 *(1 << 20))
#define GPAGE_SZ  (64 * 1024)
#define GMAGIC    0xDEADBEEF
#define SMAGIC    0x55555555
#define addr_leq(a, b) ((uintptr_t)(a) <= (uintptr_t)(b))
#define META_SZ   (8 * 1024)

// spin lock
typedef int spinlock_t;
#define SPIN_INIT() 0

typedef struct S_node_t {
  struct S_node_t *next;
  size_t size;
} S_node_t;

typedef struct S_header_t {
  struct S_node_t *n_head;
  struct S_header_t *next;
  size_t     sz;
  spinlock_t lk;
  char payload[0];
} S_header_t;

typedef struct meta_t {
  void *start;
  void *end;
  bool is_alloc;
  bool is_slab;
} meta_t;


static void *S_alloc(size_t size);
static void *G_alloc(size_t npage, bool is_slab);

// debug

#define assert_nocycle(p) assert(p->next != p)

// typedef union page_t;

// typedef union {
//   struct {
//     page_t *next;
//   } freed;

//   struct {
//     struct S_node_t *n_head;
//     size_t   unit_size;
//     uint32_t magic;
//     char payload[0];
//   } slab;
// } page_t; // 64KB Page


// void slab_init(page_t *page, size_t size) {
// }

