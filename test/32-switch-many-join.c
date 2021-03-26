#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>
#include <stdint.h>
#include "thread.h"

/* test de plein de switch pendant que N-1 threads sont bloqués dans join
 *
 * La durée du programme doit etre proportionnelle au nombre de threads et de yields donnés en argument
 *
 * support nécessaire:
 * - thread_create()
 * - thread_yield() depuis ou vers le main
 * - retour sans thread_exit()
 * - thread_join() avec récupération de la valeur de retour
 */

static int nbyield;

static void * thfunc(void *_nbth)
{
  int nbth = (intptr_t) _nbth;
  if ((unsigned long) nbth > 0) {
    thread_t th;
    int err;
    void *res;
    err = thread_create(&th, thfunc, _nbth-1);
    assert(!err);
    err = thread_join(th, &res);
    assert(!err);
    assert(res == _nbth-1);
  } else {
    int i;
    struct timeval tv1, tv2;
    unsigned long us;
    gettimeofday(&tv1, NULL);
    for(i=0; i<nbyield; i++)
      thread_yield();
    gettimeofday(&tv2, NULL);
    us = (tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec);
    printf("%d yield avec plein de threads dans join: %ld us\n",
	   nbyield, us);
  }
  return _nbth;
}

int main(int argc, char *argv[])
{
  unsigned long nbth;

  if (argc < 3) {
    printf("arguments manquants: nombre de threads, puis nombre de yield\n");
    return -1;
  }

  nbth = atoi(argv[1]);
  nbyield = atoi(argv[2]);

  thfunc((void*) nbth);

  printf("%ld threads créés et détruits\n", nbth);
  return 0;
}
