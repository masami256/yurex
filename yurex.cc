#include <iostream>
#include <libusb-1.0/libusb.h>

#define YUREX_VENDOR_ID 0x0c45
#define YUREX_PRODUCT_ID 0x1010

// Those commands come from OpenBSD driver.
// Thanks to @yojiro san.
#define CMD_NONE       0xf0
#define CMD_EOF        0x0d
#define CMD_ACK        0x21
#define CMD_MODE       0x41 /* XXX */
#define CMD_VALUE      0x43
#define CMD_READ       0x52
#define CMD_WRITE      0x53
#define CMD_PADDING    0xff

enum {
     Yurex_not_open = 0,
     Yurex_open,
};

class Yurex
{
public:
     Yurex();
     ~Yurex();

     bool findYurex();
     void showYurexInfo();

     bool openYurex();

     void findEndPoint();
     bool detachKernelDriver();
     bool claimToYurex();
     bool releaseInterface();

     bool readDataASync();
     bool writeDataASync();

private:
     struct libusb_device_descriptor descriptor_;
     void setDescriptor(struct libusb_device_descriptor descriptor) { descriptor_ = descriptor; }
     struct libusb_device_descriptor *getDescriptor() { return &descriptor_; }

     libusb_device *device_;
     libusb_device *getDevice() { return device_; }
     void setDevice(libusb_device *device) { device_ = device; }

     libusb_device **devices_;
     void setDevices(libusb_device **devices) { devices_ = devices; }
     libusb_device **getDevices() { return devices_; }

     libusb_device_handle *handle_;
     void setHandle(libusb_device_handle *handle) { handle_ = handle; }
     libusb_device_handle *getHandle() { return handle_; }

     libusb_endpoint_descriptor const *endpoint_;
     void setEndPoint(const libusb_endpoint_descriptor *endpoint) { endpoint_ = endpoint; }
     const libusb_endpoint_descriptor *getEndPoint() { return endpoint_; }

     libusb_config_descriptor *config_;
     void setConfig(libusb_config_descriptor *config) { config_ = config; }
     libusb_config_descriptor *getConfig() { return config_; }

     // if Yurex device was opened successfully, it should be set "Yurex_open".
     bool dev_state;
     void init() throw (const char *);
     int findDevices() throw (const char *);

     bool checkYurexDevice() throw (const char *);
     void clearDeviceList();

};

Yurex::Yurex()
{
     dev_state = Yurex_not_open;
     devices_ = NULL;
     handle_ = NULL;
     config_ = NULL;
}

Yurex::~Yurex()
{
     clearDeviceList();

     if (handle_ != NULL)
	  libusb_close(handle_);

     if (dev_state == Yurex_open)
	  libusb_exit(NULL);
}

void Yurex::clearDeviceList()
{
     if (devices_) 
	  libusb_free_device_list(devices_, 1);
     devices_ = NULL;
}

void Yurex::init() throw (const char *)
{
     int ret;
     libusb_context *ctx = NULL;

     ret = libusb_init(&ctx);

     if (ret < 0) 
	  throw "libusb_init was failed";

     libusb_set_debug(ctx, 3);

}

int Yurex::findDevices() throw (const char *)
{
     int cnt = 0;
     libusb_device **devs;

     cnt = libusb_get_device_list(NULL, &devs);
     if (cnt < 0)
	  throw "There are no USB devices";
     
     setDevices(devs);

     return cnt;
}

bool Yurex::checkYurexDevice() throw (const char *)
{
     libusb_device *dev;
     libusb_device **devs = getDevices();
     int i = 0;
     bool ret = false;
     
     while ((dev = devs[i++]) != NULL) {
	  struct libusb_device_descriptor desc;
	  int r = libusb_get_device_descriptor(dev, &desc);
	  if (r < 0) 
	       throw ("failed to get device descriptor");

	  if (desc.idVendor == YUREX_VENDOR_ID &&
	      desc.idProduct == YUREX_PRODUCT_ID) {
	       setDescriptor(desc);
	       setDevice(dev);
	       ret = true;
	       break;
	  }
     }

     return ret;
}

bool Yurex::findYurex()
{
     bool ret = false;
     try {
	  init();
	  findDevices();

	  ret = checkYurexDevice();

     } catch (const char *msg) {
	  std::cout << msg << std::endl;
     }
     return ret;
}

bool Yurex::openYurex()
{
     libusb_device_handle *h;

     h = libusb_open_device_with_vid_pid(NULL, 
					 getDescriptor()->idVendor, 
					 getDescriptor()->idProduct);
     if (h)
	  setHandle(h);
     
     clearDeviceList();

     return h ? true : false;
}

void Yurex::showYurexInfo()
{
     std::cout << "Yurex info" << std::endl << std::hex << std::showbase 
	       << "Vendor: " << getDescriptor()->idVendor << std::endl
	       << "Product: " << getDescriptor()->idProduct << std::endl;
//	       << "Bus: " << libusb_get_bus_number(getDevice()) << std::endl
//	       << "Device:" << libusb_get_device_address(getDevice()) << std::endl;

}

