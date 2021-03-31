#include <ucontext.h>
#include <stdint.h>
#include <stdlib.h>
#include "thread.h"
#include "queue.h"
#include <valgrind/valgrind.h>

struct thread {
    int is_main;
    int yielded;
    int joinable;
    void *(*func)(void *);
    void *funcarg;
    void *retval;
    int valgrind_stackid;
    ucontext_t uctx;
    STAILQ_ENTRY(thread) threads;
};

// init FIFO
typedef STAILQ_HEAD(thread_fifo, thread) head_t;
head_t head = STAILQ_HEAD_INITIALIZER(head);

// main context & thread
static ucontext_t main_ctx;
static struct thread main_thread;
static int initialized = 0;

/*
 * strategy:
 * put head of FIFO (current thread) at tail
 */
void scheduler(void) {
    // get the thread from head of FIFO
    struct thread *curr_th = STAILQ_FIRST(&head);

    // if thread finished, remove it from FIFO and set contex of next thread
    if (!curr_th->yielded) {
        curr_th->joinable = 1;
        STAILQ_REMOVE_HEAD(&head, threads);
        if (!STAILQ_EMPTY(&head)) {
            struct thread *next_th = STAILQ_FIRST(&head);
            // set context for next_th
            setcontext(&next_th->uctx);
        }
    } else {
        STAILQ_REMOVE_HEAD(&head, threads);

        // reset yielded property of the thread
        curr_th->yielded = 0;

        if (STAILQ_EMPTY(&head)) {// only main thread is yielding to self
            STAILQ_INSERT_HEAD(&head, curr_th, threads);

            // set context for main again
            setcontext(&curr_th->uctx);
        } else {
            struct thread *next_th = STAILQ_FIRST(&head);

            // put curr_th at the end of FIFO
            STAILQ_INSERT_TAIL(&head, curr_th, threads);

            // swap to next_th context
            setcontext(&next_th->uctx);
        }
    }
}

static int init_main(void) {
    initialized = 1;
    getcontext(&main_ctx);
    main_ctx.uc_link = NULL;
    main_ctx.uc_stack.ss_size = 64*1024;
    main_ctx.uc_stack.ss_sp = malloc(main_ctx.uc_stack.ss_size);

    main_thread.is_main = 1;
    main_thread.uctx = main_ctx;
    STAILQ_INSERT_HEAD(&head, &main_thread, threads);
    makecontext(&main_ctx, scheduler, 0);
    return 0;
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
    // get the thread from FIFO
    struct thread *curr_th = STAILQ_FIRST(&head);

    // set retval in the thread structure
    curr_th->retval = retval;

    //swap to scheduler context
    makecontext(&main_ctx, scheduler, 0);
    setcontext(&main_ctx);
}

void thread_runner(void) {
    struct thread *curr_th = STAILQ_FIRST(&head);
    thread_exit(curr_th->func((void *)curr_th->funcarg));
}

/* recuperer l'identifiant du thread courant.
 */
thread_t thread_self(void) {
    if (!initialized)
        init_main();
    struct thread *curr_thread = STAILQ_FIRST(&head);
    return (thread_t)curr_thread;
}

/* creer un nouveau thread qui va exécuter la fonction func avec l'argument funcarg.
 * renvoie 0 en cas de succès, -1 en cas d'erreur.
 */
int thread_create(thread_t *newthread, void *(*func)(void *), void *funcarg) {
    // allocate a node in the global FIFO for the new thread
    struct thread *thn = malloc(sizeof(struct thread));
    thn->is_main = 0;
    thn->yielded = 0;
    thn->joinable = 0;
    thn->func = func;
    thn->funcarg = funcarg;
    *newthread = (thread_t)thn;

    // set up the context of the new thread
    getcontext(&thn->uctx);
    thn->uctx.uc_stack.ss_size = 64*1024;
    thn->uctx.uc_stack.ss_sp = malloc(thn->uctx.uc_stack.ss_size);

    /* for valgrind */
    thn->valgrind_stackid = VALGRIND_STACK_REGISTER(thn->uctx.uc_stack.ss_sp,
                                                   thn->uctx.uc_stack.ss_sp + thn->uctx.uc_stack.ss_size);

    // uc_link always the main ctx
    thn->uctx.uc_link = &main_ctx;

    // set up the thread runner as entry point
    makecontext(&thn->uctx, thread_runner, 0);

    // add new thread to FIFO
    STAILQ_INSERT_TAIL(&head, thn, threads);

    return 0;
}

/* passer la main à un autre thread.
 */
int thread_yield(void) {
    if (!initialized)
        init_main();

    // get the thread who yielded from head of FIFO
    struct thread *curr_th = STAILQ_FIRST(&head);

    // set yielded to true for scheduler
    curr_th->yielded = 1;

    // swap to scheduler context
    swapcontext(&curr_th->uctx, &main_ctx);
    return 0;
}


/* attendre la fin d'exécution d'un thread.
 * la valeur renvoyée par le thread est placée dans *retval.
 * si retval est NULL, la valeur de retour est ignorée.
 */
int thread_join(thread_t thread, void **retval) {
    struct thread *th = (struct thread *)thread;

    while (!th->joinable) // wait for thread to finish
        thread_yield();

    // get the return val set by thread_exit
    if (retval)
        *retval = th->retval;

    // free sigaltstack then node
    VALGRIND_STACK_DEREGISTER(th->valgrind_stackid);
    free(th->uctx.uc_stack.ss_sp);
    if (!th->is_main)
        free(th);

    return 0;
}

int thread_mutex_init(thread_mutex_t *mutex);
int thread_mutex_destroy(thread_mutex_t *mutex);
int thread_mutex_lock(thread_mutex_t *mutex);
int thread_mutex_unlock(thread_mutex_t *mutex);