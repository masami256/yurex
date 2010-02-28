#include <iostream>
#include <libusb-1.0/libusb.h>

#define YUREX_VENDOR_ID 0x0c45
#define YUREX_PRODUCT_ID 0x1010

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
     ret = libusb_init(NULL);

     if (ret < 0) 
	  throw "libusb_init was failed";

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

     return 0;
}
