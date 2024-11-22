/* ==========================================================================
 * timeout.h - Tickless hierarchical timing wheel.
 * --------------------------------------------------------------------------
 * Copyright (c) 2013, 2014  William Ahern
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the
 * following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
 * NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 * ==========================================================================
 */
#ifndef TIMEOUT_H
#define TIMEOUT_H

#include <stdbool.h>    /* bool */
#include <stdio.h>      /* FILE */

#include <inttypes.h>   /* PRIu64 PRIx64 PRIX64 uint64_t */

//#include <sys/queue.h>  /* TAILQ(3) */
#ifdef QUEUE_MACRO_DEBUG
#define _Q_INVALIDATE(a) (a) = ((void *)-1)
#else
#define _Q_INVALIDATE(a)
#endif

 /*
  * Singly-linked List definitions.
  */
#define SLIST_HEAD(name, type)						\
struct name {								\
	struct type *slh_first;	/* first element */			\
}

#define	SLIST_HEAD_INITIALIZER(head)					\
	{ NULL }

#define SLIST_ENTRY(type)						\
struct {								\
	struct type *sle_next;	/* next element */			\
}

  /*
   * Singly-linked List access methods.
   */
#define	SLIST_FIRST(head)	((head)->slh_first)
#define	SLIST_END(head)		NULL
#define	SLIST_EMPTY(head)	(SLIST_FIRST(head) == SLIST_END(head))
#define	SLIST_NEXT(elm, field)	((elm)->field.sle_next)

#define	SLIST_FOREACH(var, head, field)					\
	for((var) = SLIST_FIRST(head);					\
	    (var) != SLIST_END(head);					\
	    (var) = SLIST_NEXT(var, field))

#define	SLIST_FOREACH_PREVPTR(var, varp, head, field)			\
	for ((varp) = &SLIST_FIRST((head));				\
	    ((var) = *(varp)) != SLIST_END(head);			\
	    (varp) = &SLIST_NEXT((var), field))

   /*
    * Singly-linked List functions.
    */
#define	SLIST_INIT(head) {						\
	SLIST_FIRST(head) = SLIST_END(head);				\
}

#define	SLIST_INSERT_AFTER(slistelm, elm, field) do {			\
	(elm)->field.sle_next = (slistelm)->field.sle_next;		\
	(slistelm)->field.sle_next = (elm);				\
} while (0)

#define	SLIST_INSERT_HEAD(head, elm, field) do {			\
	(elm)->field.sle_next = (head)->slh_first;			\
	(head)->slh_first = (elm);					\
} while (0)

#define	SLIST_REMOVE_NEXT(head, elm, field) do {			\
	(elm)->field.sle_next = (elm)->field.sle_next->field.sle_next;	\
} while (0)

#define	SLIST_REMOVE_HEAD(head, field) do {				\
	(head)->slh_first = (head)->slh_first->field.sle_next;		\
} while (0)

#define SLIST_REMOVE(head, elm, type, field) do {			\
	if ((head)->slh_first == (elm)) {				\
		SLIST_REMOVE_HEAD((head), field);			\
	} else {							\
		struct type *curelm = (head)->slh_first;		\
									\
		while (curelm->field.sle_next != (elm))			\
			curelm = curelm->field.sle_next;		\
		curelm->field.sle_next =				\
		    curelm->field.sle_next->field.sle_next;		\
		_Q_INVALIDATE((elm)->field.sle_next);			\
	}								\
} while (0)

    /*
     * List definitions.
     */
#define LIST_HEAD(name, type)						\
struct name {								\
	struct type *lh_first;	/* first element */			\
}

#define LIST_HEAD_INITIALIZER(head)					\
	{ NULL }

#define LIST_ENTRY(type)						\
struct {								\
	struct type *le_next;	/* next element */			\
	struct type **le_prev;	/* address of previous next element */	\
}

     /*
      * List access methods
      */
#define	LIST_FIRST(head)		((head)->lh_first)
#define	LIST_END(head)			NULL
#define	LIST_EMPTY(head)		(LIST_FIRST(head) == LIST_END(head))
#define	LIST_NEXT(elm, field)		((elm)->field.le_next)

#define LIST_FOREACH(var, head, field)					\
	for((var) = LIST_FIRST(head);					\
	    (var)!= LIST_END(head);					\
	    (var) = LIST_NEXT(var, field))

      /*
       * List functions.
       */
#define	LIST_INIT(head) do {						\
	LIST_FIRST(head) = LIST_END(head);				\
} while (0)

