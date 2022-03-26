#include <pmm.h>
#include <common.h>
#define addr_leq(a, b) ((uintptr_t)(a) <= (uintptr_t)(b))

#ifdef TEST
extern  Area   heap;
#endif

// Spin lock
void spin_lock(spinlock_t *lk) { while (atomic_xchg(lk, 1))  ; }
void spin_unlock(spinlock_t *lk) { atomic_xchg(lk, 0); }

unsigned int nextPower_2(unsigned int x){
  x = x - 1;
  x = x | (x >> 1);
  x = x | (x >> 2);
  x = x | (x >> 4);
  x = x | (x >> 8);
  x = x | (x >> 16);
  return x + 1;
}

spinlock_t lk[LK_SIZE];
__node_t * head;
uint32_t list_size;


void list_init(){
  list_size = ((uintptr_t)heap.end - (uintptr_t)heap.start) / 2;
  list_size = ROUNDDOWN(list_size, 16 * (1 << 20));
  assert(ROUNDUP(list_size, 16 * (1 << 20)) == list_size);
  head = (__node_t*)heap.start;
  head->size = list_size;
  head->next = NULL;
}

void  fill_header(header_t *header, void *start, int size){
  header->start = start;
  header->size = size;
}

void  drag_node(__node_t *from, __node_t *to){
  // this function only drags relative position in list from one node to another
  __node_t ** prev = &head;
  __node_t * curr = head;
  if (curr == from){
    head = to;
    head->next = from->next;
    return; // interesting bug
  }
  else{
    curr = curr->next;
  }
  while(curr != NULL && curr != from){
    prev = &((*prev)->next);
    assert(*prev == curr);
    curr = curr->next;
  }
  assert(curr == from && (*prev)->next == from);

  (*prev)->next = to;
  to->next = from->next;
}

void* list_alloc(size_t size){
  assert(size >= sizeof(struct header_t));
  // add lock
  spin_lock(&lk[LK_ALLOC]);
  __node_t * curr = head;
  void * ret = NULL;
  // always give a larger size
  size = nextPower_2(size);
  // Log("Enter LOOP\n");

  while(1){
    // TODO
    if (curr == NULL){
      break;
    }
    uint32_t rd_sz = ((ROUNDUP(curr, size) - (uintptr_t)curr));
    uint32_t free_sz = curr->size - rd_sz; 
    if (free_sz >= size){
      ret = (void *)ROUNDUP(curr, size);
      fill_header((header_t *)ret, curr, size + rd_sz);
      __node_t * new_curr = (__node_t *)((uintptr_t)ret + size);
      assert(((size_t)new_curr - (size_t)curr) == size + rd_sz);
      drag_node(curr, new_curr);
      curr = new_curr;
      curr->size = free_sz - size;
      break;
    }
    curr = curr->next;
  }
  // Log("FINISH LOOP\n");
  // release lock
  spin_unlock(&lk[LK_ALLOC]);
  if (ret == NULL){ return NULL; }
  return ret + list_size;
}

void  list_insert(__node_t *node){
  __node_t **prev = &head;
  __node_t *curr = head;
  if (addr_leq(node, curr)){
    head = node;
    head->next = curr;
    return;
  }
  else {
    curr = curr->next;
  }

  while(curr != NULL && !addr_leq(node, curr)){
    prev = &((*prev)->next);
    assert(*prev == curr);
    curr = curr->next;
  }

  if (curr == NULL){
    (*prev)->next = node;
    node->next = NULL;
  }
  else{
    assert(addr_leq(node, curr));
    assert(addr_leq((*prev)->next, node));
    (*prev)->next = node;
    node->next = curr;
  }
}

void  list_free(void *ptr){
  void *lptr = ptr - list_size;
  header_t *h = (header_t *)lptr;
  __node_t *cur = (__node_t *)h->start;
  cur->size = h->size;
  spin_lock(&lk[LK_ALLOC]); // name needs to be modify
  list_insert(cur);
  spin_unlock(&lk[LK_ALLOC]);
}
