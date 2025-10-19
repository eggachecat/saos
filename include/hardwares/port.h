#ifndef __SAOS__HARDWARES__PORT_H
#define __SAOS__HARDWARES__PORT_H

#include <common/types.h>
namespace saos
{
namespace hardwares
{
class Port
{
public:
  saos::common::uint16_t portnumber;
  Port(saos::common::uint16_t portnumber);
  ~Port();
};

class Port_8Bit : public Port
{
public:
  Port_8Bit(saos::common::uint16_t portnumber);
  ~Port_8Bit();
  virtual void Write(saos::common::uint8_t data);
  virtual saos::common::uint8_t Read();
};

class Port_8Bit_Slow : public Port_8Bit
{
public:
  Port_8Bit_Slow(saos::common::uint16_t portnumber);
  ~Port_8Bit_Slow();
  virtual void Write(saos::common::uint8_t data);
};

class Port_16Bit : public Port
{
public:
  Port_16Bit(saos::common::uint16_t portnumber);
  ~Port_16Bit();
  virtual void Write(saos::common::uint16_t data);
  virtual saos::common::uint16_t Read();
};

class Port_32Bit : public Port
{
public:
  Port_32Bit(saos::common::uint16_t portnumber);
  ~Port_32Bit();
  virtual void Write(saos::common::uint32_t data);
  virtual saos::common::uint32_t Read();
};
} // namespace HARDWARES
} // namespace saos
#endif