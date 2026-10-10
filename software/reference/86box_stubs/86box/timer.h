/* Stub: timer types */
#ifndef _86BOX_TIMER_H
#define _86BOX_TIMER_H

typedef struct pc_timer_t { int dummy; } pc_timer_t;

static inline void timer_add(pc_timer_t *t, void (*fn)(void *), void *arg, int delay)
    { (void)t; (void)fn; (void)arg; (void)delay; }

#endif /* _86BOX_TIMER_H */
