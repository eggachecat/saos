#include <hardwares/pci.h>
using namespace saos::common;
using namespace saos::hardwares;
using namespace saos::drivers;

void printf(char *str);
void printfHex(uint8_t);
PeripheralComponentInterconnectDeviceDescriptor::PeripheralComponentInterconnectDeviceDescriptor() {}
PeripheralComponentInterconnectDeviceDescriptor::~PeripheralComponentInterconnectDeviceDescriptor() {}
PeripheralComponentInterconnectController::PeripheralComponentInterconnectController()
    : dataPort(0xCFC),
      commandPort(0xCF8)
{
}

PeripheralComponentInterconnectController::~PeripheralComponentInterconnectController()
{
}
uint32_t PeripheralComponentInterconnectController::Read(uint16_t bus, uint16_t device, uint16_t function, uint32_t register_offset)
{
    // send identifier to the PCI
    // register_offset & 0xFC cutting of the last 2 bits
    // numbers are bytes
    // [ ] [ ] [ ] [ ] [ ] [ ] [ ] [ ]
    //  0   1   2   3   4   5   6   7
    // <---team-01---> <---team-02--->
    // if we want the 2th
    // we got 0123(team-01)
    // then we get the 2th from team-01
    uint32_t id = 0x1 << 31 | ((bus & 0xFF) << 16) | ((device & 0x1F) << 11) | ((function & 0x07) << 8) | (register_offset & 0xFC);
    commandPort.Write(id);
    uint32_t result = dataPort.Read();
    return result >> (8 * (register_offset % 4));
}
void PeripheralComponentInterconnectController::Write(uint16_t bus, uint16_t device, uint16_t function, uint32_t register_offset, uint32_t value)
{
    uint32_t id = 0x1 << 31 | ((bus & 0xFF) << 16) | ((device & 0x1F) << 11) | ((function & 0x07) << 8) | ((register_offset & 0xFC));
    commandPort.Write(id);
    dataPort.Write(value);
}
bool PeripheralComponentInterconnectController::DeviceHasFunction(saos::common::uint16_t bus, saos::common::uint16_t device)
{
    return Read(bus, device, 0, 0x0E) & (1 << 7); // only the 7th bit tell it has function or not
}
BaseAddressRegister PeripheralComponentInterconnectController::GetBaseAddressRegister(uint16_t bus, uint16_t device, uint16_t function, uint16_t bar)
{
    BaseAddressRegister result;

    uint32_t header_type = Read(bus, device, function, 0x0E) & 0x7F;
    int maxBARs = 6 - (4 * header_type);
    if (bar >= maxBARs)
    {
        return result;
    }

    // base of registers are size of 4
    // lspci -x
    uint32_t bar_value = Read(bus, device, function, 0x10 + 4 * bar);
    result.type = (bar_value & 0x01) ? InputOutput : MemoryMapping; // last bit counts

    uint32_t tmp;

    // write all ones
    // while not all bits are writeable
    // the device will give make unwriteable bits 0
    // read it back and we will know which bits are unwriteable
    // information for mapping range

    if (result.type == MemoryMapping)
    {
        switch ((bar_value >> 1) & 0x03)
        {
        case 0: //32 Bit Mode
        case 1: //20 Bit Mode
        case 2: //64 bit Mode
            break;
        }
        result.prefetchable = ((bar_value >> 3) & 0x01 == 1);
    }
    else // InputOutput
    {
        result.address = (uint8_t *)(bar_value & ~0x3); // remove last values
        result.prefetchable = false;
    }

    return result;
}
Driver *PeripheralComponentInterconnectController::GetDriver(PeripheralComponentInterconnectDeviceDescriptor dev, saos::hardwares::InterruptManager *interrupts)
{
    switch (dev.vendor_id)
    {
    case 0x1022: // AMD
        switch (dev.device_id)
        {
        case 0x2000: // am
            printf("driver 0x2000\n");
            break;
        }
        break;
    case 0x8086:
        printf("driver 0x8086\n");

        break;
    }
    switch (dev.class_id)
    {
    case 0x03: // graphics
        switch (dev.subclass_id)
        {
        case 0x00: // VGA
            printf("driver 0x00\n");

            break;
        }
        break;
    }
    return 0;
}
void PeripheralComponentInterconnectController::SelectDrivers(saos::drivers::DriverManager *driverManager, InterruptManager *interrupts)
{
    for (int bus = 0; bus < 9; bus++)
    {
        for (int device = 0; device < 32; device++)
        {
            int numFunction = DeviceHasFunction(bus, device) ? 8 : 1;
            for (int function = 0; function < numFunction; function++)
            {
                PeripheralComponentInterconnectDeviceDescriptor dev = GetDeviceDescriptor(bus, device, function);
                if (dev.vendor_id == 0x0000 || dev.vendor_id == 0xFFFF)
                {
                    // functions can be gaped
                    // that is
                    // function-0, function 5 (without 1,2,3,4)
                    continue;
                }

                // we actually add driver to it!
                for (int barNum = 0; barNum < 6; barNum++)
                {
                    BaseAddressRegister bar = GetBaseAddressRegister(bus, device, function, barNum);
                    if (bar.address && (bar.type == InputOutput))
                    {
                        dev.portBase = (uint32_t)bar.address;
                    }
                    Driver *driver = GetDriver(dev, interrupts);
                    if (driver != 0)
                    {
                        driverManager->AddDriver(driver);
                    }
                }

                printf("PCI BUS: ");
                printfHex(bus & 0xFF);

                printf(", Device: ");
                printfHex(device & 0xFF);

                printf(", Function: ");
                printfHex(function & 0xFF);

                printf(" = VENDOR: ");
                printfHex((dev.vendor_id & 0xFF00) >> 8);
                printfHex(dev.vendor_id & 0xFF);

                printf(", DEVICE: ");
                printfHex((dev.device_id & 0xFF00) >> 8);
                printfHex(dev.device_id & 0xFF);

                printf("\n");
            }
        }
    }
}
PeripheralComponentInterconnectDeviceDescriptor PeripheralComponentInterconnectController::GetDeviceDescriptor(saos::common::uint16_t bus, saos::common::uint16_t device, saos::common::uint16_t function)
{
    PeripheralComponentInterconnectDeviceDescriptor result;
    result.bus = bus;
    result.device = device;
    result.function = function;

    result.vendor_id = Read(bus, device, function, 0x00);
    result.device_id = Read(bus, device, function, 0x02);

    result.class_id = Read(bus, device, function, 0x0b);
    result.subclass_id = Read(bus, device, function, 0x0a);
    result.interface_id = Read(bus, device, function, 0x09);

    result.revision = Read(bus, device, function, 0x08);
    result.interrupt = Read(bus, device, function, 0x3c);
    return result;
}