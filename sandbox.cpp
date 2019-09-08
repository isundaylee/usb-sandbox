#include <iostream>
#include <string>

#include <stdio.h>

#include "libusb.h"

std::string REPORT_ITEM_TYPES[] = {"main", "global", "local", "reserved"};

std::string REPORT_ITEM_GLOBAL_TYPES[] = {
    "usage page",
    "logical minimum",
    "logical maximum",
    "physical minimum",
    "physical maximum",
    "unit exponent",
    "unit",
    "report size",
    "report id",
    "report count",
    "push",
    "pop",
    "reserved",
    "reserved",
    "reserved",
    "reserved",
};

std::string REPORT_ITEM_LOCAL_TYPES[] = {
    "usage",
    "usage minimum",
    "usage maximum",
    "designator index",
    "designator minimum",
    "designator maximum",
    "!!!!!!",
    "string index",
    "string minimum",
    "string maximum",
    "delimiter",
    "reserved",
    "reserved",
    "reserved",
    "reserved",
    "reserved",
    "reserved",
};

void check_error(int code, std::string action) {
  if (code < 0) {
    std::cout << "Failed to " << action << ": " << code << std::endl;
    exit(code);
  }
}

std::string get_collection_string(int type) {
  switch (type) {
  case 0x00:
    return "physical";
  case 0x01:
    return "application";
  case 0x02:
    return "logical";
  case 0x03:
    return "report";
  case 0x04:
    return "named array";
  case 0x05:
    return "usage switch";
  case 0x06:
    return "usage modifier";
  }

  return "!!!!!!";
}

std::string get_global_string(int type) {
  switch (type) {
  case 0x00:
    return "physical";
  case 0x01:
    return "application";
  case 0x02:
    return "logical";
  case 0x03:
    return "report";
  case 0x04:
    return "named array";
  case 0x05:
    return "usage switch";
  case 0x06:
    return "usage modifier";
  }

  return "!!!!!!";
}

std::string make_hex_string(unsigned char *cur, size_t len) {
  char buf[3];
  std::string result = "";
  for (size_t i = 0; i < len; i++) {
    sprintf(buf, "%02x", cur[i]);
    result += " " + std::string(buf);
  }
  return result;
}

int main(void) {
  libusb_device **devs;
  libusb_context *context;
  int r;
  unsigned char buf[1024];
  ssize_t cnt;

  r = libusb_init(&context);
  check_error(r, "initialize libusb");

  libusb_device_handle *devh =
      // libusb_open_device_with_vid_pid(context, 0x1532, 0x0016);
      libusb_open_device_with_vid_pid(context, 0xfeed, 0x1307);
  libusb_device *dev = libusb_get_device(devh);
  if (devh == NULL) {
    std::cout << "Failed to open device." << std::endl;
    return 1;
  }

  libusb_config_descriptor *configd;
  r = libusb_get_active_config_descriptor(dev, &configd);
  check_error(r, "get active config descriptor");

  for (size_t i = 0; i < configd->bNumInterfaces; i++) {
    auto interfaced = &configd->interface[i];

    printf("Interface: \n");
    for (size_t j = 0; j < interfaced->num_altsetting; j++) {
      auto altsetting = &interfaced->altsetting[j];
      printf("    Alt setting: interface class %d, extra length %u\n",
             altsetting->bInterfaceClass, altsetting->extra_length);

      printf("        Extra:");
      for (size_t k = 0; k < altsetting->extra_length; k++) {
        printf(" 0x%02x", altsetting->extra[k]);
      }
      printf("\n");

      for (size_t k = 0; k < altsetting->bNumEndpoints; k++) {
        auto endpoint = &altsetting->endpoint[k];

        printf("        Endpoint: \n");
      }
    }
  }

  r = libusb_control_transfer(
      devh, LIBUSB_ENDPOINT_IN | LIBUSB_RECIPIENT_INTERFACE,
      LIBUSB_REQUEST_GET_DESCRIPTOR, (LIBUSB_DT_REPORT << 8) | 0, 0, buf,
      sizeof(buf), 5000);
  check_error(r, "control transfer");

  auto cur = buf;
  while (cur < buf + r) {
    if (cur[0] == 0b11111110) {
      printf("LONG!!!\n");
      exit(1);
    }

    unsigned char tag = (cur[0] & 0b11110000) >> 4;
    unsigned char type = (cur[0] & 0b00001100) >> 2;
    unsigned char size = (cur[0] & 0b00000011) >> 0;

    std::string tag_string = "";

    if (type == 0b00) {
      switch (tag) {
      case 0b1000:
        tag_string = "input";
        break;
      case 0b1001:
        tag_string = "output";
        break;
      case 0b1011:
        tag_string = "feature";
        break;
      case 0b1010:
        tag_string = "col (" + get_collection_string(cur[1]) + ")";
        break;
      case 0b1100:
        tag_string = "end col";
        break;
      }
    } else if (type == 0b01) {
      tag_string = REPORT_ITEM_GLOBAL_TYPES[tag];
    } else if (type == 0b10) {
      tag_string = REPORT_ITEM_LOCAL_TYPES[tag];
    }

    printf("%6s: (size %d) %20s: %10s %s\n", REPORT_ITEM_TYPES[type].c_str(),
           size, tag_string.c_str(),
           make_hex_string(&cur[1], (size == 0b11 ? 4 : size)).c_str(),
           type == 0b00 ? "----------------------------" : "");

    cur += 1 + (size == 0b11 ? 4 : size);
  }

  printf("Descriptor:");
  for (size_t i = 0; i < r; i++) {
    printf(" 0x%02x", buf[i]);
  }
  printf("\n");

  libusb_exit(NULL);
  return 0;
}
