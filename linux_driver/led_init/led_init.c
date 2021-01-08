#include "linux/fs.h"
#include "linux/cdev.h"
#include "linux/module.h"
#include "linux/kernel.h"
#include "linux/device.h"
#include "asm/uaccess.h"
#include "asm/io.h"
#include "linux/slab.h"

// name of device
#define DEVICE_NAME "LEDS"

// num of leds
#define NUM_LEDS 4

//state of led
#define LED_ON	1
#define LED_OFF 0

/*----------------------------------------------------------------------------------*/
/*                                  EXPLAIN											 /
leds_dev[]: lưu trữ thông tin cho mỗi led
leds_fops: đăng ký các hàm qua driver inteface đc quy định theo chuẩn.
bên trong cấu trúc leds_dev có 1 biến cấu trúc cdev(struct cdev cdev), struct cdev là
kiểu cấu trúc lưu trữ thông tin của 1 tập tin lưu trữ trong kernel.

* cấu trúc file+operations: 
 - Cấu trúc lệnh của character driver (file_operations) là một cấu trúc dùng để liên
 kết những hàm chứa các thao tác của driver điều khiển thiết bị với những hàm chuẩn 
 trong hệ điều hành giúp giao tiếp giữa người lập trình ứng dụng với thiết bị vật lý.
 - Cấu trúc file_operation được định nghĩa trong thư viện <linux/fs.h>.
 Mỗi một tập tin thiết bị được mở trong hệ điều hành linux điều được hệ điều hành dành
 cho một vùng nhớ mô tả cấu trúc tập tin, trong cấu trúc tập tin có rất nhiều thông 
 tin liên quan phục vụ cho việc thao tác với tập tin đó. Một trong những thông tin này
 là file_operations, dùng mô tả những hàm mà driver thiết bị đang được mở.
*/
//per led structure
struct leds_device
{
	int number;
	int status;
	struct cdev cdev;
}leds_dev[NUM_LEDS];

//file operations structure
static struct file_operation leds_fops = {
	.owner	= THIS_MODULE,
	.open   = leds_open,
	.release	= leds_release,
	.read		= leds_read,
	.write		= leds_write,
};

//led driver major number
static dev_t leds_dev_number;



//driver initialization
int __init leds_init(void)
{
	int ret,i;

	/*int alloc_chrdev_region (dev_t *dev, unsigned int firstminor, unsigned int count, char *name);

	Hàm alloc_chrdev_region () cũng làm nhiệm vụ đăng ký định danh cho một thiết bị mới. Nhưng có một điểm khác biệt là số Major trong định danh không còn cố định nữa, số này do hệ thống linux tự động cấp vì thế sẽ không trùng với bất kỳ số định danh nào khác đã tồn tại. Bắt đầu phân tích từng tham số :

	+ Tham số thứ nhất của hàm dev_t *dev :  là con trỏ kiểu dev_t dùng để lưu trữ số định danh đầu tiên trong khoảng định danh được cấp nếu hàm thực hiện thành công .

	+ Tham số thứ hai  unsigned int first minor :  là số Minor đầu tiên của khoảng định danh muốn cấp .

	+ Tham số thứ ba unsigned int count :  là số lượng định danh muốn cấp, tính từ số Major được cấp động và số Minor unsigned int first minor;

	+ Tham số thứ tư char *name : là tên của driver thiết bị muốn đăng ký.*/
	// request device major number
	if((ret = alloc_chrdev_region(&leds_dev_number, 0, NUM_LEDS, DEVICE_NAME) <0))
	{
		printk(KERN_DEBUG "Error registering device!\n");
		return ret;
	}

	// init leds GPIO port
	initLedPort();

	// init each led device
	for(i = 0;i<NUM_LEDS;i++)
	{
		//init led status
		leds_dev[i].number = i+1;
		leds_dev[i].status = LED_OFF;

		// connect file operations to this device
		cdev_init(&leds_dev[i].cdev, &leds_fops);
		leds_dev[i].cdev.owner = THIS_MODULE;

		// connect major/minor number
		if((ret = cdev_add(&leds_dev[i].cdev, (leds_dev_number+i),1)))
		{
			printk(KERN_DEBUG "Error adding device\n");
			return ret;
		}

		//init led status
		changeLedStatus(leds_dev_number[i].number, LED_OFF);
	}

	printk("Led driver initialized\n");
	return 0;
		
}

//driver exit
void __exit leds_exit(void)
{
	int i;

	//delete devices
	for(i = 0; i< NUM_LEDS; i++)
	{
		cdev_del(&leds_dev_[i].cdev);
	}

	//release major number
	unregister_chrdev_region(leds_dev_number, NUM_LEDS);
	printk("Exiting leds driver\n");
}

module_init(leds_init);
module_exit(leds_exit);
MODULE_AUTHOR("txt");
MODULE_LICENSE("GPL");