#define LIST_INSERT_AFTER(listelm, elm, field) do {			\
	if (((elm)->field.le_next = (listelm)->field.le_next) != NULL)	\
		(listelm)->field.le_next->field.le_prev =		\
		    &(elm)->field.le_next;				\
	(listelm)->field.le_next = (elm);				\
	(elm)->field.le_prev = &(listelm)->field.le_next;		\
} while (0)

#define	LIST_INSERT_BEFORE(listelm, elm, field) do {			\
	(elm)->field.le_prev = (listelm)->field.le_prev;		\
	(elm)->field.le_next = (listelm);				\
	*(listelm)->field.le_prev = (elm);				\
	(listelm)->field.le_prev = &(elm)->field.le_next;		\
} while (0)

#define LIST_INSERT_HEAD(head, elm, field) do {				\
	if (((elm)->field.le_next = (head)->lh_first) != NULL)		\
		(head)->lh_first->field.le_prev = &(elm)->field.le_next;\
	(head)->lh_first = (elm);					\
	(elm)->field.le_prev = &(head)->lh_first;			\
} while (0)

#define LIST_REMOVE(elm, field) do {					\
	if ((elm)->field.le_next != NULL)				\
		(elm)->field.le_next->field.le_prev =			\
		    (elm)->field.le_prev;				\
	*(elm)->field.le_prev = (elm)->field.le_next;			\
	_Q_INVALIDATE((elm)->field.le_prev);				\
	_Q_INVALIDATE((elm)->field.le_next);				\
} while (0)

#define LIST_REPLACE(elm, elm2, field) do {				\
	if (((elm2)->field.le_next = (elm)->field.le_next) != NULL)	\
		(elm2)->field.le_next->field.le_prev =			\
		    &(elm2)->field.le_next;				\
	(elm2)->field.le_prev = (elm)->field.le_prev;			\
	*(elm2)->field.le_prev = (elm2);				\
	_Q_INVALIDATE((elm)->field.le_prev);				\
	_Q_INVALIDATE((elm)->field.le_next);				\
} while (0)

       /*
        * Simple queue definitions.
        */
#define SIMPLEQ_HEAD(name, type)					\
struct name {								\
	struct type *sqh_first;	/* first element */			\
	struct type **sqh_last;	/* addr of last next element */		\
}

#define SIMPLEQ_HEAD_INITIALIZER(head)					\
	{ NULL, &(head).sqh_first }

#define SIMPLEQ_ENTRY(type)						\
struct {								\
	struct type *sqe_next;	/* next element */			\
}

        /*
         * Simple queue access methods.
         */
#define	SIMPLEQ_FIRST(head)	    ((head)->sqh_first)
#define	SIMPLEQ_END(head)	    NULL
#define	SIMPLEQ_EMPTY(head)	    (SIMPLEQ_FIRST(head) == SIMPLEQ_END(head))
#define	SIMPLEQ_NEXT(elm, field)    ((elm)->field.sqe_next)

#define SIMPLEQ_FOREACH(var, head, field)				\
	for((var) = SIMPLEQ_FIRST(head);				\
	    (var) != SIMPLEQ_END(head);					\
	    (var) = SIMPLEQ_NEXT(var, field))

         /*
          * Simple queue functions.
          */
#define	SIMPLEQ_INIT(head) do {						\
	(head)->sqh_first = NULL;					\
	(head)->sqh_last = &(head)->sqh_first;				\
} while (0)

#define SIMPLEQ_INSERT_HEAD(head, elm, field) do {			\
	if (((elm)->field.sqe_next = (head)->sqh_first) == NULL)	\
		(head)->sqh_last = &(elm)->field.sqe_next;		\
	(head)->sqh_first = (elm);					\
} while (0)

#define SIMPLEQ_INSERT_TAIL(head, elm, field) do {			\
	(elm)->field.sqe_next = NULL;					\
	*(head)->sqh_last = (elm);					\
	(head)->sqh_last = &(elm)->field.sqe_next;			\
} while (0)

#define SIMPLEQ_INSERT_AFTER(head, listelm, elm, field) do {		\
	if (((elm)->field.sqe_next = (listelm)->field.sqe_next) == NULL)\
		(head)->sqh_last = &(elm)->field.sqe_next;		\
	(listelm)->field.sqe_next = (elm);				\
} while (0)

