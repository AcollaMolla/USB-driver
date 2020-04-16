#include <linux/usb.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/usb/input.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ANTON");
MODULE_DESCRIPTION("Simple USB driver");
MODULE_VERSION("0.1");

static int dev_probe(struct usb_interface *interface, const struct usb_device_id *id);
static void dev_disconnect(struct usb_interface *interface);
static ssize_t usb_read(struct file *file, char __user *buffer, size_t count, loff_t *ppos);
static int usb_mouse_open(struct input_dev *dev);

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

struct usb_mouse {
	char name[128];
	char phys[64];
	struct usb_device *usbdev;
	struct input_dev *dev;
	struct urb *irq;
	signed char *data;
	dma_addr_t data_dma;	
};

static struct usb_driver skel_driver = {
	.name = "usb-test",
	.id_table = dev_table,
	.probe = dev_probe,
	.disconnect = dev_disconnect
};

static struct file_operations usbdriver_fops = {
	.owner = THIS_MODULE,
	.read = usb_read,
	.write = NULL,
	.open = NULL,
	.release = NULL,
};

static struct usb_class_driver usbdriver_class = {
	.name = "usb/usbtest%d",
	.fops = &usbdriver_fops,
	.minor_base = 192,
};

static void usb_mouse_irq(struct urb *urb)
{
	printk(KERN_ALERT "Mouse irq\n");
}

static int dev_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
	struct usb_host_interface *iface_desc;
	struct usb_endpoint_descriptor *endpoint;
	struct usb_mouse *mouse;
	struct input_dev *input_dev;
	int i, errno, interval = 5, pipe = 0, maxp = 5;

	device = interface_to_usbdev(interface);
	/*errno = usb_register_dev(interface, &usbdriver_class);

	if(errno)
	{
		printk(KERN_ALERT "Error creating a minor for this device\n");
		usb_set_intfdata(interface, NULL);
		return -1;
	}*/

	mouse = kzalloc(sizeof(struct usb_mouse), GFP_KERNEL);
	input_dev = input_allocate_device();
	
	if(!mouse)
		return -2;
	mouse->data = usb_alloc_coherent(device, 8, GFP_ATOMIC, &mouse->data_dma);
	if(!mouse->data)
		return -3;
	mouse->irq = usb_alloc_urb(0, GFP_KERNEL);
	if(!mouse->irq)
		return -4;
	mouse->usbdev = device;
	input_dev->open = usb_mouse_open;
	mouse->dev = input_dev;
	
	iface_desc = interface->cur_altsetting;
	printk(KERN_ALERT "USB interface %d now probed: (%04X:%04X)\n", iface_desc->desc.bInterfaceNumber, id->idVendor, id->idProduct);	
	printk(KERN_ALERT "Number of endpoints: %02X\n", iface_desc->desc.bNumEndpoints);
	printk(KERN_ALERT "Interface class: %02X\n", iface_desc->desc.bInterfaceClass);

	for(i = 0; i < iface_desc->desc.bNumEndpoints; i++)
	{
		endpoint = &iface_desc->endpoint[i].desc;
		interval = endpoint->bInterval;
		pipe = usb_rcvintpipe(device, endpoint->bEndpointAddress);
		maxp = usb_maxpacket(device, pipe, usb_pipeout(pipe));
		printk(KERN_ALERT "Endpoint[%d] address: 0x%02X\n", i, endpoint->bEndpointAddress);
		printk(KERN_ALERT "Endpoint[%d] attributes: 0x%02X\n", i, endpoint->bmAttributes);
		if(endpoint->bmAttributes == 3)
			printk(KERN_ALERT "Endpoint[%d] interval: 0x%02X\n",i, endpoint->bInterval);
		printk(KERN_ALERT "Endpoint[%d] max pkt size: 0x%04X\n", i, endpoint->wMaxPacketSize);
	}
	usb_fill_int_urb(mouse->irq, mouse->usbdev, pipe, mouse->data, (maxp > 8 ? 8 : maxp), usb_mouse_irq, mouse, interval);
	errno = input_register_device(mouse->dev);
	if(errno){
		printk(KERN_ALERT "Can't register device\n");
		return -1;
	}
	errno = register_chrdev(0, "mymouse", &usbdriver_fops);
	if(errno < 0)
		printk(KERN_ALERT "mymouse registration failed\n");
	else
		printk(KERN_ALERT "mymouse registration succeeded\n");
	return errno;	
}

static void dev_disconnect(struct usb_interface *interface)
{
	printk(KERN_ALERT "USB disconnecting\n");
	usb_deregister_dev(interface, NULL);
}

static int usb_mouse_open(struct input_dev *dev)
{
	struct usb_mouse *mouse;
	printk(KERN_ALERT "usb mouse open\n");
	mouse = input_get_drvdata(dev);
	printk(KERN_ALERT "Got +1\n");
	mouse->irq->dev = mouse->usbdev;
	printk(KERN_ALERT "Got +2\n");
	if(usb_submit_urb(mouse->irq, GFP_KERNEL))
	{
		printk(KERN_ALERT "Failed submiting urb\n");
	}
	return 0;
}

static ssize_t usb_read(struct file *file, char __user *buffer, size_t count, loff_t *ppos)
{
	printk(KERN_ALERT "Reading device...\n");
	return 0;
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
