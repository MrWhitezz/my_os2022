// abstraction:
// A set of [li, ri) - free, or allocated
// 
// Allocate sufficient space (pool)
// number of pages
// struct { } page_meta[NUM_PAGES];

#include <common.h>
#include <pmm.h>

// Spin lock

void spin_lock(spinlock_t *lk) { while (atomic_xchg(lk, 1))  ; }
void spin_unlock(spinlock_t *lk) { atomic_xchg(lk, 0); }

enum slab_index{ _16B, _32B, _64B, _128B, _256B, _512B, _1KB, _2KB, _4KB, SLAB_SIZE };

S_header_t *Slab[8][SLAB_SIZE];
int n_slab = 0;

void      *G_start;
spinlock_t G_lock;
meta_t    *Meta;
int        n_meta = 0;

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
  assert(index < SLAB_SIZE);
  return index;
}

static int get_meta_index(void *p){
  assert(ROUNDDOWN(p, GPAGE_SZ) == (uintptr_t)p);
  return (p - G_start) / GPAGE_SZ;
}

static bool is_valid_ret(void *p){
  assert(p != NULL);
  if (p < G_start) return false;
  if (p >= G_start + (n_meta * GPAGE_SZ)) return false;
  return true;
}

static void meta_init(){
  size_t sz = sizeof(meta_t) * META_SZ;
  Meta      = (meta_t *)(heap.start);
  G_start   = (void *)((uintptr_t)Meta + sz);
  G_start   = (void *)ROUNDUP(G_start, GPAGE_SZ); // last change
  for (int i = 0; i < META_SZ; i++) {
    if (G_start + (i + 1) * GPAGE_SZ > heap.end) {
      n_meta = i;
      return;
    }
    Meta[i].start    = (G_start + i * GPAGE_SZ);
    Meta[i].end      = NULL;
    Meta[i].is_alloc = false;
    Meta[i].is_slab  = false;
  }
  assert(0);
}

static void G_init() {
  G_lock = SPIN_INIT();
  meta_init();
}

static void nhead_init(S_node_t *n_head, size_t size) {
  n_head->next = NULL;
  n_head->size = size;
}

static void S_init() {
  n_slab = cpu_count();
  for (int i = 0; i < n_slab; i++) {
    for (int j = 0; j < SLAB_SIZE; j++) {
      S_header_t *slab = (S_header_t *)G_alloc(1, true);
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
    if (node->size == size) {
      slab->n_head = node->next; 
    } else {
      assert(node->size > size);
      S_node_t *new_node = (S_node_t *)((uintptr_t)node + size);
      new_node->size = node->size - size;
      new_node->next = node->next;
      slab->n_head   = new_node; 
    }
  }

  spin_unlock(lk);
  assert((ROUNDDOWN((uintptr_t)node, size)) == (uintptr_t)node);
  return (void *)node;
}

static void S_free(void *ptr){
  assert(0);
  return;
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

static bool try_alloc(void *ret, size_t sz, bool is_slab){
  assert(ROUNDDOWN(ret, sz) == (uintptr_t)ret);
  assert(sz > 0 && ROUNDUP(sz, GPAGE_SZ) == sz);
  assert(is_valid_ret(ret));
  int id = get_meta_index(ret);
  assert(id >= 0 && id < n_meta);
  meta_t *meta = &Meta[id];
  assert(meta->start == ret);
  size_t n_pg = sz / GPAGE_SZ;
  for (int i = 0; i < n_pg; ++i){
    if (meta[i].is_alloc == true) return false;
  }
  assert(meta->is_alloc == false);
  return true;
}

static void* slow_alloc(void *ret, size_t sz, bool is_slab){
  int id = get_meta_index(ret);
  assert(id >= 0 && id < n_meta);
  meta_t *meta = &Meta[id];
  assert(meta->is_alloc == false);
  size_t n_pg = sz / GPAGE_SZ;
  debug("alloc range: Meta[%d] to Meta[%d]\n", id, id + n_pg - 1);
  for (int i = 0; i < n_pg; i++) {
    assert(meta[i].is_alloc == false);
    meta[i].is_alloc = true;
    meta[i].is_slab  = is_slab;
    meta[i].end      = (void *)((uintptr_t)ret + sz);
    // debug("alloc Meta[%d], start: %p, end: %p\n", get_meta_index((void *)(meta[i].start)), meta[i].start, meta[i].end);
  }
  return ret;
}

static void *G_alloc(size_t npage, bool is_slab) {
  spin_lock(&G_lock);
  size_t sz = npage * GPAGE_SZ;
  void *try_ret = (void *)ROUNDUP(G_start, sz);
  for (; is_valid_ret(try_ret); try_ret += sz){
    if (try_alloc(try_ret, sz, is_slab) == true) {
      slow_alloc(try_ret, sz, is_slab);
      spin_unlock(&G_lock);
      assert(ROUNDDOWN(try_ret, sz) == (uintptr_t)try_ret);
      return try_ret;
    }
  }
  spin_unlock(&G_lock);
  return NULL;
}

static void G_free(void *ptr){
  spin_lock(&G_lock);
  assert(ROUNDDOWN(ptr, GPAGE_SZ) == (uintptr_t)ptr);
  int id = get_meta_index(ptr);
  assert(id >= 0 && id < n_meta);
  meta_t *meta = &Meta[id];
  assert(meta->start == ptr);
  void *free_end = meta->end;
  assert(free_end != NULL);
  int n_pg = (free_end - ptr) / GPAGE_SZ;
  debug("free range: Meta[%d] to Meta[%d]\n", id, id + n_pg - 1);
  for (int i = 0; i < n_pg; ++i){
    // debug("free Meta[%d] , start: %p, end: %p\n", get_meta_index((void *)(meta[i].start)), meta[i].start, meta[i].end);
    assert(meta[i].is_alloc == true);
    assert(meta[i].is_slab  == false);
    assert(meta[i].end      == free_end);
    meta[i].is_alloc = false;
    meta[i].end      = NULL;
  }
  spin_unlock(&G_lock);
}

static void *kalloc(size_t size) {
  //brute force

  size_t npage = size / GPAGE_SZ;
  if (npage == 0) npage = 1;
  return G_alloc(npage, false);

  // normal klloc
  size = nextPower_2(size);
  if (size < 16) size = 16;
  if (size <= 4096) {
    return S_alloc(size);
  } else {
    size_t npage = size / GPAGE_SZ;
    if (npage == 0) npage = 1;
    return G_alloc(npage, false);
  } 
}

static void kfree(void *ptr) {
  assert(ptr != NULL);
  void *p_rd = (void *)ROUNDDOWN(ptr, GPAGE_SZ);
  int id = get_meta_index(p_rd);
  assert(id >= 0 && id < n_meta);
  meta_t *meta = &Meta[id];
  assert(meta->start == p_rd);
  if (meta->is_slab) {
    // brute force, should be removed
    assert(0);
    S_free(ptr);
  } else {
    assert(p_rd == ptr);
    G_free(ptr);
  }
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
  printf("Got %d MiB heap: [%p, %p)\n", HEAP_SIZE >> 20, heap.start, heap.end);
  G_init();
  S_init();

}
#endif

MODULE_DEF(pmm) = {
  .init  = pmm_init,
  .alloc = kalloc,
  .free  = kfree,
};
