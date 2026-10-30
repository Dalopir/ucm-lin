#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm-generic/errno.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,2,1)
#define __cconst__ const
#else
#define __cconst__ 
#endif

MODULE_DESCRIPTION("Modleds-Dev-Pi Kernel Module");
MODULE_AUTHOR("Jorge Hernandez Palop & Daniel Lopez Piris");
MODULE_LICENSE("GPL");

/* Actual GPIOs used for controlling LEDs */

#define ALL_LEDS_ON 0x7
#define ALL_LEDS_OFF 0
#define NR_GPIO_LEDS  3

const int led_gpio[NR_GPIO_LEDS] = {25, 27, 4};

/* Array to hold gpio descriptors */
struct gpio_desc* gpio_descriptors[NR_GPIO_LEDS];

/* Set led state to that specified by mask */
static inline int set_pi_leds(unsigned int mask) {
  int i;
  for (i = 0; i < NR_GPIO_LEDS; i++)
    gpiod_set_value(gpio_descriptors[i], (mask >> i) & 0x1 );
  return 0;
}


static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

#define DEVICE_NAME "leds"
#define CLASS_NAME "lights"
#define BUFFER_LENGTH 32

static dev_t start;
static struct cdev* ledsdev = NULL;
static struct class* class = NULL;
static struct device* device = NULL;
static int is_device_open = 0;

static struct file_operations fops = {
    .write = device_write,
    .open = device_open,
    .release = device_release
};


static char *lights_devnode(__cconst__ struct device *dev, umode_t *mode)
{
    if (!mode) {
      return NULL;
    }
    if (MAJOR(dev->devt) == MAJOR(start)) {
        *mode = 0666;
    }
    return NULL;
}

static int __init init_led_module(void) {
  int major;
  int minor;
  int ret;

  if (( ret = alloc_chrdev_region(&start, 0, 1, DEVICE_NAME))) {
      printk(KERN_INFO "Can't allocate chrdev_region()");
      return ret;
  }

  if ((ledsdev = cdev_alloc()) == NULL) {
    printk(KERN_INFO "cdev_alloc() failed ");
    ret = -ENOMEM;
    goto error_alloc;
  }

  cdev_init(ledsdev, &fops);
  if ((ret = cdev_add(ledsdev, start, 1))) {
    printk(KERN_INFO "cdev_add() failed ");
    goto error_add;
  }

  class = class_create(THIS_MODULE, CLASS_NAME);

  if (IS_ERR(class)) {
    printk(KERN_INFO "class_create() failed ");
    ret = PTR_ERR(class);
    goto error_class;
  }

  class->devnode = lights_devnode;
  
  device = device_create(class, NULL, start, NULL, DEVICE_NAME);

  if(IS_ERR(device)) {
    printk(KERN_INFO "device_create() failed ");
    ret = PTR_ERR(device);
    goto error_device;
  }

  major = MAJOR(start);
  minor = MINOR(start);

  // Initialize GPIO
  int i, j;
  char gpio_str[10];

  for (i = 0; i < NR_GPIO_LEDS; i++) {
    sprintf(gpio_str, "led_%d", i);
    if ((ret = gpio_request(led_gpio[i], gpio_str))) {
      pr_err("gpio_request %d failed %d request\n", i, led_gpio[i]);
      goto error_gpio;
    }

    if (!(gpio_descriptors[i] = gpio_to_desc(led_gpio[i]))) {
      pr_err("gpio_to_desc %d failed %d request\n", i, led_gpio[i]);
      ret = -EINVAL;
      goto error_gpio;
    }

    gpiod_direction_output(gpio_descriptors[i], 0);
  }

  set_pi_leds(0b111);

  return 0;

error_gpio:
  for (j = 0; j < i; j++)
    gpiod_put(gpio_descriptors[j]);
  device_destroy(class, start);
error_device:
  class_destroy(class);
error_class:
  if(ledsdev) {
    cdev_del(ledsdev);
    ledsdev = NULL;
  }
error_add:
  if(ledsdev) {
    kobject_put(&ledsdev->kobj);
  }
error_alloc:
  unregister_chrdev_region(start, 1);

  pr_err("modleds module loading failed with code %d\n", ret);
  return ret;
}

static void __exit exit_led_module(void) {
  if (device)
    device_destroy(class, device->devt);

  if (class)
    class_destroy(class);

  if (ledsdev)
    cdev_del(ledsdev);

  unregister_chrdev_region(start, 1);

  int i = 0;

  set_pi_leds(ALL_LEDS_OFF);

  for (i = 0; i < NR_GPIO_LEDS; i++)
    gpiod_put(gpio_descriptors[i]);
}

static int device_open(struct inode *inode, struct file *file) {
  if (is_device_open)
    return -EBUSY;

  is_device_open++;

  try_module_get(THIS_MODULE);
  return 0;
}

static int device_release(struct inode *inode, struct file * file) {
  is_device_open--;
  module_put(THIS_MODULE);
  return 0;
}

static ssize_t device_write(struct file *filp, const char *buff, size_t len, loff_t * off)
{

  char buffer[BUFFER_LENGTH];
  size_t available_space = BUFFER_LENGTH - 1;
  size_t bytes_to_write = len;
  if(bytes_to_write > available_space) bytes_to_write = available_space;

  if(copy_from_user(buffer, buff, bytes_to_write))
    return -EFAULT;

  buffer[bytes_to_write] = '\0';
  
  int x;
  if(kstrtoint(buffer, 10, &x))
    return -EINVAL;

  if(x < 0 || 7 < x) return -EINVAL;
  
  x = ((x & 0b100) >> 2) |
      (x & 0b010) |
      ((x & 0b001) << 2);

  printk(KERN_INFO "Received valid parameter");
  set_pi_leds(x);

  return bytes_to_write;
}

module_init(init_led_module);
module_exit(exit_led_module);
