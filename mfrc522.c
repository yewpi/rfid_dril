#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/of_device.h>
#include <linux/mod_devicetable.h>
#include <linux/cdev.h>
#include <linux/ioctl.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/gpio/driver.h>
#include <linux/gpio/consumer.h>
#include <linux/init.h>

#define BUFFER_SIZE  64
#define IOCTL_HELLO 0
#define IOCTL_READ 1
#define IOCTL_WRITE 2

MODULE_AUTHOR("Hamdi Abdallah & Maxime Mehio");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("A MFRC522 module.");

static int     my_open( struct inode *, struct file * );
static ssize_t my_read( struct file * ,        char *  , size_t, loff_t *);
static ssize_t my_write(struct file * , const  char *  , size_t, loff_t *);
static int my_release(struct inode *, struct file * );
long ioctl_dispatch(struct file *filp,unsigned int cmd, unsigned long arg);

struct spi_device *spi_mfrc522;

struct file_operations fops = {
open : my_open,
       read : my_read,
       write : my_write,
       unlocked_ioctl : ioctl_dispatch,
       release : my_release,
       owner : THIS_MODULE
};

struct dev {
	struct cdev cdev;
	char *msg;
};
static struct dev *dev;
static int major;

static struct class *cl;
static dev_t devno;

static int cleanup(int err);

int mfrc522_probe(struct spi_device *spi_dev) {
	int err;
	spi_mfrc522 = spi_dev;
	pr_info("Hello, card reader!\n");
	if ((alloc_chrdev_region(&devno, 0, 1, "mfrc522")) < 0)
		return -1;
	major = MAJOR(devno);
	if ((cl = class_create(THIS_MODULE, "mfrc522")) == NULL)
		return cleanup(1);
	if (device_create(cl, NULL, devno, NULL, "mfrc522") == NULL)
		return 	cleanup(2);
	dev = kmalloc(sizeof(struct dev), GFP_KERNEL);
	if (!dev)
		return 	cleanup(3);
	dev->msg = (char *)kmalloc(BUFFER_SIZE, GFP_KERNEL);
	if (!dev->msg)
		return 	cleanup(4);
	dev->cdev.owner = THIS_MODULE;
	cdev_init(&dev->cdev, &fops);
	register_chrdev_region(devno, 1 , "mfrc522");
	err = cdev_add(&dev->cdev, devno, 1);
	if (err < 0)
		return 	cleanup(5);
	return 0;
}

static int cleanup(int err) {
	if (err > 4 )
		device_destroy(cl, devno);
	if (err > 3)
		kfree(dev->msg);
	if (err > 2 )
		kfree(dev);
	if (err > 1)
		class_destroy(cl);
	unregister_chrdev_region(devno, 1);
	pr_err("Error initializing module.\n");
	return -1;
}

int mfrc522_remove(struct spi_device *spi_dev) {
	spi_mfrc522 = spi_dev;
	pr_info("Remove!\n");

	kfree(dev->msg);	

	cdev_del(&dev->cdev);
	kfree(dev);

	device_destroy(cl, devno);
	class_destroy(cl);

	unregister_chrdev_region(devno, 1);
	return 0;
}

unsigned char readRawRC(struct spi_device *spi, unsigned char Address) {
	unsigned char ucAddr;
	unsigned char ucRes;
	int ret;
	ucRes = 0;
	ucAddr = ((Address << 1) & 0x7E) | 0x80;

	ret = spi_write_then_read(spi, &ucAddr, 1, &ucRes, 1);
	if (ret != 0) {
		printk("spi_write_then_read err = %d\n", ret);
	}
	return ucRes;

}

void writeRawRC(struct spi_device *spi, unsigned char Address, unsigned char value) {
	unsigned char ucAddr;
	struct spi_transfer st[2];
	struct spi_message msg;

	ucAddr = ((Address << 1) & 0x7E);

	spi_message_init(&msg);
	memset(st, 0, sizeof(st));

	st[0].tx_buf = &ucAddr;
	st[0].len = 1;
	spi_message_add_tail(&st[0], &msg);

	st[1].tx_buf = &value;
	st[1].len = 1;
	spi_message_add_tail(&st[1], &msg);

	spi_sync(spi, &msg);
}



