#include <linux/syscalls.h>
#include <linux/vt_kern.h>
#include <linux/kernel.h>

struct tty_driver* kbd_driver = NULL;

struct tty_driver* get_kbd_driver_handler(void){
   return vc_cons[fg_console].d->port.tty->driver;
}

static inline int set_leds(struct tty_driver* handler, unsigned int mask){
    return (handler->ops->ioctl) (vc_cons[fg_console].d->port.tty, KDSETLED, mask);
}

SYSCALL_DEFINE1(ledctl, unsigned int, arg) {
    if (arg > 7)
        return -EINVAL;

    unsigned int leds = ((arg&0x4)>>1)|((arg&0x2)<<1)|(arg&0x1);

    if (kbd_driver == NULL)
        kbd_driver = get_kbd_driver_handler();

    int err = set_leds(kbd_driver, leds);
    if (err != 0) {
        printk(KERN_INFO "ledctl: something went wrong\n");
        return err;
    }

    return 0;
}

