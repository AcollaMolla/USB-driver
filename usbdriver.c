#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/usb/input.h>
#include <linux/hid.h>
#include <linux/fs.h>
#include <asm/segment.h>
#include <asm/uaccess.h>
#include <linux/buffer_head.h>
#include <linux/device.h>
#include <linux/cdev.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ANTON");
MODULE_DESCRIPTION("Simple USB driver");
MODULE_VERSION("0.1");

static int dev_probe(struct usb_interface *interface, const struct usb_device_id *id);
static void dev_disconnect(struct usb_interface *interface);
static ssize_t usb_read(struct file *file, char __user *buffer, size_t count, loff_t *ppos);
static int usb_mouse_open(struct input_dev *dev);
static void usb_mouse_close(struct input_dev *dev);

static int registered = 0;
static int current_data = 0;
static int MAJOR = 92;

static struct usb_device *device;
static struct usb_device_id dev_table[] = {
	{USB_DEVICE(0x05f9, 0xffff)}, //Vendor ID and product ID for some USB devices I had laying around
	{USB_DEVICE(0x067b, 0x2303)},
	{USB_DEVICE(0x195d, 0x1010)},
	{USB_DEVICE(0x04d9, 0xfa31)},
	{USB_DEVICE(0x046d, 0xc52b)},
	{USB_DEVICE(0x062a, 0x4182)},
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
	struct usb_mouse *mouse = urb->context;
	signed char *data = mouse->data;
	struct input_dev *dev = mouse->dev;
	int status;


	switch (urb->status) {
	case 0:			/* success */
		break;
	case -ECONNRESET:	/* unlink */
	case -ENOENT:
	case -ESHUTDOWN:
		return;
	/* -EPIPE:  should clear the halt */
	default:		/* error */
		goto resubmit;
	}


	input_sync(dev);
resubmit:
	status = usb_submit_urb (urb, GFP_ATOMIC);
	if (status)
		dev_err(&mouse->usbdev->dev,
			"can't resubmit intr, %s-%s/input0, status %d\n",
			mouse->usbdev->bus->bus_name,
			mouse->usbdev->devpath, status);
	
	printk(KERN_ALERT "data[0]=%d\n", data[0]);
	printk(KERN_ALERT "data[1]=%d\n", data[1]);
	printk(KERN_ALERT "data[2]=%d\n", data[2]);
	printk(KERN_ALERT "data[3]=%d\n", data[3]);
	printk(KERN_ALERT "data[4]=%d\n", data[4]);
	printk(KERN_ALERT "data[5]=%d\n", data[5]);
	printk(KERN_ALERT "data[6]=%d\n", data[6]);
	printk(KERN_ALERT "data[7]=%d\n", data[7]);
	current_data = data[0];		
		
	//check which button pressed
	if(data[1] & 0x01){
		pr_info("Left mouse button clicked!\n");
		
	}
	else if(data[1] & 0x02){
		pr_info("Right mouse button clicked!\n");
	}
	else if(data[1] & 0x04){
		pr_info("Wheel button clicked!\n");
	}
	else if(data[5]){
		pr_info("Wheel moves!\n");
	}
}

