#ifndef __SAOS__HARDWARES__PCI_H
#define __SAOS__HARDWARES__PCI_H

#include <hardwares/port.h>
#include <common/types.h>
#include <drivers/driver.h>
#include <hardwares/interrupts.h>
namespace saos
{
namespace hardwares
{

enum BaseAddressRegisterType
{
  MemoryMapping = 0,
  InputOutput = 1
};

class BaseAddressRegister
{
public:
  bool prefetchable;
  saos::common::uint8_t *address;
  saos::common::uint32_t size;
  BaseAddressRegisterType type;
};

class PeripheralComponentInterconnectDeviceDescriptor
{
public:
  saos::common::uint32_t portBase;
  saos::common::uint32_t interrupt;

  saos::common::uint16_t bus;
  saos::common::uint16_t device;
  saos::common::uint16_t function;

  saos::common::uint16_t vendor_id;
  saos::common::uint16_t device_id;

  saos::common::uint8_t class_id;
  saos::common::uint8_t subclass_id;
  saos::common::uint8_t interface_id;

  saos::common::uint8_t revision;
  PeripheralComponentInterconnectDeviceDescriptor();
  ~PeripheralComponentInterconnectDeviceDescriptor();
};

class PeripheralComponentInterconnectController
{
  saos::hardwares::Port_32Bit dataPort;
  saos::hardwares::Port_32Bit commandPort;

public:
  PeripheralComponentInterconnectController();
  ~PeripheralComponentInterconnectController();
  saos::common::uint32_t Read(saos::common::uint16_t bus, saos::common::uint16_t device, saos::common::uint16_t function, saos::common::uint32_t register_offset);
  void Write(saos::common::uint16_t bus, saos::common::uint16_t device, saos::common::uint16_t function, saos::common::uint32_t register_offset, saos::common::uint32_t value);
  bool DeviceHasFunction(saos::common::uint16_t bus, saos::common::uint16_t device);
  void SelectDrivers(saos::drivers::DriverManager *driverManager, saos::hardwares::InterruptManager* interrupts);
  saos::drivers::Driver* GetDriver(PeripheralComponentInterconnectDeviceDescriptor, saos::hardwares::InterruptManager*);
  
  PeripheralComponentInterconnectDeviceDescriptor GetDeviceDescriptor(saos::common::uint16_t bus, saos::common::uint16_t device, saos::common::uint16_t function);
  BaseAddressRegister GetBaseAddressRegister(saos::common::uint16_t bus, saos::common::uint16_t device, saos::common::uint16_t function, saos::common::uint16_t bar); // bar for Base Register Address
};
} // namespace hardwares
} // namespace saos

#endif