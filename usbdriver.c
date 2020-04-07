#include <linux/input.h>
#include <linux/module.h>
#include <linux/init.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ANTON");
MODULE_DESCRIPTION("Simple USB driver");
MODULE_VERSION("0.1");

struct input_dev usb_dev; //input_dev is the structure containing data for the USB driver. From input.h.

static int __init usb_init(void)
{
	int err;
	printk(KERN_ALERT "USB driver loaded into kernel tree\n");
	//Descriptive labels for the USB device
	usb_dev.name = "Example USB device";
	usb_dev.phys = "Fake/Path";
	usb_dev.id.bustype = BUS_HOST;
	usb_dev.id.vendor = 0x0001;
	usb_dev.id.product = 0x0001;
	usb_dev.id.version = 0x0100;

	//Register the device with input core
	err = input_register_device(&usb_dev);
	return 0;
}

static void __exit usb_exit(void)
{
	//Unregister the device from input core
	input_unregister_device(&usb_dev);
}

module_init(usb_init);
module_exit(usb_exit);
