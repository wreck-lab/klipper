#ifndef __LINUX_IRQ_H
#define __LINUX_IRQ_H

#include <signal.h> // sigset_t

typedef sigset_t irqstatus_t;

void irq_disable(void);
void irq_enable(void);
irqstatus_t irq_save(void);
void irq_restore(irqstatus_t flag);
void irq_wait(void);
void irq_poll(void);

#endif // irq.h
