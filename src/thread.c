#include <ucontext.h>
#include <stdlib.h>
#include <stdio.h>
#include "thread.h"
#include "queue.h"
#include <valgrind/valgrind.h>

#define STACK_SIZE 64*1024
#define JOINABLE (1U << 0)
#define MAIN (1U << 1)

typedef enum {
  LOW,
  NORMAL,
  HIGH
} priority;

struct thread {
    void *(*func)(void *);
    void *funcarg;
    void *retval;
    unsigned int flags;
    int valgrind_stackid;
    ucontext_t uctx;
    priority p;
    struct thread *master;
    TAILQ_ENTRY(thread) threads;
};

// init FIFO for normal priority runnable threads
typedef TAILQ_HEAD(runnable_fifo, thread) runnable_hd_t;
runnable_hd_t runnable_hd = TAILQ_HEAD_INITIALIZER(runnable_hd);

// init FIFO for high priority runnable threads
typedef TAILQ_HEAD(high_prio_fifo, thread) high_prio_hd_t;
high_prio_hd_t high_prio_hd = TAILQ_HEAD_INITIALIZER(high_prio_hd);

// init FIFO for abandoned threads; threads who exited but never got joined
typedef TAILQ_HEAD(abandoned_fifo, thread) abandoned_hd_t;
abandoned_hd_t abandoned_hd = TAILQ_HEAD_INITIALIZER(abandoned_hd);

// main thread
struct thread main_th;
// current thread
struct thread *current_th;

/**
 * frees the memory allocated to the abandoned threads
 * when main() returns/exits.
 */
__attribute__ ((destructor)) void free_thread(void) {
    // free the abandoned threads
    while (!TAILQ_EMPTY(&abandoned_hd)) {
        struct thread *th = TAILQ_FIRST(&abandoned_hd);
        TAILQ_REMOVE(&abandoned_hd, th, threads);
        VALGRIND_STACK_DEREGISTER(th->valgrind_stackid);
        free(th->uctx.uc_stack.ss_sp);
        free(th);
    }
}

/**
 * initializes the main thread (and context) and adds it to runnable FIFO
 */
__attribute__((constructor)) void init_thread(void) {
    // set the flag and priority of main context
    main_th.flags |= MAIN;
    main_th.p = NORMAL;

    // init the context for the main thread
    getcontext(&main_th.uctx);

    // add main thread to runnable fifo
    current_th = &main_th;
}

/* terminer le thread courant en renvoyant la valeur de retour retval.
 * cette fonction ne retourne jamais.
 *
 * L'attribut noreturn aide le compilateur à optimiser le code de
 * l'application (élimination de code mort). Attention à ne pas mettre
 * cet attribut dans votre interface tant que votre thread_exit()
 * n'est pas correctement implémenté (il ne doit jamais retourner).
 */
void thread_exit(void *retval) {
    // get the thread at the head of runnable FIFO
    struct thread *curr_th = current_th;

    // add it to abandoned FIFO
    if (!(curr_th->flags & MAIN)) {
        TAILQ_INSERT_TAIL(&abandoned_hd, curr_th, threads);
    }

    // change current exiting thread master priority back to high
    if (curr_th->master) {
        curr_th->master->p = HIGH;
        TAILQ_INSERT_HEAD(&high_prio_hd, curr_th->master, threads);
    }

    // if this is not the last thread in runnable FIFO
    if (!TAILQ_EMPTY(&high_prio_hd)) {
        // set retval in the thread structure
        curr_th->retval = retval;

        // make the thread joinable
        curr_th->flags |= JOINABLE;

        // resume context of the next thread in runnable FIFO
        struct thread *old_th = current_th;
        current_th = TAILQ_FIRST(&high_prio_hd);
        
        // remove it from runnable FIFO
        TAILQ_REMOVE(&high_prio_hd, current_th, threads);

        swapcontext(&old_th->uctx, &current_th->uctx);

    } else if (!TAILQ_EMPTY(&runnable_hd)) {
        // set retval in the thread structure
        curr_th->retval = retval;

        // make the thread joinable
        curr_th->flags |= JOINABLE;

        // resume context of the next thread in runnable FIFO
        struct thread *old_th = current_th;
        current_th = TAILQ_FIRST(&runnable_hd);
        
        // remove it from runnable FIFO
        TAILQ_REMOVE(&runnable_hd, current_th, threads);

        swapcontext(&old_th->uctx, &current_th->uctx);

    } else if (!(curr_th->flags & MAIN)) { // the last thread isnt the main thread
        // restore context of main thread to clean up with destructor
        current_th = &main_th;
        setcontext(&main_th.uctx);
    }

    exit(EXIT_SUCCESS);
}

void thread_runner(void) {
    // get the thread at the head of runnable FIFO
    struct thread *curr_th = current_th;

    // call the entry function of the thread and pass the return value to thread_exit
    thread_exit(curr_th->func(curr_th->funcarg));
}

/* recuperer l'identifiant du thread courant.
 */
thread_t thread_self(void) {
    // get the calling thread from runnable fifo
    return (thread_t)current_th;
}

/* creer un nouveau thread qui va exécuter la fonction func avec l'argument funcarg.
 * renvoie 0 en cas de succès, -1 en cas d'erreur.
 */
