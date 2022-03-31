#include <common.h>
#include <pmm.h>

// Spin lock

void spin_lock(spinlock_t *lk) { while (atomic_xchg(lk, 1))  ; }
void spin_unlock(spinlock_t *lk) { atomic_xchg(lk, 0); }

enum slab_index{ _16B, _32B, _64B, _128B, _256B, _512B, _1KB, _2KB, _4KB, SLAB_SIZE };

S_header_t *Slab[8][SLAB_SIZE];
int n_slab = 0;

spinlock_t G_lock;

size_t nextPower_2(size_t x){
  x = x - 1;
  x = x | (x >> 1);
  x = x | (x >> 2);
  x = x | (x >> 4);
  x = x | (x >> 8);
  x = x | (x >> 16);
  return x + 1;
}

static int get_slab_index(size_t x){
  int index = 0;
  assert(x == nextPower_2(x));
  while (x > 16) {
    x = x >> 1;
    index++;
  }
  return index;
}

static void G_init() {
}

void nhead_init(S_node_t *n_head, size_t size) {
  n_head->next = NULL;
  n_head->size = size;
}

static void S_init() {
  n_slab = cpu_count();
  for (int i = 0; i < n_slab; i++) {
    for (int j = 0; j < SLAB_SIZE; j++) {
      S_header_t *slab = (S_header_t *)G_alloc(1);
      Slab[i][j]   = slab;
      size_t size  = (1 << (j + 4));
      slab->sz     = size;
      slab->n_head = (S_node_t *)ROUNDUP(&(slab->payload), size);
      slab->lk     = SPIN_INIT();
      uintptr_t next_pg = ((uintptr_t)slab) + GPAGE_SZ;
      size_t n_sz  = ROUNDDOWN((next_pg - (uintptr_t)&(slab->payload)), size);
      nhead_init(slab->n_head, n_sz);
    }
  }
}

static void *S_alloc(size_t size){
  int cpu = cpu_current();
  int id  = get_slab_index(size);
  assert(cpu < cpu_count());
  assert(size = nextPower_2(size));

  S_header_t *slab = Slab[cpu][id];
  spinlock_t *lk   = &slab->lk;

  spin_lock(lk);
  S_node_t *node = slab->n_head;

  if (node != NULL) {
    if (node->size >= size) {
      if (node->size == size) {
        slab->n_head = node->next; 
      } else {
        S_node_t *new_node = (S_node_t *)((uintptr_t)node + size);
        new_node->size = node->size - size;
        new_node->next = node->next;
        slab->n_head   = new_node; 
      }
    }
  }

  spin_unlock(lk);
  assert((ROUNDDOWN((uintptr_t)node, size)) == (uintptr_t)node);
  return (void *)node;
}

static void S_free(void *ptr){
  assert(sizeof(S_node_t) <= 16);
  S_header_t *slab = (S_header_t *)(ROUNDDOWN((uintptr_t)ptr, GPAGE_SZ));
  spinlock_t *lk = &slab->lk;
  spin_lock(lk);
  S_node_t *node = (S_node_t *)ptr;
  node->size = slab->sz;
  S_node_t *node_head = slab->n_head;
  S_node_t *prev = NULL;
  if (node_head == NULL) {
    slab->n_head = node; 
    spin_unlock(lk);
    return;
  }
  assert(node_head != NULL);
  while (node_head != NULL) {
    if (addr_leq(node, node_head)) {
      assert(node != node_head);
      node->next = node_head;
      if (prev == NULL) { slab->n_head = node; }
      else { prev->next = node; }
      break;
    }
    prev = node_head;
    node_head = node_head->next;
  }
  // end of list
  if (node_head == NULL) { node->next = NULL; prev->next = node; }
  spin_unlock(lk);
}

// abstraction:
// A set of [li, ri) - free, or allocated
// 
// Allocate sufficient space (pool)
// number of pages
// struct { } page_meta[NUM_PAGES];

static void *G_alloc(size_t npage){
  return NULL;
}

// static void G_free(void *ptr){
// }

static void *kalloc(size_t size) {
  size = nextPower_2(size);
  if (size < 16) size = 16;
  if (size <= 4096) {
    return S_alloc(size);
  } else {
    size_t npage = size / GPAGE_SZ;
    if (npage == 0) npage = 1;
    return G_alloc(npage);
  } 
}

static void kfree(void *ptr) {
  S_free(ptr);
}

#ifndef TEST
static void pmm_init() {
  // memset(lk, SPIN_INIT(), sizeof(lk)); // abuse of SPIN_INIT() == 0
  G_init();
  S_init();
  uintptr_t pmsize = ((uintptr_t)heap.end - (uintptr_t)heap.start);
  printf("Got %d MiB heap: [%p, %p)\n", pmsize >> 20, heap.start, heap.end);

}
#else
// 测试代码的 pmm_init ()

#define HEAP_SIZE (128 * (1 << 20))
static void pmm_init() {
  char *ptr  = malloc(HEAP_SIZE);
  heap.start = ptr;
  heap.end   = ptr + HEAP_SIZE;
  G_init();
  S_init();
  printf("Got %d MiB heap: [%p, %p)\n", HEAP_SIZE >> 20, heap.start, heap.end);

}
#endif

MODULE_DEF(pmm) = {
  .init  = pmm_init,
  .alloc = kalloc,
  .free  = kfree,
};