#define SIMPLEQ_REMOVE_HEAD(head, field) do {			\
	if (((head)->sqh_first = (head)->sqh_first->field.sqe_next) == NULL) \
		(head)->sqh_last = &(head)->sqh_first;			\
} while (0)

          /*
           * Tail queue definitions.
           */
#define TAILQ_HEAD(name, type)						\
struct name {								\
	struct type *tqh_first;	/* first element */			\
	struct type **tqh_last;	/* addr of last next element */		\
}

#define TAILQ_HEAD_INITIALIZER(head)					\
	{ NULL, &(head).tqh_first }

#define TAILQ_ENTRY(type)						\
struct {								\
	struct type *tqe_next;	/* next element */			\
	struct type **tqe_prev;	/* address of previous next element */	\
}

           /*
            * tail queue access methods
            */
#define	TAILQ_FIRST(head)		((head)->tqh_first)
#define	TAILQ_END(head)			NULL
#define	TAILQ_NEXT(elm, field)		((elm)->field.tqe_next)
#define TAILQ_LAST(head, headname)					\
	(*(((struct headname *)((head)->tqh_last))->tqh_last))
            /* XXX */
#define TAILQ_PREV(elm, headname, field)				\
	(*(((struct headname *)((elm)->field.tqe_prev))->tqh_last))
#define	TAILQ_EMPTY(head)						\
	(TAILQ_FIRST(head) == TAILQ_END(head))

#define TAILQ_FOREACH(var, head, field)					\
	for((var) = TAILQ_FIRST(head);					\
	    (var) != TAILQ_END(head);					\
	    (var) = TAILQ_NEXT(var, field))

#define TAILQ_FOREACH_REVERSE(var, head, headname, field)		\
	for((var) = TAILQ_LAST(head, headname);				\
	    (var) != TAILQ_END(head);					\
	    (var) = TAILQ_PREV(var, headname, field))

/*
 * Tail queue functions.
 */
#define	TAILQ_INIT(head) do {						\
	(head)->tqh_first = NULL;					\
	(head)->tqh_last = &(head)->tqh_first;				\
} while (0)

#define TAILQ_INSERT_HEAD(head, elm, field) do {			\
	if (((elm)->field.tqe_next = (head)->tqh_first) != NULL)	\
		(head)->tqh_first->field.tqe_prev =			\
		    &(elm)->field.tqe_next;				\
	else								\
		(head)->tqh_last = &(elm)->field.tqe_next;		\
	(head)->tqh_first = (elm);					\
	(elm)->field.tqe_prev = &(head)->tqh_first;			\
} while (0)

#define TAILQ_INSERT_TAIL(head, elm, field) do {			\
	(elm)->field.tqe_next = NULL;					\
	(elm)->field.tqe_prev = (head)->tqh_last;			\
	*(head)->tqh_last = (elm);					\
	(head)->tqh_last = &(elm)->field.tqe_next;			\
} while (0)

#define TAILQ_INSERT_AFTER(head, listelm, elm, field) do {		\
	if (((elm)->field.tqe_next = (listelm)->field.tqe_next) != NULL)\
		(elm)->field.tqe_next->field.tqe_prev =			\
		    &(elm)->field.tqe_next;				\
	else								\
		(head)->tqh_last = &(elm)->field.tqe_next;		\
	(listelm)->field.tqe_next = (elm);				\
	(elm)->field.tqe_prev = &(listelm)->field.tqe_next;		\
} while (0)

#define	TAILQ_INSERT_BEFORE(listelm, elm, field) do {			\
	(elm)->field.tqe_prev = (listelm)->field.tqe_prev;		\
	(elm)->field.tqe_next = (listelm);				\
	*(listelm)->field.tqe_prev = (elm);				\
	(listelm)->field.tqe_prev = &(elm)->field.tqe_next;		\
} while (0)

#define TAILQ_REMOVE(head, elm, field) do {				\
	if (((elm)->field.tqe_next) != NULL)				\
		(elm)->field.tqe_next->field.tqe_prev =			\
		    (elm)->field.tqe_prev;				\
	else								\
		(head)->tqh_last = (elm)->field.tqe_prev;		\
	*(elm)->field.tqe_prev = (elm)->field.tqe_next;			\
	_Q_INVALIDATE((elm)->field.tqe_prev);				\
	_Q_INVALIDATE((elm)->field.tqe_next);				\
} while (0)

