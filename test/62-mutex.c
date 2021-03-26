#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "thread.h"

/* test de faire une somme avec plein de thread sur un compteur partagé
 *
 * valgrind doit etre content.
 * Les résultats doivent etre égals au nombre de threads * 1000.
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

#define NB_MUTEX 10

int counter[NB_MUTEX] = { 0 };
thread_mutex_t lock[NB_MUTEX];

static void * thfunc(void *_nb)
{
    unsigned long nb = (unsigned long) _nb;
    unsigned long i = 0;
    int tmp;

    int m = nb % NB_MUTEX;

    for(i=0; i<1000;i++) {
        /* Verrouille la section critique accédant a counter */
        thread_mutex_lock(&lock[m]);
        tmp = counter[m];
        thread_yield();
        tmp++;
        thread_yield();
        counter[m] = tmp;
        thread_mutex_unlock(&lock[m]);
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    thread_t *th;
    int i, nbthrd;
    int err, nb;

    if (argc < 2) {
        printf("argument manquant: nombre de threads\n");
        return -1;
    }

    nb = atoi(argv[1]);
    nbthrd = nb * NB_MUTEX;

    for(i=0; i<NB_MUTEX; i++) {
        if (thread_mutex_init(&lock[i]) != 0) {
            fprintf(stderr, "thread_mutex_init(lock[%d]) failed\n", i);
            return -1;
        }
    }

    th = malloc(nbthrd*sizeof(*th));
    if (!th) {
        perror("malloc");
        return -1;
    }

    /* on cree tous les threads */
    for(i=0; i<nbthrd; i++) {
        err = thread_create(&th[i], thfunc, (void*)((intptr_t)i));
        assert(!err);
    }

    /* on leur passe la main, ils vont tous terminer */
    for(i=0; i<nb; i++) {
        thread_yield();
    }

    /* on les joine tous, maintenant qu'ils sont tous morts */
    for(i=0; i<nbthrd; i++) {
        err = thread_join(th[i], NULL);
        assert(!err);
    }

    free(th);
    for(i=0; i<NB_MUTEX; i++) {
        thread_mutex_destroy(&lock[i]);
    }

    for(i=0; i<NB_MUTEX; i++) {
        if ( counter[i] == ( nb * 1000 ) ) {
            printf("La somme %d a été correctement calculée: %d * 1000 = %d\n", i, nb, counter[i]);
        }
        else {
            printf("Le résultat %d est INCORRECT: %d * 1000 != %d\n", i, nb, counter[i]);
        }
    }
    return 0;
}
