#ifndef __SAOS__DRIVERS__KEYBOARD_H
#define __SAOS__DRIVERS__KEYBOARD_H

#include <common/types.h>
#include <hardwares/interrupts.h>
#include <hardwares/port.h>
#include <drivers/driver.h>
namespace saos
{
namespace drivers
{
class KeyboardEventHandler
{
public:
  KeyboardEventHandler();

  virtual void OnKeyDown(char);
  virtual void OnKeyUp(char);
};
class KeyboardDriver : public saos::hardwares::InterruptHandler, public Driver
{
  saos::hardwares::Port_8Bit dataport;
  saos::hardwares::Port_8Bit commandport;
  KeyboardEventHandler *handler;

public:
  KeyboardDriver(saos::hardwares::InterruptManager *manager, KeyboardEventHandler *handler);
  ~KeyboardDriver();
  virtual saos::common::uint32_t HandleInterrupt(saos::common::uint32_t esp);
  virtual void Activate();
};
} // namespace drivers
} // namespace saos
#endif