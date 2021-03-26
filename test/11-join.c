#include <stdio.h>
#include <assert.h>
#include "thread.h"

/* test du join, avec ou sans thread_exit.
 *
 * le programme doit retourner correctement.
 * valgrind doit être content.
 *
 * support nécessaire:
 * - thread_create()
 * - thread_exit()
 * - retour sans thread_exit()
 * - thread_join() avec récupération valeur de retour, avec et sans thread_exit()
 */

static void * thfunc(void *dummy __attribute__((unused)))
{
  thread_exit((void*)0xdeadbeef);
  return NULL; /* unreachable, shut up the compiler */
}

static void * thfunc2(void *dummy __attribute__((unused)))
{
  return (void*) 0xbeefdead;
}

int main()
{
  thread_t th, th2;
  int err;
  void *res = NULL;

  err = thread_create(&th, thfunc, NULL);
  assert(!err);
  err = thread_create(&th2, thfunc2, NULL);
  assert(!err);

  err = thread_join(th, &res);
  assert(!err);
  assert(res == (void*) 0xdeadbeef);

  err = thread_join(th2, &res);
  assert(!err);
  assert(res == (void*) 0xbeefdead);

  printf("join OK\n");
  return 0;
}
