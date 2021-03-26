#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>
#include <stdint.h>
#include "thread.h"

/* test de plein de switch par plein de threads
 *
 * la durée du programme doit etre proportionnelle au nombre de threads et de yields donnés en argument
 *
 * support nécessaire:
 * - thread_create()
 * - thread_yield() depuis ou vers le main
 * - retour sans thread_exit()
 * - thread_join() avec récupération de la valeur de retour
 */

static void * thfunc(void *_nbyield)
{
  int nbyield = (intptr_t) _nbyield;
  int i;

  for(i=0; i<nbyield; i++)
    thread_yield();
  return NULL;
}

int main(int argc, char *argv[])
{
  int nbth, i, err;
  int nbyield;
  thread_t *ths;
  struct timeval tv1, tv2;
  unsigned long us;

  if (argc < 3) {
    printf("arguments manquants: nombre de threads, puis nombre de yield\n");
    return -1;
  }

  nbth = atoi(argv[1]);
  nbyield = atoi(argv[2]);

  ths = malloc(nbth * sizeof(thread_t));
  assert(ths);

  gettimeofday(&tv1, NULL);

  for(i=0; i<nbth; i++) {
    err = thread_create(&ths[i], thfunc, (void*) (intptr_t) nbyield);
    assert(!err);
  }

  for(i=0; i<nbyield; i++)
    thread_yield();

  for(i=0; i<nbth; i++) {
    void *res;
    err = thread_join(ths[i], &res);
    assert(!err);
    assert(res == NULL);
  }

  gettimeofday(&tv2, NULL);
  us = (tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec);
  printf("%d yield avec %d threads: %ld us\n",
	 nbyield, nbth, us);

  free(ths);

  return 0;
}