int thread_create(thread_t *newthread, void *(*func)(void *), void *funcarg) {
    // allocate a struct thread instance for the new thread
    struct thread *thn = malloc(sizeof(struct thread));
    thn->flags = 0U;
    thn->func = func;
    thn->funcarg = funcarg;
    thn->retval = NULL;
    thn->p = NORMAL;
    thn->master = NULL;

    // set the thread id as the pointer to its struct thread instance
    *newthread = (thread_t)thn;

    // set up the context of the new thread
    getcontext(&thn->uctx);
    thn->uctx.uc_link = NULL;
    thn->uctx.uc_stack.ss_size = STACK_SIZE;
    thn->uctx.uc_stack.ss_sp = malloc(thn->uctx.uc_stack.ss_size);
    thn->valgrind_stackid = VALGRIND_STACK_REGISTER(thn->uctx.uc_stack.ss_sp,
                                                   thn->uctx.uc_stack.ss_sp + thn->uctx.uc_stack.ss_size);
    // set the thread runner as entry point
    makecontext(&thn->uctx, thread_runner, 0);

    // add new thread to runnable FIFO
    TAILQ_INSERT_TAIL(&runnable_hd, thn, threads);

    return 0;
}

/* passer la main à un autre thread.
 */
int thread_yield(void) {
    // get backup value of old thread
    struct thread *old_th = current_th;

    // insert old thread at tail
    switch (old_th->p) {
        case NORMAL:
            TAILQ_INSERT_TAIL(&runnable_hd, old_th, threads);
            break;
        case HIGH:
            TAILQ_INSERT_TAIL(&high_prio_hd, old_th, threads);
            break;
        case LOW:
            break;
    }

    // get the next thread in runnable FIFO and remove it from runnable FIFO
    if (!TAILQ_EMPTY(&high_prio_hd)) {
        current_th = TAILQ_FIRST(&high_prio_hd);
        TAILQ_REMOVE(&high_prio_hd, current_th, threads);
    } else {
        current_th = TAILQ_FIRST(&runnable_hd);
        TAILQ_REMOVE(&runnable_hd, current_th, threads);
    }
   
    // swap to the context of next thread
    swapcontext(&old_th->uctx, &current_th->uctx);

    return 0;
}

/* Cette fonction permet de passer la main au thread t*/
int thread_yield_to(struct thread * t) {
    // get backup value of old thread
    struct thread *old_th = current_th;

    // insert old thread at tail
    switch (old_th->p) {
        case NORMAL:
            TAILQ_INSERT_TAIL(&runnable_hd, old_th, threads);
            break;
        case HIGH:
            TAILQ_INSERT_TAIL(&high_prio_hd, old_th, threads);
            break;
        case LOW:
            break;
    }

    // remove the thread t from the list
    switch (t->p) {
        case NORMAL:
            TAILQ_REMOVE(&runnable_hd, t, threads);
            break;
        case HIGH:
            TAILQ_REMOVE(&high_prio_hd, t, threads);
            break;
        case LOW:
            break;
    }

    // get the next thread in runnable FIFO
    current_th = t;

    // swap to the context of next thread
    swapcontext(&old_th->uctx, &current_th->uctx);

    return 0;
}

/* attendre la fin d'exécution d'un thread.
 * la valeur renvoyée par le thread est placée dans *retval.
 * si retval est NULL, la valeur de retour est ignorée.
 */
int thread_join(thread_t thread, void **retval) {
    // cast the thread ID back to (struct thread *)
    struct thread *th = (struct thread *)thread;

    // loop until thread becomes JOINABLE
    if (!(th->flags & JOINABLE)) {
        th->master = current_th;
        current_th->p = LOW;
        if(th->p == NORMAL) {
            TAILQ_REMOVE(&runnable_hd, th, threads);
            TAILQ_INSERT_TAIL(&high_prio_hd, th, threads);
            th->p = HIGH;
        }
        thread_yield();
    }

    // remove the thread from abandonned FIFO
    if (!(th->flags & MAIN)) {
        TAILQ_REMOVE(&abandoned_hd, th, threads);
    }

    // if retval is not NULL, get the return val set by thread_exit
    if (retval)
        *retval = th->retval;

    // free memory allocated to the thread if not main thread
    if (!(th->flags & MAIN)) {
        VALGRIND_STACK_DEREGISTER(th->valgrind_stackid);
        free(th->uctx.uc_stack.ss_sp);
        free(th);
    }

    return 0;
}

/*      Implémentation des mutexes      */


int thread_mutex_init(thread_mutex_t *mutex) {
    if (mutex != NULL) {
        mutex->is_destroyed = 0;
        mutex->locker = NULL;
        return EXIT_SUCCESS;
    }

    return EXIT_FAILURE;
}

int thread_mutex_destroy(thread_mutex_t *mutex) {
    if (mutex != NULL) {
        mutex->is_destroyed = 1;
        mutex->locker = NULL;
        return EXIT_SUCCESS;
    }

    return EXIT_FAILURE;
}

int thread_mutex_lock(thread_mutex_t *mutex) {
    struct thread *curr_th = current_th;

    //  we don't block if mutex not initialized/destroyed or owned by calling thread
    if (mutex == NULL || mutex->is_destroyed != 0 || mutex->locker == (thread_t)curr_th) {
        return EXIT_FAILURE;
    }

    // otherwise we block until mutex is available
    while (mutex->locker != NULL) {
        // eventually we could yield to locker thread directly 
        thread_yield_to(mutex->locker);
    }

    // lock the mutex
    mutex->locker = (thread_t)curr_th;

    return EXIT_SUCCESS;
}

int thread_mutex_unlock(thread_mutex_t *mutex) {
    struct thread *curr_th = current_th;

    // unlocking a mutex not owned by calling thread is an error
    if (mutex == NULL || mutex->is_destroyed != 0 || mutex->locker != (thread_t)curr_th) {
        return EXIT_FAILURE;
    }

    // release mutex
    mutex->locker = NULL;

    return EXIT_SUCCESS;
}