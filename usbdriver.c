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

static struct usb_device *device;
static struct usb_device_id dev_table[] = {
	{USB_DEVICE(0x05f9, 0xffff)}, //Vendor ID and product ID for some USB devices I had laying around
	{USB_DEVICE(0x067b, 0x2303)},
	{USB_DEVICE(0x195d, 0x1010)},
	{USB_DEVICE(0x04d9, 0xfa31)},
	{USB_DEVICE_AND_INTERFACE_INFO(0x0951, 0x1665, 8, 6, 0x50)},
	//{.match_flags = USB_DEVICE_ID_MATCH_DEVICE, .idVendor = 0x0951, .idProduct = 0x1665, .bInterfaceClass = 3},
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
	struct usb_host_interface *iface_desc;
	struct usb_endpoint_descriptor *endpoint;
	int i;

	iface_desc = interface->cur_altsetting;
	printk(KERN_ALERT "USB interface %d now probed: (%04X:%04X)\n", iface_desc->desc.bInterfaceNumber, id->idVendor, id->idProduct);	
	printk(KERN_ALERT "Number of endpoints: %02X\n", iface_desc->desc.bNumEndpoints);
	printk(KERN_ALERT "Interface class: %02X\n", iface_desc->desc.bInterfaceClass);

	for(i = 0; i < iface_desc->desc.bNumEndpoints; i++)
	{
		endpoint = &iface_desc->endpoint[i].desc;
		printk(KERN_ALERT "Endpoint[%d] address: 0x%02X\n", i, endpoint->bEndpointAddress);
		printk(KERN_ALERT "Endpoint[%d] attributes: 0x%02X\n", i, endpoint->bmAttributes);
		printk(KERN_ALERT "Endpoint[%d] max pkt size: 0x%04X\n", i, endpoint->wMaxPacketSize);
	}

	device = interface_to_usbdev(interface);
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
