#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/atomic.h>
#include <linux/jiffies.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define __preserve noinline

#define STOPPED_VALUE (-(1<<20))
#define MINUTE (60)

MODULE_LICENSE("GPL");

static atomic_t counter = ATOMIC_INIT(0);
static DEFINE_TIMER(timer, 0);

static irqreturn_t keyboard_irq_handler(int irq_line, void *dev) {
    atomic_t *cnt = dev;
    atomic_inc(cnt);
    return 0;
}

static void reschedule(struct timer_list* tmr, unsigned long delay_sec) {
    unsigned long j1 = tmr->expires + delay_sec * HZ;
    unsigned long j2 = jiffies + HZ;
    if (time_after(j1, j2)) {
        tmr->expires = j1;
    } else {
        tmr->expires = j2;
    }
}

static void collect_stats(struct timer_list* tmr) {
    int value = atomic_xchg(&counter, 0);
    if (value < 0) {
        return;
    }

    printk(KERN_INFO "keystats_kmodule: received %d interrupts on IRQ 1\n", value);
    printk(KERN_INFO "keystats_kmodule: approximately %d key presses\n", value / 2);
    reschedule(tmr, MINUTE);
    add_timer(tmr);
}

static int __init first_module_init(void) {
    printk(KERN_INFO "keystats_kmodule: init\n");
    int ret = request_irq(1, keyboard_irq_handler, IRQF_SHARED, "keyboard", &counter);
    if (ret) {
        printk(KERN_ERR "keystats_kmodule: failed to acquire IRQ (%d)\n", ret);
        return ret;
    }
    timer.function = collect_stats;
    timer.expires = jiffies;
    reschedule(&timer, MINUTE);
    add_timer(&timer);
    printk(KERN_INFO "keystats_kmodule: initialization completed\n");
    return 0;
}

static void __exit first_module_exit(void) {
    free_irq(1, &counter);
    atomic_set(&counter, STOPPED_VALUE);
    del_timer_sync(&timer);
    printk(KERN_INFO "keystats_kmodule: exit\n");
}


module_init(first_module_init);
module_exit(first_module_exit);
