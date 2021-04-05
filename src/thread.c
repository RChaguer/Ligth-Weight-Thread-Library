#include <ucontext.h>
#include <stdlib.h>
#include "thread.h"
#include "queue.h"
#include <valgrind/valgrind.h>

#define STACK_SIZE 64*1024
#define JOINABLE (1U << 0)

struct thread {
    void *(*func)(void *);
    void *funcarg;
    void *retval;
    unsigned int flags;
    int valgrind_stackid;
    ucontext_t uctx;
    STAILQ_ENTRY(thread) threads;
};

// init FIFO for runnable threads
typedef STAILQ_HEAD(runnable_fifo, thread) runnable_hd_t;
runnable_hd_t runnable_hd = STAILQ_HEAD_INITIALIZER(runnable_hd);

static int main_initialized;

/**
 * frees the memory allocated to main thread / context
 * when main() returns/exits with main_thread still in FIFO;
 * meaning no other thread called thread_join on main_thread
 */
void free_main_thread(void) {
    // if main wasn't freed due to a join by another thread
    if (!STAILQ_EMPTY(&runnable_hd)) {
        struct thread *th = STAILQ_FIRST(&runnable_hd);
        VALGRIND_STACK_DEREGISTER(th->valgrind_stackid);
        free(th->uctx.uc_stack.ss_sp);
        free(th);
    }
}

/**
 * initializes the main thread (and context) and adds it to runnable FIFO
 * @return 0 if main initialization successful, -1 on error
 */
int init_main_thread(void) {
    // mark main thread initialized
    main_initialized = 1;

    // register cleanup function after main() returns/exits
    atexit(free_main_thread);

    // allocates memory for a new struct thread instance
    struct thread *main_th = malloc(sizeof(struct thread));
    main_th->flags = 0U;

    // set up the context for the main thread
    getcontext(&main_th->uctx);
    main_th->uctx.uc_link = NULL;
    main_th->uctx.uc_stack.ss_size = STACK_SIZE;
    main_th->uctx.uc_stack.ss_sp = malloc(main_th->uctx.uc_stack.ss_size);
    main_th->valgrind_stackid = VALGRIND_STACK_REGISTER(main_th->uctx.uc_stack.ss_sp,
                                                    main_th->uctx.uc_stack.ss_sp + main_th->uctx.uc_stack.ss_size);

    // add main thread to runnable fifo
    STAILQ_INSERT_HEAD(&runnable_hd, main_th, threads);

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
    // get the thread at the head of runnable FIFO
    struct thread *curr_th = STAILQ_FIRST(&runnable_hd);

    // remove it from runnable FIFO
    STAILQ_REMOVE_HEAD(&runnable_hd, threads);

    // set retval in the thread structure
    curr_th->retval = retval;

    // make the thread joinable
    curr_th->flags |= JOINABLE;

    // set context of the next thread in runnable FIFO
    if (!STAILQ_EMPTY(&runnable_hd)) {
        struct thread *next_th = STAILQ_FIRST(&runnable_hd);
        setcontext(&next_th->uctx);
    }

    exit(EXIT_SUCCESS);
}

void thread_runner(void) {
    // get the thread at the head of runnable FIFO
    struct thread *curr_th = STAILQ_FIRST(&runnable_hd);

    // call the entry function of the thread and pass the return value to thread_exit
    thread_exit(curr_th->func((void *)curr_th->funcarg));
}

/* recuperer l'identifiant du thread courant.
 */
thread_t thread_self(void) {
    // initialize the main thread if not done already
    if (!main_initialized) {
        init_main_thread();
    }

    // get the calling thread from runnable fifo
    struct thread *curr_thread = STAILQ_FIRST(&runnable_hd);
    return (thread_t)curr_thread;
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
    STAILQ_INSERT_TAIL(&runnable_hd, thn, threads);

    return 0;
}

/* passer la main à un autre thread.
 */
int thread_yield(void) {
    // initialize the main thread if not done already
    if (!main_initialized) {
        init_main_thread();
    }

    // get the thread who yielded from head of runnable FIFO
    struct thread *curr_th = STAILQ_FIRST(&runnable_hd);

    // remove it from runnable FIFO
    STAILQ_REMOVE_HEAD(&runnable_hd, threads);

    // insert it at tail
    STAILQ_INSERT_TAIL(&runnable_hd, curr_th, threads);

    // get the next thread in runnable FIFO
    struct thread *next_th = STAILQ_FIRST(&runnable_hd);

    // swap to the context of next thread
    swapcontext(&curr_th->uctx, &next_th->uctx);

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
    while (!(th->flags & JOINABLE)) {
        thread_yield();
    }

    // if retval is not NULL, get the return val set by thread_exit
    if (retval)
        *retval = th->retval;

    // free memory allocated to the thread/context
    VALGRIND_STACK_DEREGISTER(th->valgrind_stackid);
    free(th->uctx.uc_stack.ss_sp);
    free(th);

    return 0;
}

/*      Implémentation des mutexes      */


/*int thread_mutex_init(thread_mutex_t *mutex) {
    if(mutex != NULL) {
        mutex->is_destroyed = 0;
        mutex->locker = NULL;
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}

int thread_mutex_destroy(thread_mutex_t *mutex) {
    if(mutex != NULL) {
        mutex->is_destroyed = 1;
        mutex->locker = NULL;
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}

int thread_mutex_lock(thread_mutex_t *mutex) {
    if(mutex->is_destroyed == 0){
        struct thread *curr_th = STAILQ_FIRST(&runnable_hd); // we should change this to the running thread
        mutex->locker = curr_th;
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}
int thread_mutex_unlock(thread_mutex_t *mutex) {
    struct thread *curr_th = STAILQ_FIRST(&runnable_hd); // we should change this to the running thread
    if(mutex->is_destroyed == 0 && mutex->locker == curr_th){
        STAILQ_REMOVE_HEAD(&runnable_hd, threads);
        if (!STAILQ_EMPTY(&runnable_hd)) {
            struct thread *next_th = STAILQ_FIRST(&runnable_hd);
            mutex->locker = next_th; // changer au prochain thread qui va s'executer
        }
        else{
            if(curr_th->yielded){
                mutex->locker = curr_th; // dans le cas ou on a un seul thread
            }
        }
    }
    return EXIT_FAILURE;
}*/
