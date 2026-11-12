#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/uaccess.h>
#include <linux/moduleparam.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Producer Consumer Module Practica 4 - FDI-UCM");
MODULE_AUTHOR("Jorge Hernandez Palop & Daniel Lopez Piris");

static int long_buffer_prodcon = 0;
module_param(long_buffer_prodcon, int, 0000);
MODULE_PARAM_DEC(long_buffer_prodcon, "If not 0 enenable long buffer (8B instead of 4B)");

static struct list_head mylist;
static struct proc_dir_entry* proc_entry;
static struct semaphore* mtx, huecos, elementos;
static struct kfifo cbuff;

static ssize_t prodcon_write(struct file* filp, const char __user* buf, size_t len, loff_t* off) {
  
}

static ssize_t prodcon_read(struct file* filp, char __user* buf, size_t len, loff_t* off) {

}

static const struct proc_ops proc_entry_fops = {
    .proc_read =  prodcon_read,
    .proc_write = prodcon_write,
};

int modulo_lin_init(void) {
  int ret = 0;
  size_t buffer_size = long_buffer_prodcon ? 4 : 8;

  sema_init(&mtx, 1);
  sema_init(&huecos, 0);
  sema_init(&elementos, buffer_size);

  ret = kfifo_alloc(&cbuff, buffer_size, GFP_KERNEL);
  if(ret != 0) goto kfifo_alloc_failed;

  proc_entry = proc_create("prodcons", 0666, NULL, &proc_entry_fops);
  if (proc_entry == NULL) {
    ret = -ENOMEM;
    printk(KERN_INFO "Prodcons: Can't create /proc entry\n");
    goto proc_create_failed;
  } else {
    printk(KERN_INFO "Prodcons: Module loaded\n");
  }

  return ret;
proc_create_failed:
  kfifo_free(&cbuff);
fifo_allocation_failed:
  return ret;
}

void modulo_lin_clean(void) {
  remove_proc_entry("prodcons", NULL);
  kfifo_free(&cbuff);

  printk(KERN_INFO "Prodcons: Module unloaded.\n");
}

/* Declaración de funciones init y exit */
module_init(modulo_lin_init);
module_exit(modulo_lin_clean);
