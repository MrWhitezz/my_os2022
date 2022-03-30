#include <common.h>
#include <pmm.h>

// Spin lock
typedef int spinlock_t;
#define SPIN_INIT() 0

void spin_lock(spinlock_t *lk) { while (atomic_xchg(lk, 1))  ; }
void spin_unlock(spinlock_t *lk) { atomic_xchg(lk, 0); }

enum slab_index{ _16B, _32B, _64B, _128B, _256B, _512B, _1KB, _2KB, _4KB, SLAB_SIZE };

G_header_t *G_head = NULL;
S_header_t *Slab[8][SLAB_SIZE];
int n_slab = 0;

spinlock_t G_lock;
spinlock_t S_lock[8][SLAB_SIZE];

unsigned int nextPower_2(unsigned int x){
  x = x - 1;
  x = x | (x >> 1);
  x = x | (x >> 2);
  x = x | (x >> 4);
  x = x | (x >> 8);
  x = x | (x >> 16);
  return x + 1;
}

static spinlock_t *get_slab_lk(S_header_t *s_head){
  for (int i = 0; i < 8; i++){
    for (int j = 0; j < SLAB_SIZE; j++){
      if (Slab[i][j] == s_head){
        return &S_lock[i][j];
      }
    }
  }
  assert(0);
  return NULL;
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
  G_lock = SPIN_INIT();
  G_head = (G_header_t *)(ROUNDUP((uintptr_t)heap.start, GPAGE_SZ));
  G_head->next = NULL;
  G_head->size = (size_t)(heap.end - (ROUNDUP((uintptr_t)heap.start, GPAGE_SZ)));
}

static void S_init() {
  n_slab = cpu_count();
  for (int i = 0; i < n_slab; i++) {
    for (int j = 0; j < SLAB_SIZE; j++) {
      S_lock[i][j] = SPIN_INIT();
      Slab[i][j] = (S_header_t *)G_alloc(1);
      Slab[i][j]->unit_size = (1 << (j + 4));
      // printf("Slab[%d][%d]->unit_size = %ld\n", i, j, Slab[i][j]->unit_size);
      Slab[i][j]->n_head = (S_node_t *)ROUNDUP(((uintptr_t)Slab[i][j] + sizeof(S_header_t)), Slab[i][j]->unit_size);
      Slab[i][j]->S_magic = SMAGIC;
      Slab[i][j]->n_head->next = NULL;
      Slab[i][j]->n_head->size = (GPAGE_SZ - ((uintptr_t)Slab[i][j]->n_head - (uintptr_t)Slab[i][j]));
    }
  }
}

static void *S_alloc(size_t size){
  int cpu = cpu_current();
  assert(cpu < cpu_count());
  assert(size = nextPower_2(size));
  int id = get_slab_index(size);
  assert(Slab[cpu][id]->S_magic == SMAGIC);
  assert(size == Slab[cpu][id]->unit_size);
  spin_lock(&S_lock[cpu][id]);
  S_node_t *node = Slab[cpu][id]->n_head;
  S_node_t *prev = NULL;
  while (node != NULL) {
    if (node->size >= size) {
      if (node->size == size) {
        if (prev == NULL) { Slab[cpu][id]->n_head = node->next; }
        else { assert(0); prev->next = node->next; }
      } else {
        S_node_t *new_node = (S_node_t *)((uintptr_t)node + size);
        new_node->size = node->size - size;
        new_node->next = node->next;
        if (prev == NULL) { Slab[cpu][id]->n_head = new_node; }
        else { prev->next = new_node; }
      }
      assert((ROUNDDOWN((uintptr_t)node, size)) == (uintptr_t)node);
      break;
    }
    // should seldom reach
    prev = node;
    node = node->next;
  }
  spin_unlock(&S_lock[cpu][id]);
  return (void *)node;
}

