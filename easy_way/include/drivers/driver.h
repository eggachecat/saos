#ifndef __SAOS__DRIVERS__DRIVER_H
#define __SAOS__DRIVERS__DRIVER_H

namespace saos
{
namespace drivers
{
class Driver
{
public:
  Driver();
  ~Driver();

  virtual void Activate();

  // when os starts we need to reset the hardware
  // scenario:
  //      kernel update without rebooting
  //      the bootloader might have left things in an uncertain state
  virtual int Reset();

  virtual void Deactivate();
};

class DriverManager
{
private:
  Driver *drivers[255];
  int numDrivers;

public:
  DriverManager();
  ~DriverManager();
  void AddDriver(Driver *);
  void ActivateAll();
};
} // namespace drivers
} // namespace saos

#endif