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

size_t nextPower_2(size_t x){
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
      Slab[i][j] = (S_header_t *)G_alloc(1, 0);
      Slab[i][j]->unit_size = (1 << (j + 4));
      Slab[i][j]->n_head = (S_node_t *)ROUNDUP(((uintptr_t)Slab[i][j] + sizeof(S_header_t)), Slab[i][j]->unit_size);
      // debug("n_head: %p with unit_size: %ld\n", Slab[i][j]->n_head, Slab[i][j]->unit_size);
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
  // debug("size: %ld, get_slab_index: %d\n", size, get_slab_index(size));
  assert(Slab[cpu][id]->S_magic == SMAGIC);
  assert(size == Slab[cpu][id]->unit_size);
  // debug("Slab[%d][%d]: %p, size: %ld\n", cpu, id, Slab[cpu][id], Slab[cpu][id]->unit_size);
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
  assert((ROUNDDOWN((uintptr_t)node, size)) == (uintptr_t)node);
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

static void *G_alloc(size_t npage, size_t rd_sz){
  // alloc npage * GPAGE_SZ, alligned, neglect info_t
  assert(no_cycle(G_head));
  size_t sz = npage * GPAGE_SZ;
  debug("G_alloc: before G lock %p\n", &G_lock);
  spin_lock(&G_lock);
  debug("Get G_lock\n");
  G_header_t *p = G_head;
  G_header_t *prev = NULL;
  while (p != NULL){
    if (rd_sz == 0){
      if (p->size >= sz) {
        if (p->size == sz){
          if (prev == NULL){ G_head = p->next; assert_nocycle(G_head);}
          else { prev->next = p->next; assert_nocycle(prev); assert_nocycle(prev->next);}
        } else {
          // if ((uintptr_t)p == (ROUNDUP((uintptr_t)p, sz))){
            G_header_t *p_new = (G_header_t *)((uintptr_t)p + sz);
            p_new->next = p->next; assert_nocycle(p_new);
            p_new->size = p->size - sz;
            if (prev == NULL){ G_head = p_new; assert_nocycle(G_head);}
            else{ prev->next = p_new; assert_nocycle(prev); assert_nocycle(prev->next);}
          }
          break;
        }
      }
    else {
      if (!((npage - 1) * GPAGE_SZ == rd_sz)){
        debug("npage: %ld, rd_sz: %ld\n", npage, rd_sz);
      }
      assert((npage - 1) * GPAGE_SZ == rd_sz);
      
      uintptr_t rd_target = ROUNDUP((uintptr_t)(p) + GPAGE_SZ, rd_sz);
      uintptr_t ret       = rd_target - GPAGE_SZ;
      size_t    avail_sz  = (uintptr_t)(p) + p->size - rd_target;
      size_t    target_sz = (npage - 1) * GPAGE_SZ;
      if (avail_sz > target_sz){
        G_header_t *p_new = (G_header_t *)(rd_target + target_sz);
        p_new->size = avail_sz - target_sz;
        p_new->next = p->next; assert_nocycle(p_new);
        size_t rm_sz = ret - (uintptr_t)(p);
        assert(rm_sz + GPAGE_SZ + target_sz + p_new->size == p->size);
        if (rm_sz == 0){
          if (prev == NULL){ G_head = p_new; assert_nocycle(G_head);}
          else { prev->next = p_new; assert_nocycle(prev); assert_nocycle(prev->next);}
        }
        else {
          assert(rm_sz > 0);
          p->size = rm_sz;
          p->next = p_new;
          assert_nocycle(p_new);
          assert_nocycle(p);
        }
        p = (G_header_t *)(ret);
        assert((ROUNDDOWN(rd_target, rd_sz)) == rd_target);
        break;
      }
    }
    prev = p;
    p = p->next;
  }

  debug("G_alloc: before G unlock %p\n", &G_lock);
  assert(no_cycle(G_head));
  spin_unlock(&G_lock);
  return (void *)(p);
}

static void G_free(void *ptr){
  debug("G_free: before G lock %p\n", &G_lock);
  spin_lock(&G_lock);
  assert((ROUNDDOWN((uintptr_t)ptr, GPAGE_SZ)) == (uintptr_t)ptr);
  info_t *info = (info_t *)((uintptr_t)ptr - GPAGE_SZ);
  assert(info->G_magic == GMAGIC);
  G_header_t *p = (G_header_t *)info;
  p->size = info->size;
  G_header_t *p_head = G_head;
  G_header_t *prev = NULL;
  debug("before loop\n");
  while (p_head != NULL){
    if (prev != NULL) assert(addr_leq(prev, p_head));
    if (addr_leq(p, p_head)){
      assert(p != p_head);
      p->next = p_head;
      if (prev == NULL){ G_head = p; assert_nocycle(G_head);}
      else { prev->next = p; assert_nocycle(prev); assert_nocycle(prev->next);}
      break;
    }
    debug("p: %p, p_head: %p, prev: %p\n", p, p_head, prev);
    prev = p_head;
    p_head = p_head->next;
  }
  debug("after loop\n");
  if (p_head == NULL){ assert(prev != NULL); p->next = NULL; prev->next = p; }
  debug("G_free: before G unlock %p\n", &G_lock);
  spin_unlock(&G_lock);
}

bool no_cycle(G_header_t *p){
  while (p != NULL){
    if (p == p->next) return false;
    p = p->next;
  }
  return true;
}

static void *kalloc(size_t size) {
  // debug("kalloc: %zu\n", size);
  if (size > MAX_ALLOC) return NULL;
  size = nextPower_2(size);
  if (size <= 4 * 1024) {
    if (size < 16) {size = 16;}
    return S_alloc(size);
  } else {
    size_t npage = size / GPAGE_SZ;
    if (npage == 0) {npage = 1; size = GPAGE_SZ;}
    assert(npage > 0);
    void *ptr = G_alloc(npage + 1, size);
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
