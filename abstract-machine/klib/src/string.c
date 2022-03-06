#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  size_t len = 0;
  for (; s[len] != '\0'; ++len) {}
  return len;
}

char *strcpy(char *dst, const char *src) {
  //while(*dst++ = *strsrc++);
  size_t i = 0;
  for (; src[i] != '\0'; ++i)
    dst[i] = src[i];
  dst[i] = '\0'; 
  return dst;
}

char *strncpy(char *dst, const char *src, size_t n) {
  size_t i;
  for (i = 0; i < n && src[i] != '\0'; ++i){
    dst[i] = src[i];
  }
  for (; i < n; ++i)
    dst[i] = '\0';
  return dst;
}

char *strcat(char *dst, const char *src) {
  size_t dest_len = strlen(dst);
  size_t i;
  for (i = 0; src[i] != '\0'; ++i)
    dst[dest_len + i] = src[i];
  dst[dest_len + i] = '\0';
  return dst;
}

int strcmp(const char *s1, const char *s2) {
  while (*s1 && *s1 == *s2){
    ++s1;
    ++s2;
  } 
  return *(unsigned char *)s1 - *(unsigned char *)s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
  panic("Not implemented");
}

void *memset(void *s, int c, size_t n) {
  unsigned char *p = s;
  size_t i;
  for (i = 0; i < n; ++i)
    p[i] = (unsigned char) c;
  return s;
}

void *memmove(void *dst, const void *src, size_t n) {
  panic("Not implemented");
}

void *memcpy(void *out, const void *in, size_t n) {
  char *chout = (char *)out;
  char *chin = (char *)in;
  for (int i = 0; i < n; ++i)
    chout[i] = chin[i];
  return out;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  const unsigned char *p = s1, *q = s2;
  size_t i;
  int res = 0;
  if (s1 == s2)
    return res;
  for (i = 0; i < n; ++i){
    if (p[i] != q[i]){
      res = p[i] < q[i] ? -1 : 1;
      break;
    }
  }
  return res;
}

#endif
