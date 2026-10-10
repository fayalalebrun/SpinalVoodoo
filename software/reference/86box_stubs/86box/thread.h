/* Stub: threading primitives — all no-ops for single-threaded ref model */
#ifndef _86BOX_THREAD_H
#define _86BOX_THREAD_H

typedef void thread_t;
typedef void event_t;
typedef void mutex_t;

static inline thread_t *thread_create(void (*fn)(void *), void *arg)
    { (void)fn; (void)arg; return NULL; }
static inline event_t *thread_create_event(void) { return NULL; }
static inline mutex_t *thread_create_mutex(void) { return NULL; }
static inline void thread_set_event(event_t *e) { (void)e; }
static inline void thread_reset_event(event_t *e) { (void)e; }
static inline void thread_wait_event(event_t *e, int timeout)
    { (void)e; (void)timeout; }
static inline void thread_wait_mutex(mutex_t *m) { (void)m; }
static inline void thread_release_mutex(mutex_t *m) { (void)m; }

#endif /* _86BOX_THREAD_H */