#define TAILQ_REPLACE(head, elm, elm2, field) do {			\
	if (((elm2)->field.tqe_next = (elm)->field.tqe_next) != NULL)	\
		(elm2)->field.tqe_next->field.tqe_prev =		\
		    &(elm2)->field.tqe_next;				\
	else								\
		(head)->tqh_last = &(elm2)->field.tqe_next;		\
	(elm2)->field.tqe_prev = (elm)->field.tqe_prev;			\
	*(elm2)->field.tqe_prev = (elm2);				\
	_Q_INVALIDATE((elm)->field.tqe_prev);				\
	_Q_INVALIDATE((elm)->field.tqe_next);				\
} while (0)

 /*
  * Circular queue definitions.
  */
#define CIRCLEQ_HEAD(name, type)					\
struct name {								\
	struct type *cqh_first;		/* first element */		\
	struct type *cqh_last;		/* last element */		\
}

#define CIRCLEQ_HEAD_INITIALIZER(head)					\
	{ CIRCLEQ_END(&head), CIRCLEQ_END(&head) }

#define CIRCLEQ_ENTRY(type)						\
struct {								\
	struct type *cqe_next;		/* next element */		\
	struct type *cqe_prev;		/* previous element */		\
}

  /*
   * Circular queue access methods
   */
#define	CIRCLEQ_FIRST(head)		((head)->cqh_first)
#define	CIRCLEQ_LAST(head)		((head)->cqh_last)
#define	CIRCLEQ_END(head)		((void *)(head))
#define	CIRCLEQ_NEXT(elm, field)	((elm)->field.cqe_next)
#define	CIRCLEQ_PREV(elm, field)	((elm)->field.cqe_prev)
#define	CIRCLEQ_EMPTY(head)						\
	(CIRCLEQ_FIRST(head) == CIRCLEQ_END(head))

#define CIRCLEQ_FOREACH(var, head, field)				\
	for((var) = CIRCLEQ_FIRST(head);				\
	    (var) != CIRCLEQ_END(head);					\
	    (var) = CIRCLEQ_NEXT(var, field))

#define CIRCLEQ_FOREACH_REVERSE(var, head, field)			\
	for((var) = CIRCLEQ_LAST(head);					\
	    (var) != CIRCLEQ_END(head);					\
	    (var) = CIRCLEQ_PREV(var, field))

   /*
    * Circular queue functions.
    */
#define	CIRCLEQ_INIT(head) do {						\
	(head)->cqh_first = CIRCLEQ_END(head);				\
	(head)->cqh_last = CIRCLEQ_END(head);				\
} while (0)

#define CIRCLEQ_INSERT_AFTER(head, listelm, elm, field) do {		\
	(elm)->field.cqe_next = (listelm)->field.cqe_next;		\
	(elm)->field.cqe_prev = (listelm);				\
	if ((listelm)->field.cqe_next == CIRCLEQ_END(head))		\
		(head)->cqh_last = (elm);				\
	else								\
		(listelm)->field.cqe_next->field.cqe_prev = (elm);	\
	(listelm)->field.cqe_next = (elm);				\
} while (0)

#define CIRCLEQ_INSERT_BEFORE(head, listelm, elm, field) do {		\
	(elm)->field.cqe_next = (listelm);				\
	(elm)->field.cqe_prev = (listelm)->field.cqe_prev;		\
	if ((listelm)->field.cqe_prev == CIRCLEQ_END(head))		\
		(head)->cqh_first = (elm);				\
	else								\
		(listelm)->field.cqe_prev->field.cqe_next = (elm);	\
	(listelm)->field.cqe_prev = (elm);				\
} while (0)

#define CIRCLEQ_INSERT_HEAD(head, elm, field) do {			\
	(elm)->field.cqe_next = (head)->cqh_first;			\
	(elm)->field.cqe_prev = CIRCLEQ_END(head);			\
	if ((head)->cqh_last == CIRCLEQ_END(head))			\
		(head)->cqh_last = (elm);				\
	else								\
		(head)->cqh_first->field.cqe_prev = (elm);		\
	(head)->cqh_first = (elm);					\
} while (0)

#define CIRCLEQ_INSERT_TAIL(head, elm, field) do {			\
	(elm)->field.cqe_next = CIRCLEQ_END(head);			\
	(elm)->field.cqe_prev = (head)->cqh_last;			\
	if ((head)->cqh_first == CIRCLEQ_END(head))			\
		(head)->cqh_first = (elm);				\
	else								\
		(head)->cqh_last->field.cqe_next = (elm);		\
	(head)->cqh_last = (elm);					\
} while (0)

