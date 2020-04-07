#include <linux/usb.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ANTON");
MODULE_DESCRIPTION("Simple USB driver");
MODULE_VERSION("0.1");

static struct usb_device_id dev_ids[] = {
	{.driver_info = 42},
	{}
};

static struct usb_driver skel_driver = {
	.name = "Example USB driver",
	.id_table = dev_ids,
	.probe = NULL,
	.disconnect = NULL,
};

static int __init usb_init(void)
{
	int err;
	printk(KERN_ALERT "USB driver loaded into kernel tree\n");
	//Register this driver with the USB subsystem
	err = usb_register(&skel_driver);
	if(err)
		printk(KERN_ALERT "usb_register failed. Error number %d\n", err);
	return err;
}

static void __exit usb_exit(void)
{
	//Unregister this driver from the USB subsystem
	usb_deregister(&skel_driver);
}

module_init(usb_init);
module_exit(usb_exit);
