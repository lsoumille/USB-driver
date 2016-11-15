#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/usb.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

//Choose my minor
#define USB_SKEL_MINOR_BASE     192
#define DRIVER_SHORT    "driverusb"

//Driver specific stuff
struct driverusb {
  struct usb_interface *interface;
  struct usb_endpoint_descriptor *myInEndpoint;
};

static struct usb_driver usbdriver;

#define VENDOR_ID 0x06c2
#define PRODUCT_ID 0x0045

//Usb driver table to say the device that need this driver
static const struct usb_device_id usbdriver_table[] = {
  { USB_DEVICE(VENDOR_ID, PRODUCT_ID) },
  { }
};

MODULE_DEVICE_TABLE(usb, usbdriver_table);

//Print the content of the endpoint
static int my_show_function(struct file *file, char *buf, size_t count, loff_t *ppos)
{
  struct driverusb *dev;
  dev = file->private_data;
  if(*ppos == 0)
  {
    int len = 0;
    sprintf(buf + len,"Endpoint Descriptor :\n bLength :%d\n bDescriptorType: %d\n bEndpointAddress: 0x%x\n bmAttributes: %d\n wMaxPacketSize: %d\n bInterval: %d\n",
								dev->myInEndpoint->bLength,
								dev->myInEndpoint->bDescriptorType,
								dev->myInEndpoint->bEndpointAddress,
								dev->myInEndpoint->bmAttributes,
								dev->myInEndpoint->wMaxPacketSize,
								dev->myInEndpoint->bInterval);
     len += strlen(buf);
     *ppos += len;
	   return len;
  }
  return 0;
}

//Copy the dev data in the private data field
static int my_open_function(struct inode *inode, struct file *file)
{
  struct driverusb *dev;
  struct usb_interface *interface;
  interface = usb_find_interface(&usbdriver, iminor(inode));
  if (!interface)
  {
    return -ENODEV;
  }
  //Get dev info using usb interface
  dev = usb_get_intfdata(interface);
  if (!dev)
  {
    return -ENODEV;
  }
  file->private_data = dev;
  return 0;
}

//Open is important to avoid "No Such Device"
static const struct file_operations fops = {
  .owner = THIS_MODULE,
  .read = my_show_function,
  .open = my_open_function
};

static struct usb_class_driver skel_class = {
  .name = "usbdriver%d",
  .fops = &fops,
  .minor_base = USB_SKEL_MINOR_BASE
};

//Function executed when the device is pluged
static int usbdriver_probe(struct usb_interface *iface, const struct usb_device_id *id)
{
  struct driverusb *dev;
  int result;
  dev = kzalloc(sizeof(struct driverusb), GFP_KERNEL);
  //link dev the usb interface to retrieve data later
  usb_set_intfdata(iface, dev);
  result = usb_register_dev(iface, &skel_class);

  //V1
  printk("IN PROBE FUNCTION\n");
  //V1
  /* set up the endpoint information */
  struct usb_host_interface *iface_desc;
  iface_desc = iface->cur_altsetting;
  struct usb_endpoint_descriptor *endpoint;
  int cpt = 0;
  //Iterate through all the endpoints
  while (cpt < iface_desc->desc.bNumEndpoints)
  {
    endpoint = &iface_desc->endpoint[cpt].desc;
    if (usb_endpoint_is_int_in(endpoint))
    {
      dev->myInEndpoint = &iface_desc->endpoint[cpt].desc;
      break;
    }
    ++cpt;
  }
  return result;
}

//Function executed when the device is unpluged
static void usbdriver_disconnect(struct usb_interface *interface)
{
  struct driverusb *dev ;
  dev = usb_get_intfdata(interface);
  usb_deregister_dev(interface, &skel_class);
  usb_set_intfdata(interface, NULL);
  //V1
  printk("IN DISCONNECT FUNCTION\n");
  //V1
  //Free the dev struct using to store endpoints data
  kfree(dev);
}

static struct usb_driver usbdriver = {
  .name = DRIVER_SHORT,
  .probe = usbdriver_probe,
  .disconnect = usbdriver_disconnect,
  .id_table = usbdriver_table
};

module_usb_driver(usbdriver);

MODULE_LICENSE("GPL");
