#ifndef __SAOS__HARDWARES__PCI_H
#define __SAOS__HARDWARES__PCI_H

#include <hardwares/port.h>
#include <common/types.h>
#include <drivers/driver.h>

namespace saos
{
namespace hardwares
{

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
  void SelectDrivers(saos::drivers::DriverManager *driverManager);
  PeripheralComponentInterconnectDeviceDescriptor GetDeviceDescriptor(saos::common::uint16_t bus, saos::common::uint16_t device, saos::common::uint16_t function);
};
} // namespace hardwares
} // namespace saos

#endif