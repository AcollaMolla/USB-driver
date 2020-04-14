#include <linux/usb.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ANTON");
MODULE_DESCRIPTION("Simple USB driver");
MODULE_VERSION("0.1");

static int dev_probe(struct usb_interface *interface, const struct usb_device_id *id);
static void dev_disconnect(struct usb_interface *interface);

static struct usb_device_id dev_table[] = {
	{USB_DEVICE(0x05f9, 0xffff)}, //Vendor ID and product ID for two USB devices I had laying around
	{USB_DEVICE(0x067b, 0x2303)},
	{USB_DEVICE(0x195d, 0x1010)},
	{USB_DEVICE(0x04d9, 0xfa31)},
	{}
};
MODULE_DEVICE_TABLE(usb, dev_table);

static struct usb_driver skel_driver = {
	.name = "usb-test",
	.id_table = dev_table,
	.probe = dev_probe,
	.disconnect = dev_disconnect
};

static int dev_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
	printk(KERN_ALERT "Probing USB device\n");
	return 0;
}

static void dev_disconnect(struct usb_interface *interface)
{
	printk(KERN_ALERT "USB disconnecting\n");
} 

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
