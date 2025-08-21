package tlp

import "fmt"

// DeviceID is a configuration space address that uniquely identifies
// the device on the PCIe fabric.
type DeviceID struct {
	Bus      uint8
	Device   uint8
	Function uint8
}

func (id *DeviceID) toBytes() []byte {
	b := make([]byte, 2)
	be.PutUint16(b, id.ToUint16())
	return b
}

func (id *DeviceID) fromBytes(b []byte) {
	id.FromUint16(be.Uint16(b))
}

// ToUint16 encodes DeviceID to a uint16 value.
func (id *DeviceID) ToUint16() uint16 {
	return uint16(int(id.Bus)<<8 | int(id.Device)<<3 | int(id.Function))
}

// FromUint16 assigns DeviceID from an encoded uint16 value.
func (id *DeviceID) FromUint16(value uint16) {
	id.Bus = uint8(value >> 8)
	id.Device = uint8((value >> 3) & 0x1f)
	id.Function = uint8(value & 0x07)
}

// NewDeviceID builds a new DeviceID from an encoded uint16 value.
func NewDeviceID(value uint16) (addr DeviceID) {
	addr.FromUint16(value)
	return addr
}

func (id DeviceID) String() string {
	return fmt.Sprintf("%02x:%02x.%01x", id.Bus, id.Device, id.Function)
}

// FromString assigns DeviceID from an encoded string value.
func (id *DeviceID) FromString(value string) error {
	n, err := fmt.Sscanf(value, "%02x:%02x.%01x", &id.Bus, &id.Device, &id.Function)
	if n != 3 || err != nil {
		return err
	}
	return nil
}
