#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "thread.h"

/* test de faire une somme avec plein de thread sur un compteur partagé
 *
 * valgrind doit etre content.
 * Le résultat doit etre égal au nombre de threads * 1000.
 * La durée du programme doit etre proportionnelle au nombre de threads donnés en argument.
 *
 * support nécessaire:
 * - thread_create()
 * - thread_exit()
 * - thread_join() sans récupération de la valeur de retour
 * - thread_mutex_init()
 * - thread_mutex_destroy()
 * - thread_mutex_lock()
 * - thread_mutex_unloc()
 */

int counter = 0;
thread_mutex_t lock;

static void * thfunc(void *dummy __attribute__((unused)))
{
    unsigned long i = 0;
    int tmp;

    for(i=0; i<1000;i++) {
	/* Verrouille la section critique accédant a counter */
	thread_mutex_lock(&lock);
	tmp = counter;
	thread_yield();
	tmp++;
	thread_yield();
	counter = tmp;
	thread_mutex_unlock(&lock);
    }

    return NULL;
}

int main(int argc, char *argv[])
{
  thread_t *th;
  int err, i, nb;

  if (argc < 2) {
    printf("argument manquant: nombre de threads\n");
    return -1;
  }

  nb = atoi(argv[1]);

  if (thread_mutex_init(&lock) != 0) {
      fprintf(stderr, "thread_mutex_init failed\n");
      return -1;
  }

  th = malloc(nb*sizeof(*th));
  if (!th) {
    perror("malloc");
    return -1;
  }

  /* on cree tous les threads */
  for(i=0; i<nb; i++) {
    err = thread_create(&th[i], thfunc, NULL);
    assert(!err);
  }

  /* on leur passe la main, ils vont tous terminer */
  for(i=0; i<nb; i++) {
    thread_yield();
  }

  /* on les joine tous, maintenant qu'ils sont tous morts */
  for(i=0; i<nb; i++) {
    err = thread_join(th[i], NULL);
    assert(!err);
  }

  free(th);
  thread_mutex_destroy(&lock);

  if ( counter == ( nb * 1000 ) ) {
      printf("La somme a été correctement calculée: %d * 1000 = %d\n", nb, counter);
      return EXIT_SUCCESS;
  }
  else {
      printf("Le résultat est INCORRECT: %d * 1000 != %d\n", nb, counter);
      return EXIT_FAILURE;
  }
}
