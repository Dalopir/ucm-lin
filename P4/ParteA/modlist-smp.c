#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Linked List Module Practica 4 - FDI-UCM");
MODULE_AUTHOR("Jorge Hernandez Palop & Daniel Lopez Piris");

#define BUFFER_LENGTH 128

static struct list_head mylist;
static struct proc_dir_entry *proc_entry;
static DEFINE_RWLOCK(rwl);

struct list_item {
  int data;
  struct list_head links;
};

static int add(char *buffer)
{
  int valid;
  int value;
  struct list_item *new_node;

  valid = sscanf(buffer, "add %d", &value) == 1;
  if (!valid)
    return 0;

  /* kmalloc() may sleep: do it BEFORE taking the spinlock */
  new_node = kmalloc(sizeof(*new_node), GFP_KERNEL);
  if (new_node == NULL)
    return -ENOMEM;

  new_node->data = value;

  write_lock(&rwl);
  list_add_tail(&new_node->links, &mylist);
  write_unlock(&rwl);

  return 1;
}

static int remove(char *buffer)
{
  int valid;
  int value;
  struct list_head *cur_node, *aux_node;
  struct list_item *item, *tmp;
  LIST_HEAD(to_free);

  valid = sscanf(buffer, "remove %d", &value) == 1;
  if (!valid)
    return 0;

  write_lock(&rwl);
  list_for_each_safe(cur_node, aux_node, &mylist) {
    item = list_entry(cur_node, struct list_item, links);
    if (item->data == value) {
      list_del(cur_node);
      list_add_tail(cur_node, &to_free);
    }
  }
  write_unlock(&rwl);

  /* Free outside the spinlock: shorter critical section. */
  list_for_each_entry_safe(item, tmp, &to_free, links) {
    list_del(&item->links);
    kfree(item);
  }

  return 1;
}

static void cleanup_aux(void)
{
  struct list_head *cur_node, *aux_node;
  struct list_item *item, *tmp;
  LIST_HEAD(to_free);

  write_lock(&rwl);
  list_for_each_safe(cur_node, aux_node, &mylist) {
    item = list_entry(cur_node, struct list_item, links);
    list_del(cur_node);
    list_add_tail(cur_node, &to_free);
  }
  write_unlock(&rwl);

  list_for_each_entry_safe(item, tmp, &to_free, links) {
    list_del(&item->links);
    kfree(item);
  }
}

static int cleanup(char *buffer)
{
  char cmd[16];

  if (sscanf(buffer, "%15s", cmd) != 1 || strcmp(cmd, "cleanup") != 0)
    return 0;

  cleanup_aux();
  return 1;
}

static int modlist_open(struct inode *inode, struct file *file)
{
  /* Pin the module while the file is open; rmmod will fail with EBUSY. */
  if (!try_module_get(THIS_MODULE))
    return -ENODEV;

  return 0;
}

static int modlist_release(struct inode *inode, struct file *file)
{
  module_put(THIS_MODULE);
  return 0;
}

static ssize_t modlist_write(struct file *filp, const char __user *buf,
                             size_t len, loff_t *off)
{
  char buffer[BUFFER_LENGTH];
  int available_space = BUFFER_LENGTH - 1;
  int res;

  if (len > available_space) {
    pr_info("Modlist: not enough space!!\n");
    return -ENOSPC;
  }

  if (len == 0)
    return 0;

  if (copy_from_user(buffer, buf, len))
    return -EFAULT;

  buffer[len] = '\0';

  res = add(buffer);
  if (res == 0)
    res = remove(buffer);
  if (res == 0)
    res = cleanup(buffer);

  if (res == 0) {
    pr_info("Modlist: invalid command!!\n");
    return -EINVAL;
  }
  if (res < 0)
    return res;

  return len;
}

static ssize_t modlist_read(struct file *filp, char __user *buf,
                            size_t len, loff_t *off)
{
  char kbuf[BUFFER_LENGTH];
  int i = 0;
  int sz, written;
  int truncated = 0;
  struct list_item *item;
  struct list_head *cur_node;
  size_t n;

  if ((*off) > 0)
    return 0;

  read_lock(&rwl);
  list_for_each(cur_node, &mylist) {
    item = list_entry(cur_node, struct list_item, links);
    sz = BUFFER_LENGTH - i;
    written = snprintf(kbuf + i, sz, "%d\n", item->data);

    if (written >= sz) {
      truncated = 1;
      break;
    }
    i += written;
  }
  read_unlock(&rwl);

  if (truncated)
    pr_info("Modlist: Buffer size exceeded. Copying to user.\n");

  n = min_t(size_t, (size_t)i, len);
  if (copy_to_user(buf, kbuf, n))
    return -EFAULT;

  (*off) += n;
  return n;
}

static const struct proc_ops proc_entry_fops = {
    .proc_open = modlist_open,
    .proc_read = modlist_read,
    .proc_write = modlist_write,
    .proc_release = modlist_release,
};

static int __init modulo_lin_init(void)
{
  INIT_LIST_HEAD(&mylist);

  proc_entry = proc_create("modlist", 0666, NULL, &proc_entry_fops);
  if (proc_entry == NULL) {
    pr_err("Modlist: Can't create /proc entry\n");
    return -ENOMEM;
  }

  pr_info("Modlist: Module loaded\n");
  return 0;
}

static void __exit modulo_lin_clean(void)
{
  /* Remove the entry first; open files pin the module, so no user
   * can be inside the list code when this runs. */
  remove_proc_entry("modlist", NULL);
  cleanup_aux();
  pr_info("Modlist: Module unloaded.\n");
}

module_init(modulo_lin_init);
module_exit(modulo_lin_clean);
