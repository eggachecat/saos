#ifndef __DRIVER_H
#define __DRIVER_H

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
#endif