/*
void get_buffer(char  *data) {
	char *command = kmalloc(sizeof(char) * 2, GFP_KERNEL);
	//buffer[]	
	command[0]  = CommandReg;
	command[1]  = 0;
	spi_write(spi_driver, command, 8);// CD_WriteRegister(spi,CommandReg, PCD_Idle);            // Stop any active command.
	command[0]  = FIFOLevelReg;
	command[1]  = 0x80;
	spi_write(spi_driver, command, 16);
	command[0]  = CommandReg + 1;
	spi_write(spi_driver, command, 8);
	command[0]  = FIFOLevelReg;
	command[1]  = 0;
	spi_write(spi_driver, command, 16);
	char *res = kmalloc(sizeof(char) * 1);
	spi_read(spi_driver, res, 8);
	int size = res;
	//add registre to read
	spi_read(spi_driver, data, size);

    	//PCD_WriteRegister(spi,ComIrqReg, 0x7F);                 // Clear all seven interrupt request bits
	//PCD_WriteRegister(spi,FIFOLevelReg, 0x80);	
}

void write_buffer(char  *data) {
	char *command = kmalloc(sizeof(char) * 2);
	//buffer[]	
	command[0]  = CommandReg;
	command[1]  = 0;
	spi_write(spi_driver, command, 8);// CD_WriteRegister(spi,CommandReg, PCD_Idle);            // Stop any active command.
	command[0]  =+ 1;
	spi_write(spi_driver, command, 8);
	command[0]  = FIFOLevelReg;
	command[1]  = 0x80;
	char *res = kmalloc(sizeof(char) * 1);
	spi_read(spi_driver, res, 8);
	int size = res;
	spi_read(spi_driver, command, size);

    	//PCD_WriteRegister(spi,ComIrqReg, 0x7F);                 // Clear all seven interrupt request bits
	//PCD_WriteRegister(spi,FIFOLevelReg, 0x80);	
}
*/
long ioctl_dispatch(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret=0;
	switch(cmd) {
		case IOCTL_HELLO:
			pr_info("Hello ioctl world\n");
			break;
		case IOCTL_READ:
			pr_info("ioctl read\n");
			break;
		case IOCTL_WRITE:
			pr_info("ioctl write\n");
			break;
	}
	return ret;
}

/*
 * file operation: OPEN
 * */
static int my_open(struct inode *inod, struct file *fil)
{
	int major;
	int minor;
	major = imajor(inod);
	minor = iminor(inod);
	printk("\n*****Some body is opening me at major %d  minor %d*****\n",major, minor);
	return 0;
}


/*
 * file operation: READ
 * */
static ssize_t my_read(struct file *filp, char *buff, size_t len, loff_t *off)
{	

	//int major, minor;
	short count;

	//major = MAJOR(filp->f_dentry->d_inode->i_rdev);
	//minor = MINOR(filp->f_dentry->d_inode->i_rdev);
	printk("FILE OPERATION READ:%d:%d\n", major, 0);//minor);

	//switch(minor){
	//	case 0:
	strcpy(dev->msg,"DATA FROM MODULE: minor : 0");
	//		break;
	//	default:
	//		len = 0;
	//}
	if (len < BUFFER_SIZE)
		count = copy_to_user( buff, dev->msg, len);
	else
		count = copy_to_user( buff, dev->msg, BUFFER_SIZE);
	return 0;
}


/*
 * file operation: WRITE
 * */
static ssize_t my_write(struct file *filp, const char *buff, size_t len, loff_t *off)
{

	//int major,minor;
	short count;

	memset(dev->msg, 0, BUFFER_SIZE);
	//major = MAJOR(filp->f_dentry->d_inode->i_rdev);
	//minor = MINOR(filp->f_dentry->d_inode->i_rdev);
	// -- copy the string from the user space program which open and write this device
	if (len < BUFFER_SIZE)
		count = copy_from_user( dev->msg, buff, len );
	else	
		count = copy_from_user( dev->msg, buff, BUFFER_SIZE );

	printk("FILE OPERATION WRITE:%d:%d\n",major, 0);//minor);
	printk("msg: %s", dev->msg);

	return len;
}


/*
 * file operation : CLOSE
 * */
static int my_release(struct inode *inod, struct file *fil)
{
	int minor;
	minor = 0;//MINOR(fil->f_dentry->d_inode->i_rdev);
	printk("*****Some body is closing me at major %d minor %d*****\n", major, minor);
	return 0;
}

static const struct of_device_id mfrc522_spi_id[] ={
	{ .compatible = "nxp,mfrc522" },
	{}
};
MODULE_DEVICE_TABLE(of, mfrc522_spi_id);

static struct spi_driver mfrc522_driver = {
	.driver = 
	{
		.name = ".spi_mfrc522",
		.owner = THIS_MODULE,
		.of_match_table = mfrc522_spi_id
	},
	.probe = mfrc522_probe,
	.remove = mfrc522_remove
};

module_spi_driver(mfrc522_driver);