static void S_free(void *ptr){
  S_header_t *S_head = (S_header_t *)(ROUNDDOWN((uintptr_t)ptr, GPAGE_SZ));
  spinlock_t *lk = get_slab_lk(S_head);
  spin_lock(lk);
  assert(S_head->S_magic == SMAGIC);
  S_node_t *node = (S_node_t *)ptr;
  assert(sizeof(S_node_t) <= 16);
  node->size = S_head->unit_size;
  S_node_t *node_head = S_head->n_head;
  S_node_t *prev = NULL;
  while (node_head != NULL) {
    if (addr_leq(node, node_head)) {
      node->next = node_head;
      if (prev == NULL) { S_head->n_head = node; }
      else { prev->next = node; }
      break;
    }
    prev = node_head;
    node_head = node_head->next;
  }
  if (node_head == NULL) { node->next = NULL; prev->next = node; }
  spin_unlock(lk);
}

static void *G_alloc(size_t npage){
  // alloc npage * GPAGE_SZ, alligned, neglect info_t
  size_t sz = npage * GPAGE_SZ;
  spin_lock(&G_lock);
  G_header_t *p = G_head;
  G_header_t *prev = NULL;
  while (p != NULL){
    size_t avail_sz = ((uintptr_t)p + p->size) - (ROUNDUP((uintptr_t)p, sz));
    if (avail_sz >= sz){
      if (p->size == sz){
        assert(p->size == avail_sz);
        if (prev == NULL){ G_head = p->next; }
        else{ prev->next = p->next; }
      }else{
        if ((uintptr_t)p == (ROUNDUP((uintptr_t)p, sz))){
          G_header_t *p_new = (G_header_t *)((uintptr_t)p + sz);
          p_new->next = p->next;
          p_new->size = p->size - sz;
          if (prev == NULL){ G_head = p_new; }
          else{ prev->next = p_new; }
        }
        else {
          if (avail_sz == sz){
            p->size = p->size - avail_sz;
            // for return
          }
          else {
            G_header_t *p_new = (G_header_t *)(ROUNDUP((uintptr_t)p, sz) + sz);
            p_new->next = p->next;
            p_new->size = avail_sz - sz;
            p->next = p_new;
          }
          p = (G_header_t *)(ROUNDUP(p, sz));
        }
      }
      assert((ROUNDDOWN((uintptr_t)p, sz)) == (uintptr_t)p);
      break;
    }
    prev = p;
    p = p->next;
  }
  spin_unlock(&G_lock);
  return (void *)(p);
}

static void G_free(void *ptr){
  spin_lock(&G_lock);
  assert((ROUNDDOWN((uintptr_t)ptr, GPAGE_SZ)) == (uintptr_t)ptr);
  info_t *info = (info_t *)((uintptr_t)ptr - GPAGE_SZ);
  assert(info->G_magic == GMAGIC);
  G_header_t *p = (G_header_t *)info;
  p->size = info->size;
  G_header_t *p_head = G_head;
  G_header_t *prev = NULL;
  while (p_head != NULL){
    if (addr_leq(p, p_head)){
      p->next = p_head;
      if (prev == NULL){ G_head = p; }
      else{ prev->next = p; }
      break;
    }
    prev = p_head;
    p_head = p_head->next;
  }
  if (p_head == NULL){ assert(prev != NULL); p->next = NULL; prev->next = p; }
  spin_unlock(&G_lock);
}

static void *kalloc(size_t size) {
  if (size > MAX_ALLOC) return NULL;
  size = nextPower_2(size);
  if (size <= 4 * 1024) {
    if (size < 16) size = 16;
    return S_alloc(size);
  } else {
    size_t npage = size / GPAGE_SZ;
    if (npage == 0) npage = 1; 
    assert(npage > 0);
    void *ptr = G_alloc(npage + 1);
    if (ptr != NULL) {
      info_t *info  = (info_t *)ptr;
      info->G_magic = GMAGIC;
      info->size    = (npage + 1) * GPAGE_SZ;
      return (void *)((uintptr_t)ptr + GPAGE_SZ);
    }
  }
  return NULL;
}

static void kfree(void *ptr) {
  S_header_t *S_head = (S_header_t *)(ROUNDDOWN((uintptr_t)ptr, GPAGE_SZ));
  if (S_head->S_magic == SMAGIC) {
    S_free(ptr);
  } else {
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