bool Yurex::detachKernelDriver()
{
     libusb_device_handle *handle = getHandle();
     bool ret = true;

     //Detach driver if kernel driver is attached.
     if (libusb_kernel_driver_active(handle, 0) == 1) { 
	  std::cout<< "Kernel Driver Active" << std::endl;
	  
	  if(libusb_detach_kernel_driver(handle, 0) == 0) {
	       std::cout<< "Kernel Driver Detached!" << std::endl;
	       ret = true;
	  } else {
	       ret = false;
	  }
     }

     return ret;
}

bool Yurex::claimToYurex()
{
     libusb_device_handle *handle = getHandle();
     int ret;

     //claim interface 0 (the first) of device (mine had jsut 1)
     ret = libusb_claim_interface(handle, 0); 
     if(ret < 0) 
	  std::cout << "Cannot Claim Interface" << std::endl;

     std::cout << "ret is " << ret << std::endl;

     return ret < 0? false : true;
}

void Yurex::findEndPoint()
{
     libusb_config_descriptor *config = getConfig();
     const libusb_interface_descriptor *interdesc;
     const libusb_endpoint_descriptor *epdesc;
     const libusb_interface *inter;

     libusb_get_config_descriptor(getDevice(), 0, &config);

     std::cout << "find endpoint" << std::endl;

     // Actually, yulex may only have one endpoint.
     for(int i = 0; i < (int) config->bNumInterfaces; i++) {
	  inter = &config->interface[i];

	  std::cout << "Number of alternate settings: "<< inter->num_altsetting << " | ";

	  for(int j = 0; j < inter->num_altsetting; j++) {
	       interdesc = &inter->altsetting[j];

	       std::cout << "Interface Number: " << (int) interdesc->bInterfaceNumber << " | ";
	       std::cout << "Number of endpoints: "<< (int) interdesc->bNumEndpoints << " | ";

	       for(int k = 0; k < (int) interdesc->bNumEndpoints; k++) {
		    epdesc = &interdesc->endpoint[k];
		    std::cout << "Descriptor Type: " << (int) epdesc->bDescriptorType << " | ";
		    std::cout <<"EP Address: " << (int) epdesc->bEndpointAddress << " | " << std::endl;
		    setEndPoint(epdesc);
	       }
	  }
     }

}

bool Yurex::releaseInterface()
{
     int ret;
     bool b = true;

     libusb_free_config_descriptor(getConfig());

     ret = libusb_release_interface(getHandle(), 0); //release the claimed interface
     if(ret != 0) {
	  std::cout << "Cannot Release Interface" << std::endl;
	  b = false;
     }

     return b;
}

bool Yurex::readDataASync()
{
     unsigned char data[8] = { CMD_PADDING };
     int ret;
     int actual = 0;

     data[0] = CMD_READ;
     data[1] = CMD_EOF;
     
     ret = libusb_bulk_transfer(getHandle(), getEndPoint()->bEndpointAddress, data, sizeof(data), &actual, 2000);
     std::cout << "ret is " << ret << " : actual is " << actual << std::endl;
     if(ret < 0) {
	  std::cout << "Reading Error" << std::endl;
     } else { 
	  std::cout << "Reading Successful!" << std::endl;
	  for (int i = 0; i < sizeof(data); i++)
	       std::cout << std::hex << std::showbase << (int) data[i] << ":";
	  std::cout << std::endl;
     }


     return true;
}

bool Yurex::writeDataASync()
{
     unsigned char data[8] = { CMD_PADDING };
     int ret;
     int actual = 0;
     
     data[0] = CMD_MODE;
     data[1] = 0;
     data[2] = CMD_EOF;

     ret = libusb_bulk_transfer(getHandle(), (getEndPoint()->bEndpointAddress | LIBUSB_ENDPOINT_OUT), data, sizeof(data), &actual, 2 * 1000);
     std::cout << "ret is " << ret << " : actual is " << actual << std::endl;
     if(ret == 0 && actual == sizeof(data)) 
	  std::cout << "Writing Successful!" << std::endl;
     else
	  std::cout << "Write Error" << std::endl;

     return true;
}

int main(int argc, char **argv)
{
     Yurex op;

     if (!op.findYurex()) {
	  std::cout << "There is no Yurex" << std::endl;
	  return -1;
     }

     op.showYurexInfo();

     if (!op.openYurex()) {
	  std::cout << "Cannot open Yurex device" << std::endl;
	  return -1;
     }

     std::cout << "Open Yurex device is success" << std::endl;

     if (!op.detachKernelDriver()) {
	  std::cout << "Cannot detach kernel driver" << std::endl;
	  return -1;
     }

     std::cout << "Claim to Yurex device" << std::endl;

     if (!op.claimToYurex())
	  return -1;

     std::cout << "Start search endpoint" << std::endl;

     op.findEndPoint();
     
     op.writeDataASync();

     op.readDataASync();

     if (!op.releaseInterface())
	  return -1;

     
     std::cout << "Done." << std::endl;
     return 0;
}