#define	CIRCLEQ_REMOVE(head, elm, field) do {				\
	if ((elm)->field.cqe_next == CIRCLEQ_END(head))			\
		(head)->cqh_last = (elm)->field.cqe_prev;		\
	else								\
		(elm)->field.cqe_next->field.cqe_prev =			\
		    (elm)->field.cqe_prev;				\
	if ((elm)->field.cqe_prev == CIRCLEQ_END(head))			\
		(head)->cqh_first = (elm)->field.cqe_next;		\
	else								\
		(elm)->field.cqe_prev->field.cqe_next =			\
		    (elm)->field.cqe_next;				\
	_Q_INVALIDATE((elm)->field.cqe_prev);				\
	_Q_INVALIDATE((elm)->field.cqe_next);				\
} while (0)

#define CIRCLEQ_REPLACE(head, elm, elm2, field) do {			\
	if (((elm2)->field.cqe_next = (elm)->field.cqe_next) ==		\
	    CIRCLEQ_END(head))						\
		(head).cqh_last = (elm2);				\
	else								\
		(elm2)->field.cqe_next->field.cqe_prev = (elm2);	\
	if (((elm2)->field.cqe_prev = (elm)->field.cqe_prev) ==		\
	    CIRCLEQ_END(head))						\
		(head).cqh_first = (elm2);				\
	else								\
		(elm2)->field.cqe_prev->field.cqe_next = (elm2);	\
	_Q_INVALIDATE((elm)->field.cqe_prev);				\
	_Q_INVALIDATE((elm)->field.cqe_next);				\
} while (0)
///////////////////////////////////////

/*
 * V E R S I O N  I N T E R F A C E S
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#if !defined TIMEOUT_PUBLIC
#define TIMEOUT_PUBLIC
#endif

#define TIMEOUT_VERSION TIMEOUT_V_REL
#define TIMEOUT_VENDOR  "william@25thandClement.com"

#define TIMEOUT_V_REL 0x20160226
#define TIMEOUT_V_ABI 0x20160224
#define TIMEOUT_V_API 0x20160226

TIMEOUT_PUBLIC int timeout_version(void);

TIMEOUT_PUBLIC const char *timeout_vendor(void);

TIMEOUT_PUBLIC int timeout_v_rel(void);

TIMEOUT_PUBLIC int timeout_v_abi(void);

TIMEOUT_PUBLIC int timeout_v_api(void);


/*
 * I N T E G E R  T Y P E  I N T E R F A C E S
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define TIMEOUT_C(n) UINT64_C(n)
#define TIMEOUT_PRIu PRIu64
#define TIMEOUT_PRIx PRIx64
#define TIMEOUT_PRIX PRIX64

#define TIMEOUT_mHZ TIMEOUT_C(1000)
#define TIMEOUT_uHZ TIMEOUT_C(1000000)
#define TIMEOUT_nHZ TIMEOUT_C(1000000000)

typedef uint64_t timeout_t;

#define timeout_error_t int /* for documentation purposes */


/*
 * C A L L B A C K  I N T E R F A C E
 *
 * Callback function parameters unspecified to make embedding into existing
 * applications easier.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef TIMEOUT_CB_OVERRIDE
struct timeout_cb {
	void (*fn)();
	void *arg;
}; /* struct timeout_cb */
#endif

/*
 * T I M E O U T  I N T E R F A C E S
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef TIMEOUT_DISABLE_INTERVALS
#define TIMEOUT_INT 0x01 /* interval (repeating) timeout */
#endif
#define TIMEOUT_ABS 0x02 /* treat timeout values as absolute */

#define TIMEOUT_INITIALIZER(flags) { (flags) }

#define timeout_setcb(to, fn, arg) do { \
	(to)->callback.fn = (fn);       \
	(to)->callback.arg = (arg);     \
} while (0)

struct timeout {
	int flags;

	timeout_t expires;
	/* absolute expiration time */

	struct timeout_list *pending;
	/* timeout list if pending on wheel or expiry queue */

	TAILQ_ENTRY(timeout) tqe;
	/* entry member for struct timeout_list lists */

#ifndef TIMEOUT_DISABLE_CALLBACKS
	struct timeout_cb callback;
	/* optional callback information */
#endif

#ifndef TIMEOUT_DISABLE_INTERVALS
	timeout_t interval;
	/* timeout interval if periodic */
#endif

#ifndef TIMEOUT_DISABLE_RELATIVE_ACCESS
	struct timeouts *timeouts;
	/* timeouts collection if member of */
#endif
}; /* struct timeout */


