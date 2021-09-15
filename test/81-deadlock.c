#include <stdio.h>
#include <assert.h>
#include "thread.h"

/* test de detection d'un deadlock lors d'un cycle de thread qui joignent tous le suivant.
 * main(th0) joine th1 qui joine th2 qui joine main.
 * il faut qu'un join renvoie -1 quand il detecte le deadlock, et les autres renvoient 0 normalement.
 */


static thread_t th0, th1, th2;
int totalerr = 0;

static void * thfunc2(void *dummy __attribute__((unused)))
{
  void *res;
  int err = thread_join(th0, &res);
  printf("join th2->th0 = %d\n", err);
  totalerr += err;
  thread_exit(NULL);
}

static void * thfunc1(void *dummy __attribute__((unused)))
{
  void *res;
  int err = thread_create(&th2, thfunc2, NULL);
  assert(!err);
  err = thread_join(th2, &res);
  printf("join th1->th2 = %d\n", err);
  totalerr += err;
  thread_exit(NULL);
}

int main()
{
  void *res;
  int err;

  th0 = thread_self();

  err = thread_create(&th1, thfunc1, NULL);
  assert(!err);
  err = thread_join(th1, &res);
  printf("join th0->th1 = %d\n", err);
  totalerr += err;
  printf("somme des valeurs de retour = %d\n", totalerr);
  assert(totalerr == -1);
  thread_exit(NULL);
  return 0;
}

