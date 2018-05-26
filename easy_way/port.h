#ifndef __PORT_H
#define __PORT_H

#include "types.h"

class Port
{
  protected:
    uint16_t portnumber;
    Port(uint16_t portnumber);
    ~Port();
};

class Port_8Bit : public Port
{
  public:
    Port_8Bit(uint16_t portnumber);
    ~Port_8Bit();
    virtual void Write(uint8_t data);
    virtual uint8_t Read();
};

class Port_8Bit_Slow : public Port_8Bit
{
  protected:
    Port_8Bit_Slow(uint16_t portnumber);
    ~Port_8Bit_Slow();
    virtual void Write(uint8_t data);
};

class Port_16Bit : public Port
{
  protected:
    Port_16Bit(uint16_t portnumber);
    ~Port_16Bit();
    virtual void Write(uint16_t data);
    virtual uint16_t Read();
};

class Port_32Bit : public Port
{
  protected:
    Port_32Bit(uint16_t portnumber);
    ~Port_32Bit();
    virtual void Write(uint32_t data);
    virtual uint32_t Read();
};

#endif