#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/mm.h>

#include <mydev.h>

MODULE_DESCRIPTION("My kernel module - mykmod");
MODULE_AUTHOR("maruthisi.inukonda [at] gmail.com");
MODULE_LICENSE("GPL");

// Dynamically allocate major no
#define MYKMOD_MAX_DEVS 256
#define MYKMOD_DEV_MAJOR 0

static int mykmod_init_module(void);
static void mykmod_cleanup_module(void);

static int mykmod_open(struct inode *inode, struct file *filp);
static int mykmod_close(struct inode *inode, struct file *filp);
static int mykmod_mmap(struct file *filp, struct vm_area_struct *vma);

module_init(mykmod_init_module);
module_exit(mykmod_cleanup_module);

static struct file_operations mykmod_fops = {
	.owner = THIS_MODULE,	/* owner (struct module *) */
	.open = mykmod_open,	/* open */
	.release = mykmod_close,	/* release */
	.mmap = mykmod_mmap,	/* mmap */
};

static void mykmod_vm_open(struct vm_area_struct *vma);
static void mykmod_vm_close(struct vm_area_struct *vma);
//static int mykmod_vm_fault(struct vm_fault *vmf);
static int mykmod_vm_fault(struct vm_area_struct *vma, struct vm_fault *vmf);

// Data-structure to keep per device(per inode) info 
struct mykmod_dev_info {
	char *data;		// 1 MB memory
	size_t size;
};

// Device table data-structure to keep all devices
static struct mykmod_dev_info *devices[MYKMOD_MAX_DEVS];
static int no_of_devices = 0;

// Data-structure to keep per VMA info 
struct mykmod_vma_info {
	struct mykmod_dev_info *dev_info;	// the underlying device info
	unsigned long npagefaults;	// number of page faults in this VMA
};

static const struct vm_operations_struct mykmod_vm_ops = {
	.open = mykmod_vm_open,
	.close = mykmod_vm_close,
	.fault = mykmod_vm_fault
};

static int mykmod_major;

static int mykmod_init_module(void)
{
	printk("mykmod loaded\n");
	printk("mykmod initialized at=%p\n", init_module);

	if ((mykmod_major =
	     register_chrdev(MYKMOD_DEV_MAJOR, "mykmod", &mykmod_fops)) < 0) {
		printk(KERN_WARNING "Failed to register character device\n");
		return 1;
	} else {
		printk("register character device %d\n", mykmod_major);
	}

	return 0;
}

static void mykmod_cleanup_module(void)
{
	int i;
	printk("mykmod unloaded\n");
	unregister_chrdev(mykmod_major, "mykmod");
	//free device info structures from device.
	for (i = 0; i < no_of_devices; i++) {
		kfree(devices[i]->data);
		kfree(devices[i]);
	}
	return;
}

static int mykmod_open(struct inode *inodep, struct file *filep)
{
	// Allocate memory for devinfo and store in device table and i_private.
	// When the inode is opened for the first time, allocate 1MB kernel memory for the device
	if (inodep->i_private == NULL) {
		struct mykmod_dev_info *info;
		info = kmalloc(sizeof(struct mykmod_dev_info), GFP_KERNEL);
		info->data = (char *)kzalloc(MYDEV_LEN, GFP_KERNEL);
		info->size = MYDEV_LEN;
		inodep->i_private = info;

		// Update devices table
		devices[no_of_devices++] = info;
	}
	// Store device info in file's private_data as well
	filep->private_data = inodep->i_private;

	printk("mykmod_open: filep=%p f->private_data=%p "
	       "inodep=%p i_private=%p i_rdev=%x maj:%d min:%d\n",
	       filep, filep->private_data,
	       inodep, inodep->i_private, inodep->i_rdev, MAJOR(inodep->i_rdev),
	       MINOR(inodep->i_rdev));

	return 0;
}

static int mykmod_close(struct inode *inodep, struct file *filep)
{
	printk("mykmod_close: inodep=%p filep=%p\n", inodep, filep);
	return 0;
}

static int mykmod_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct mykmod_vma_info *vm_pvt_data;
	// Check if VMA would lie inside the file completely
	if ((vma->vm_pgoff << PAGE_SHIFT) + (vma->vm_end - vma->vm_start) >
	    MYDEV_LEN) {
		printk("mykmod_mmap: Trying to mmap out of file bounds");
		return -EINVAL;
	}
	//setup vma's flags, save private data (devinfo, npagefaults) in vm_private_data
	vma->vm_ops = &mykmod_vm_ops;
	vma->vm_flags |= VM_DONTEXPAND | VM_DONTDUMP;

	printk("mykmod_mmap: filp=%p vma=%p flags=%lx\n", filp, vma,
	       vma->vm_flags);

	// Allocate per VMA info, and initialize page faults to 0.
	vm_pvt_data = kmalloc(sizeof(struct mykmod_vma_info), GFP_KERNEL);
	vm_pvt_data->dev_info = filp->private_data;
	vm_pvt_data->npagefaults = 0;
	vma->vm_private_data = vm_pvt_data;

	mykmod_vm_open(vma);
	return 0;
}

static void mykmod_vm_open(struct vm_area_struct *vma)
{
	struct mykmod_vma_info *vm_pvt_info = vma->vm_private_data;
	printk("mykmod_vm_open: vma=%p npagefaults:%lu\n", vma,
	       vm_pvt_info->npagefaults);
}

static void mykmod_vm_close(struct vm_area_struct *vma)
{
	struct mykmod_vma_info *vm_pvt_info = vma->vm_private_data;
	printk("mykmod_vm_close: vma=%p npagefaults:%lu\n", vma,
	       vm_pvt_info->npagefaults);
	vm_pvt_info->npagefaults = 0;
	// Free the vma info data structure
	kfree(vm_pvt_info);
}

static int mykmod_vm_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	struct mykmod_vma_info *vma_prvt_data = vma->vm_private_data;
	struct mykmod_dev_info *filp_pvt_data = vma_prvt_data->dev_info;
	char *dev_data = filp_pvt_data->data;

	// Find the page * for the fault page using offsets.
	unsigned long offset = (vma->vm_pgoff + vmf->pgoff) << PAGE_SHIFT;
	unsigned long physaddr = virt_to_phys(dev_data) + offset;
	unsigned long pfn = physaddr >> PAGE_SHIFT;

	// Convert the page frame to struct page and store it in vmf
	vmf->page = pfn_to_page(pfn);

	// Keeps the reference count of mapped pages
	get_page(vmf->page);

	// Increment number of page faults
	vma_prvt_data->npagefaults++;

	printk("mykmod_vm_fault: vma=%p vmf=%p pgoff=%lu page=%p\n", vma, vmf,
	       vmf->pgoff, vmf->page);
	return 0;
}