static int dev_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	
	struct usb_device *dev = interface_to_usbdev(intf);
	struct usb_host_interface *interface;
	struct usb_endpoint_descriptor *endpoint;
	struct usb_mouse *mouse;
	struct input_dev *input_dev;
	int pipe, maxp;
	int error = -ENOMEM;
	int t;
	
	interface = intf->cur_altsetting;
	
	printk(KERN_ALERT "USB interface %d now probed: (%04X:%04X)\n", interface->desc.bInterfaceNumber, id->idVendor, id->idProduct);	
	printk(KERN_ALERT "Number of endpoints: %02X\n", interface->desc.bNumEndpoints);
	printk(KERN_ALERT "Interface class: %02X\n", interface->desc.bInterfaceClass);

	if (interface->desc.bNumEndpoints != 1)
		return -ENODEV;

	endpoint = &interface->endpoint[0].desc;
	if (!usb_endpoint_is_int_in(endpoint))
		return -ENODEV;

	pipe = usb_rcvintpipe(dev, endpoint->bEndpointAddress);
	maxp = usb_maxpacket(dev, pipe, usb_pipeout(pipe));

	mouse = kzalloc(sizeof(struct usb_mouse), GFP_KERNEL);
	input_dev = input_allocate_device();
	if (!mouse || !input_dev)
		goto fail1;

	mouse->data = usb_alloc_coherent(dev, 8, GFP_ATOMIC, &mouse->data_dma);
	if (!mouse->data)
		goto fail1;

	mouse->irq = usb_alloc_urb(0, GFP_KERNEL);
	if (!mouse->irq)
		goto fail2;

	mouse->usbdev = dev;
	mouse->dev = input_dev;

	if (dev->manufacturer)
		strlcpy(mouse->name, dev->manufacturer, sizeof(mouse->name));

	if (dev->product) {
		if (dev->manufacturer)
			strlcat(mouse->name, " ", sizeof(mouse->name));
		strlcat(mouse->name, dev->product, sizeof(mouse->name));
	}

	if (!strlen(mouse->name))
		snprintf(mouse->name, sizeof(mouse->name),
			 "USB HIDBP Mouse %04x:%04x",
			 le16_to_cpu(dev->descriptor.idVendor),
			 le16_to_cpu(dev->descriptor.idProduct));

	usb_make_path(dev, mouse->phys, sizeof(mouse->phys));
	strlcat(mouse->phys, "/input0", sizeof(mouse->phys));

	input_dev->name = mouse->name;
	input_dev->phys = mouse->phys;
	usb_to_input_id(dev, &input_dev->id);
	input_dev->dev.parent = &intf->dev;

	input_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REL);
	input_dev->keybit[BIT_WORD(BTN_MOUSE)] = BIT_MASK(BTN_LEFT) |
		BIT_MASK(BTN_RIGHT) | BIT_MASK(BTN_MIDDLE);
	input_dev->relbit[0] = BIT_MASK(REL_X) | BIT_MASK(REL_Y);
	input_dev->keybit[BIT_WORD(BTN_MOUSE)] |= BIT_MASK(BTN_SIDE) |
		BIT_MASK(BTN_EXTRA);
	input_dev->relbit[0] |= BIT_MASK(REL_WHEEL);

	input_set_drvdata(input_dev, mouse);

	input_dev->open = usb_mouse_open;
	input_dev->close = usb_mouse_close;

	usb_fill_int_urb(mouse->irq, dev, pipe, mouse->data,
			 (maxp > 8 ? 8 : maxp),
			 usb_mouse_irq, mouse, endpoint->bInterval);
	mouse->irq->transfer_dma = mouse->data_dma;
	mouse->irq->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

	error = input_register_device(mouse->dev);
	if (error)
		goto fail3;

	usb_set_intfdata(intf, mouse);
	

	
	//register device
	t = register_chrdev(MAJOR, "mymouse", &usbdriver_fops);
	if(t<0) 
	{
		pr_info("mymouse registration failed\n");
		registered = 0;
	}
	else 
	{
		pr_info("mymouse registration successful\n");
		registered = 1;
	}
	return t;

fail3:	
	usb_free_urb(mouse->irq);
fail2:	
	usb_free_coherent(dev, 8, mouse->data, mouse->data_dma);
fail1:	
	input_free_device(input_dev);
	kfree(mouse);
	return error;


	
}

static void dev_disconnect(struct usb_interface *interface)
{
	printk(KERN_ALERT "USB disconnecting\n");
	usb_deregister_dev(interface, NULL);
	if(registered)
		unregister_chrdev(MAJOR, "mymouse");
	registered = 0;
}

static int usb_mouse_open(struct input_dev *dev)
{
	struct usb_mouse *mouse;
	printk(KERN_ALERT "usb mouse open\n");
	mouse = input_get_drvdata(dev);
	if(!mouse)
	{
		printk(KERN_ALERT "no mouse\n");
		return -1;
	}
	if(!mouse->usbdev){
		printk(KERN_ALERT "no mouse->usbdev\n");
		return -1;
	}
	if(!mouse->irq){
		printk(KERN_ALERT "No mouse->irq\n");
		return -1;
	}
	printk(KERN_ALERT "Got +1\n");
	mouse->irq->dev = mouse->usbdev;
	printk(KERN_ALERT "Got +2\n");
	if(usb_submit_urb(mouse->irq, GFP_KERNEL))
	{
		printk(KERN_ALERT "Failed submiting urb\n");
	}
	return 0;
}

static void usb_mouse_close(struct input_dev *dev)
{
	struct usb_mouse *mouse = input_get_drvdata(dev);

	usb_kill_urb(mouse->irq);
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
