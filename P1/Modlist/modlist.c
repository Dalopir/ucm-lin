#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Linked List Module Practica 1 - FDI-UCM");
MODULE_AUTHOR("Jorge Hernandez Palop & Daniel Lopez Piris");

#define BUFFER_LENGTH 128

static struct list_head mylist;
static struct proc_dir_entry* proc_entry;

struct list_item {
  int data;
  struct list_head links;
};

int add(char* buffer) {
  // Declaracion de variables
  int valid = 0;
  struct list_item* new_node;
  int value;

  // Comprobar que el comando es válido
  valid = sscanf(buffer, "add %d", &value) == 1;

  if (!valid)
    return 0;

  // Crear y anadir nodo
  new_node = kmalloc(sizeof(struct list_item), GFP_KERNEL);
  if (new_node == NULL)
    return -ENOMEM;

  new_node->data = value;
  list_add_tail(&new_node->links, &mylist);

  return 1;
}

int remove(char* buffer) {
  // Declaracion de variables
  int valid = 0;
  int same_value = 0;
  struct list_item* item = NULL;
  struct list_head* cur_node = NULL;
  struct list_head* aux_node = NULL;
  int value;

  // Comprobar que el comando es válido
  valid = sscanf(buffer, "remove %d", &value) == 1;

  if (!valid)
    return 0;

  // Eliminar los nodos
  list_for_each_safe(cur_node, aux_node, &mylist) {
    item = list_entry(cur_node, struct list_item, links);

    same_value = value == item->data;

    if (same_value) {
      list_del(cur_node);
      kfree(item);
    }
  }

  return 1;
}

void cleanup_aux(void) {
  // Declaracion de variables
  struct list_item* item = NULL;
  struct list_head* cur_node = NULL;
  struct list_head* aux_node = NULL;

  // Eliminar todos los nodos
  list_for_each_safe(cur_node, aux_node, &mylist) {
    item = list_entry(cur_node, struct list_item, links);
    list_del(cur_node);
    kfree(item);
  }
}

int cleanup(char* buffer) {
  // Declaracion de variables
  char cmd[16];

  // Comprobar que el comando es válido
  if (sscanf(buffer, "%15s", cmd) != 1 || strcmp(cmd, "cleanup") != 0)
    return 0;

  // Eliminar todos los nodos
  cleanup_aux();
  return 1;
}

static ssize_t modlist_write(struct file* filp, const char __user* buf, size_t len, loff_t* off) {
  char buffer[BUFFER_LENGTH];
  int available_space = BUFFER_LENGTH - 1;
  int res;

  if ((*off) > 0)
    return 0;

  if (len > available_space) {
    printk(KERN_INFO "Modlist: not enough space!!\n");
    return -ENOSPC;
  }

  /* Transfer data from user to kernel space */
  if (copy_from_user(buffer, buf, len))
    return -EFAULT;

  buffer[len] = '\0';
  *off += len;

  res = add(buffer);
  if (res == 0)
    res = remove(buffer);
  if (res == 0)
    res = cleanup(buffer);
  if (res == 0) {
    printk(KERN_INFO "Modlist: invalid command!!\n");
    return -EINVAL;
  }
  if (res < 0)
    return res;

  return len;
}

static ssize_t modlist_read(struct file* filp, char __user* buf, size_t len, loff_t* off) {
  // Declaracion de variables
  char kbuf[BUFFER_LENGTH];
  int i = 0;
  struct list_item* item = NULL;
  struct list_head* cur_node = NULL;

  if ((*off) > 0) /* Tell the application that there is nothing left to read */
    return 0;

  list_for_each(cur_node, &mylist) {
    item = list_entry(cur_node, struct list_item, links);
    int sz = BUFFER_LENGTH - i;
    int written = snprintf(kbuf + i, sz, "%d\n", item->data);
    if (written >= sz) {
      printk(KERN_INFO "Modlist: Buffer size exceeded. Copying to user.\n");
      break;
    }
    i += written;
  }

  size_t n = min_t(size_t, (size_t)i, len);
  if (copy_to_user(buf, kbuf, n))
    return -EFAULT;

  (*off) += n; /* Update the file pointer */

  return n;
}

static const struct proc_ops proc_entry_fops = {
    .proc_read = modlist_read,
    .proc_write = modlist_write,
};

/* Función que se invoca cuando se carga el módulo en el kernel */
int modulo_lin_init(void) {
  int ret = 0;

  INIT_LIST_HEAD(&mylist);

  proc_entry = proc_create("modlist", 0666, NULL, &proc_entry_fops);
  if (proc_entry == NULL) {
    ret = -ENOMEM;
    printk(KERN_INFO "Modlist: Can't create /proc entry\n");
  } else {
    printk(KERN_INFO "Modlist: Module loaded\n");
  }

  return ret;
}

/* Función que se invoca cuando se descarga el módulo del kernel */
void modulo_lin_clean(void) {
  cleanup_aux();
  remove_proc_entry("modlist", NULL);
  printk(KERN_INFO "Modlist: Module unloaded.\n");
}

/* Declaración de funciones init y exit */
module_init(modulo_lin_init);
module_exit(modulo_lin_clean);
