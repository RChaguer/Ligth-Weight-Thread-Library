#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>
#include <stdint.h>
#include "thread.h"

/* test une chaine de thread avec un nombre decroissants de switch quand on descend dans la chaine.
 *
 * La durée du programme doit etre proportionnelle au nombre de total de yields = ( nbyield * nbthread * (nbthread+1) / 2 ) donnés en argument
 *
 * support nécessaire:
 * - thread_create()
 * - thread_yield() depuis ou vers le main
 * - retour sans thread_exit()
 * - thread_join() avec récupération de la valeur de retour
 */

static int nbyield;
static int nbthread;

static void * thfunc(void *_nbth)
{
    int nbth = (intptr_t) _nbth;
    int i;

    if ((unsigned long) nbth > 0) {
        thread_t th;
        int err;
        void *res;
        err = thread_create(&th, thfunc, _nbth-1);
        assert(!err);

        for(i=0; i<(nbyield*nbth); i++) {
            thread_yield();
        }

        err = thread_join(th, &res);
        assert(!err);
        assert(res == _nbth-1);
    }
    else {
        for(i=0; i<(nbyield*nbthread); i++) {
            thread_yield();
        }
    }
    return _nbth;
}

int main(int argc, char *argv[])
{
    struct timeval tv1, tv2;
    unsigned long us;
    unsigned long nbth;

    if (argc < 3) {
        printf("arguments manquants: nombre de threads, puis nombre de yield\n");
        return -1;
    }

    nbthread = atoi(argv[1]);
    nbth = nbthread;
    nbyield = atoi(argv[2]);

    gettimeofday(&tv1, NULL);
    thfunc((void*) nbth);
    gettimeofday(&tv2, NULL);

    us = (tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec);
    printf("%d yield avec plein de threads dans join: %ld us\n", nbyield, us);

    printf("%ld threads créés et détruits\n", nbth);
    return 0;
}
