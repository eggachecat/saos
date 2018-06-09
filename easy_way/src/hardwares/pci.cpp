#include <hardwares/pci.h>
using namespace saos::common;
using namespace saos::hardwares;
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
void PeripheralComponentInterconnectController::SelectDrivers(saos::drivers::DriverManager *driverManager)
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
                    break;
                }
                printf("PCI BUS: ");
                printfHex(bus & 0xFF);

                printf(", Device: ");
                printfHex(device & 0xFF);

                printf(", Function: ");
                printfHex(function & 0xFF);

                printf(" = VENDOR: " );
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