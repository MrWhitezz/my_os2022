#define GPAGE_SZ (64 * 1024)
#define GMAGIC   0xDEADBEEF
#define SMAGIC   0x55555555
#define addr_leq(a, b) ((uintptr_t)(a) <= (uintptr_t)(b))

typedef struct G_header_t {
  struct G_header_t *next;
  size_t size;
} G_header_t;

typedef struct info_t {
  int gabage[128];
  uint32_t G_magic;
  size_t   size;
} info_t;

typedef struct S_node_t {
  struct S_node_t *next;
  size_t size;
} S_node_t;

typedef struct S_header_t {
  struct S_node_t *n_head;
  size_t   unit_size;
  uint32_t S_magic;
} S_header_t;


static void *S_alloc(size_t size);
static void *G_alloc(size_t npage);

