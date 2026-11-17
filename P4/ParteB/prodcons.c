#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/kfifo.h>
#include <linux/semaphore.h>
#include <linux/uaccess.h>
#include <linux/kstrtox.h>
#include <linux/string.h>
#include <linux/moduleparam.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Producer/Consumer kfifo module - P4 Parte B");
MODULE_AUTHOR("Jorge Hernandez Palop & Daniel Lopez Piris");

#define DEVICE_NAME "prodcons"
#define LINE_MAX 32

static int long_buffer_prodcon = 0;
module_param(long_buffer_prodcon, int, 0444);
MODULE_PARM_DESC(long_buffer_prodcon, "0 => 4 ints, nonzero => 8 ints");

static struct kfifo cbuf;
static struct semaphore mtx;
static struct semaphore huecos;
static struct semaphore elementos;

static ssize_t prodcons_write(struct file *filp, const char __user *ubuf,
                              size_t len, loff_t *off) {
    char kbuf[LINE_MAX];
    int val, rc;

    if (len == 0)
        return 0;
    if (len >= sizeof(kbuf))
        return -EINVAL;
    if (copy_from_user(kbuf, ubuf, len))
        return -EFAULT;
    kbuf[len] = '\0';

    rc = kstrtoint(strim(kbuf), 10, &val);
    if (rc)
        return rc;

    if (down_interruptible(&huecos))
        return -ERESTARTSYS;
    if (down_interruptible(&mtx)) {
        up(&huecos);
        return -ERESTARTSYS;
    }
    if (kfifo_in(&cbuf, &val, sizeof(val)) != sizeof(val)) {
        up(&mtx);
        up(&huecos);
        return -EIO;
    }
    up(&mtx);
    up(&elementos);

    return len;
}

static ssize_t prodcons_read(struct file *filp, char __user *ubuf,
                             size_t len, loff_t *off)
{
    char kbuf[16];
    int val, n;

    if (*off > 0)
        return 0;

    if (down_interruptible(&elementos))
        return -ERESTARTSYS;
    if (down_interruptible(&mtx)) {
        up(&elementos);
        return -ERESTARTSYS;
    }
    if (kfifo_out(&cbuf, &val, sizeof(val)) != sizeof(val)) {
        up(&mtx);
        up(&elementos);
        return -EIO;
    }
    up(&mtx);
    up(&huecos);

    n = scnprintf(kbuf, sizeof(kbuf), "%d\n", val);
    if ((size_t)n > len) {
        return -EINVAL;
    }

    if (copy_to_user(ubuf, kbuf, n))
        return -EFAULT;
    *off += n;
    return n;
}

static const struct file_operations prodcons_fops = {
    .owner = THIS_MODULE,
    .read = prodcons_read,
    .write = prodcons_write,
};

static struct miscdevice prodcons_misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = DEVICE_NAME,
    .fops = &prodcons_fops,
    .mode = 0666,
};

static int __init prodcons_init(void)
{
    int elems = long_buffer_prodcon ? 8 : 4;
    int rc;

    sema_init(&mtx, 1);
    sema_init(&huecos, elems);
    sema_init(&elementos, 0);

    rc = kfifo_alloc(&cbuf, elems * sizeof(int), GFP_KERNEL);
    if (rc)
        return rc;

    rc = misc_register(&prodcons_misc);
    if (rc) {
        kfifo_free(&cbuf);
        return rc;
    }

    pr_info("Prodcons: /dev/%s registered (%d ints)\n", DEVICE_NAME, elems);
    return 0;
}

static void __exit prodcons_exit(void)
{
    misc_deregister(&prodcons_misc);
    kfifo_free(&cbuf);
    pr_info("Prodcons: unloaded\n");
}

module_init(prodcons_init);
module_exit(prodcons_exit);
