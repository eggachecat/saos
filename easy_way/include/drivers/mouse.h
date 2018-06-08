#ifndef __SAOS__DRIVERS__MOUSE_H
#define __SAOS__DRIVERS__MOUSE_H

#include <common/types.h>
#include <hardwares/interrupts.h>
#include <hardwares/port.h>
#include <drivers/driver.h>
namespace saos
{
namespace drivers
{
class MouseEventHandler
{

public:
  MouseEventHandler();
  virtual void OnActivate();

  virtual void OnMouseDown(saos::common::uint8_t button);
  virtual void OnMouseUp(saos::common::uint8_t button);
  virtual void onMouseMove(int , int );
};
class MouseDriver : public saos::hardwares::InterruptHandler, public Driver
{
  saos::hardwares::Port_8Bit dataport;
  saos::hardwares::Port_8Bit commandport;
  saos::common::uint8_t buffer[3];
  saos::common::uint8_t offset;
  saos::common::uint8_t buttons;
  MouseEventHandler *handler;

public:
  MouseDriver(saos::hardwares::InterruptManager *manager, MouseEventHandler *handler);
  ~MouseDriver();
  virtual saos::common::uint32_t HandleInterrupt(saos::common::uint32_t esp);
  virtual void Activate();
};
}}
#endif