#ifndef __SAOS__DRIVERS__VGA_H
#define __SAOS__DRIVERS__VGA_H

#include <common/types.h>
#include <hardwares/interrupts.h>
#include <hardwares/port.h>
#include <drivers/driver.h>
namespace saos
{
namespace drivers
{
class VideoGraphicsArray
{
  protected:
    saos::hardwares::Port_8Bit miscPort;      //miscellaneous
    saos::hardwares::Port_8Bit crtcIndexPort; //cathode raytube
    saos::hardwares::Port_8Bit crtcDataPort;
    saos::hardwares::Port_8Bit sequenceIndexPort;
    saos::hardwares::Port_8Bit sequenceDataPort;
    saos::hardwares::Port_8Bit graphicsControllerIndexPort;
    saos::hardwares::Port_8Bit graphicsControllerDataPort;
    saos::hardwares::Port_8Bit attributeControllerIndexPort;
    saos::hardwares::Port_8Bit attributeControllerReadPort;
    saos::hardwares::Port_8Bit attributeControllerWritePort;
    saos::hardwares::Port_8Bit attributeControllerResetPort;

    void WriteRegisters(saos::common::uint8_t *registers);
    saos::common::uint8_t *GetFrameBufferSegment();
    virtual saos::common::uint8_t GetColorIndex(saos::common::uint8_t r, saos::common::uint8_t g, saos::common::uint8_t b);

  public:
    VideoGraphicsArray();
    ~VideoGraphicsArray();
    virtual bool SupportsMode(saos::common::uint32_t width, saos::common::uint32_t height, saos::common::uint32_t color_depth);
    virtual bool SetMode(saos::common::uint32_t width, saos::common::uint32_t height, saos::common::uint32_t color_depth);
    virtual void PutPixel(saos::common::uint32_t x, saos::common::uint32_t y, saos::common::uint8_t r, saos::common::uint8_t g, saos::common::uint8_t b);
    virtual void PutPixel(saos::common::uint32_t x, saos::common::uint32_t y, saos::common::uint8_t color_index);
};
}; // namespace drivers
}; // namespace saos

#endif