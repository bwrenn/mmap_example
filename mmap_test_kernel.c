#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/mm.h>

#define DEV_MODULENAME "expdev"
#define DEV_CLASSNAME  "expdevclass"
static int majorNumber;
static struct class *devClass = NULL;
static struct device *devDevice = NULL;

#ifndef VM_RESERVED
# define  VM_RESERVED   (VM_DONTEXPAND | VM_DONTDUMP)
#endif

static void
mem_dump (const uint8_t *addr, uint32_t len)
{
	int i;

	printk(" 0x%lX [ ", (unsigned long) addr);
	for (i = 0; i < 8; i++)
	{
		printk("0x%02X ", addr[i]);
	}
	printk("... ");
	for (i = len - 8; i < len; i++)
	{
		printk("0x%02X ", addr[i]);
	}
	printk("]\n");
}

struct mmap_info
{
	char *data;
	int reference;
};

static void
dev_vm_ops_open (struct vm_area_struct *vma)
{
	struct mmap_info *info;

	// counting how many applications mapping on this dataset
	info = (struct mmap_info *) vma->vm_private_data;
	info->reference++;
}

static void
dev_vm_ops_close (struct vm_area_struct *vma)
{
	struct mmap_info *info;

	info = (struct mmap_info *) vma->vm_private_data;
	info->reference--;
}

static int
dev_vm_ops_fault (struct vm_area_struct *vma, struct vm_fault *vmf)
{
	struct page *page;
	struct mmap_info *info;

	info = (struct mmap_info *) vma->vm_private_data;
	if (!info->data)
	{
		printk("No data\n");
		return 0;
	}

	//page = virt_to_page(info->data);
	page = virt_to_page((info->data) + (vmf->pgoff * PAGE_SIZE));

	get_page(page);
	vmf->page = page;

	return 0;
}

static const struct vm_operations_struct dev_vm_ops = { .open = dev_vm_ops_open, .close =
    dev_vm_ops_close, .fault = dev_vm_ops_fault, };

int
fops_mmap (struct file *filp, struct vm_area_struct *vma)
{
	vma->vm_ops = &dev_vm_ops;
	vma->vm_flags |= VM_RESERVED;
	vma->vm_private_data = filp->private_data;
	dev_vm_ops_open(vma);
	return 0;
}

int
fops_close (struct inode *inode, struct file *filp)
{
	struct mmap_info *info;
	info = filp->private_data;

	kfree(info->data);//free_page((unsigned long )info->data);
	kfree(info);
	filp->private_data = NULL;
	return 0;
}

#define MY_MMAP_LEN 0x10000

int
fops_open (struct inode *inode, struct file *p_file)
{
	struct mmap_info *info;
	char *data;
	int i;
	uint8_t val;

	info = kmalloc(sizeof(struct mmap_info), GFP_KERNEL);

	// allocating memory on the heap for the data
	data = kcalloc(0x10000, sizeof(char), GFP_KERNEL);
	if (data == NULL)
	{
		printk(KERN_ERR "insufficient memory\n");
		/* insufficient memory: you must handle this error! */
		return ENOMEM;
	}

	info->data = data;

	/*
	printk(KERN_INFO "  > ->data:          0x%16p\n", info->data);
	memcpy(info->data, "Initial entry on mapped memory by the kernel module", 52);
	memcpy((info->data) + 0xf00, "Somewhere", 9);
	memcpy((info->data) + 0xf000, "Somehow", 7);
	printk(KERN_INFO "  > ->data: %c%c%c\n", // the output here is correct
	    *(info->data + 0xf000 + 0), *(info->data + 0xf000 + 1), *(info->data + 0xf000 + 2));
	*/

	val = 0x0;
	for (i = 0; i < (MY_MMAP_LEN / PAGE_SIZE); i++)
	{
		memset(info->data + (i * PAGE_SIZE), val, PAGE_SIZE);
		mem_dump(info->data + (i * PAGE_SIZE), PAGE_SIZE);
		val++;
	}

	/* assign this info struct to the file */
	p_file->private_data = info;
	return 0;
}

static const struct file_operations dev_fops = { .open = fops_open, .release = fops_close, .mmap =
    fops_mmap, };

static int __init
_module_init (void)
{
	int ret = 0;

	// Try to dynamically allocate a major number for the device
	majorNumber = register_chrdev(0, DEV_MODULENAME, &dev_fops);
	if (majorNumber < 0)
	{
		printk(KERN_ALERT "Failed to register a major number.\n");
		return -EIO; // I/O error
	}

	// Register the device class
	devClass = class_create(THIS_MODULE, DEV_CLASSNAME);
	// Check for error and clean up if there is
	if (IS_ERR(devClass))
	{
		printk(KERN_ALERT "Failed to register device class.\n");
		ret = PTR_ERR(devClass);
		goto goto_unregister_chrdev;
	}

	// Create and register the device
	devDevice = device_create(devClass,
	NULL, MKDEV(majorNumber, 0),
	NULL,
	DEV_MODULENAME);

	// Clean up if there is an error
	if (IS_ERR(devDevice))
	{
		printk(KERN_ALERT "Failed to create the device.\n");
		ret = PTR_ERR(devDevice);
		goto goto_class_destroy;
	}
	printk(KERN_INFO "Module registered.\n");

	return ret;

	// Error handling - using goto
	goto_class_destroy: class_destroy(devClass);
	goto_unregister_chrdev: unregister_chrdev(majorNumber, DEV_MODULENAME);

	return ret;
}

static void __exit
_module_exit(void)
{
  device_destroy(devClass, MKDEV(majorNumber, 0));
  class_unregister(devClass);
  class_destroy(devClass);
  unregister_chrdev(majorNumber, DEV_MODULENAME);
  printk(KERN_INFO "Module unregistered.\n");
}

module_init(_module_init);
module_exit(_module_exit);
MODULE_LICENSE("GPL");