TIMEOUT_PUBLIC struct timeout *timeout_init(struct timeout *, int);
/* initialize timeout structure (same as TIMEOUT_INITIALIZER) */

#ifndef TIMEOUT_DISABLE_RELATIVE_ACCESS
TIMEOUT_PUBLIC bool timeout_pending(struct timeout *);
/* true if on timing wheel, false otherwise */
 
TIMEOUT_PUBLIC bool timeout_expired(struct timeout *);
/* true if on expired queue, false otherwise */

TIMEOUT_PUBLIC void timeout_del(struct timeout *);
/* remove timeout from any timing wheel (okay if not member of any) */
#endif

/*
 * T I M I N G  W H E E L  I N T E R F A C E S
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

struct timeouts;

TIMEOUT_PUBLIC struct timeouts *timeouts_open(timeout_t, timeout_error_t *);
/* open a new timing wheel, setting optional HZ (for float conversions) */

TIMEOUT_PUBLIC void timeouts_close(struct timeouts *);
/* destroy timing wheel */

TIMEOUT_PUBLIC timeout_t timeouts_hz(struct timeouts *);
/* return HZ setting (for float conversions) */

TIMEOUT_PUBLIC void timeouts_update(struct timeouts *, timeout_t);
/* update timing wheel with current absolute time */

TIMEOUT_PUBLIC void timeouts_step(struct timeouts *, timeout_t);
/* step timing wheel by relative time */

TIMEOUT_PUBLIC timeout_t timeouts_timeout(struct timeouts *);
/* return interval to next required update */

TIMEOUT_PUBLIC void timeouts_add(struct timeouts *, struct timeout *, timeout_t);
/* add timeout to timing wheel */

TIMEOUT_PUBLIC void timeouts_del(struct timeouts *, struct timeout *);
/* remove timeout from any timing wheel or expired queue (okay if on neither) */

TIMEOUT_PUBLIC struct timeout *timeouts_get(struct timeouts *);
/* return any expired timeout (caller should loop until NULL-return) */

TIMEOUT_PUBLIC bool timeouts_pending(struct timeouts *);
/* return true if any timeouts pending on timing wheel */

TIMEOUT_PUBLIC bool timeouts_expired(struct timeouts *);
/* return true if any timeouts on expired queue */

TIMEOUT_PUBLIC bool timeouts_check(struct timeouts *, FILE *);
/* return true if invariants hold. describes failures to optional file handle. */

#define TIMEOUTS_PENDING 0x10
#define TIMEOUTS_EXPIRED 0x20
#define TIMEOUTS_ALL     (TIMEOUTS_PENDING|TIMEOUTS_EXPIRED)
#define TIMEOUTS_CLEAR   0x40

#define TIMEOUTS_IT_INITIALIZER(flags) { (flags), 0, 0, 0, 0 }

#define TIMEOUTS_IT_INIT(cur, _flags) do {                              \
	(cur)->flags = (_flags);                                        \
	(cur)->pc = 0;                                                  \
} while (0)

struct timeouts_it {
	int flags;
	unsigned pc, i, j;
	struct timeout *to;
}; /* struct timeouts_it */

TIMEOUT_PUBLIC struct timeout *timeouts_next(struct timeouts *, struct timeouts_it *);
/* return next timeout in pending wheel or expired queue. caller can delete
 * the returned timeout, but should not otherwise manipulate the timing
 * wheel. in particular, caller SHOULD NOT delete any other timeout as that
 * could invalidate cursor state and trigger a use-after-free.
 */

#define TIMEOUTS_FOREACH(var, T, flags)                                 \
	struct timeouts_it _it = TIMEOUTS_IT_INITIALIZER((flags));      \
	while (((var) = timeouts_next((T), &_it)))


/*
 * B O N U S  W H E E L  I N T E R F A C E S
 *
 * I usually use floating point timeouts in all my code, but it's cleaner to
 * separate it to keep the core algorithmic code simple.
 *
 * Using macros instead of static inline routines where <math.h> routines
 * might be used to keep -lm linking optional.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <math.h> /* ceil(3) */

#define timeouts_f2i(T, f) \
	((timeout_t)ceil((f) * timeouts_hz((T)))) /* prefer late expiration over early */

#define timeouts_i2f(T, i) \
	((double)(i) / timeouts_hz((T)))

#define timeouts_addf(T, to, timeout) \
	timeouts_add((T), (to), timeouts_f2i((T), (timeout)))

#endif /* TIMEOUT_H */
