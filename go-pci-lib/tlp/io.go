package tlp

import (
	"bytes"
	"fmt"
)

// IORd TLP: I/O read request.
type IORd struct {
	RequestHeader
	Address Address
}

// ToBytes encodes IORd to wire format.
func (tlp *IORd) ToBytes() []byte {
	buf := new(bytes.Buffer)
	tlp.RequestHeader.toBuffer(buf)
	tlp.Address.toBuffer(buf)
	return buf.Bytes()
}

// NewIORd builds memory read request.
// |length| is the number of BYTES to read and must be DWORD aligned.
func NewIORd(reqID DeviceID, tag uint8, addr uint64, length uint32) (*IORd, error) {
	tlp := &IORd{}
	tlp.Address = Address(addr)
	if tlp.Address.is64() {
		return nil, fmt.Errorf("%w: 64bit address %x is not supported", ErrBadAddress, tlp.Address)
	}
	tlp.Type = IORdT

	if err := tlp.setLength(int(length)); err != nil {
		return nil, err
	}

	tlp.ReqID = reqID
	tlp.Tag = tag
	tlp.setByteEnables()

	return tlp, nil
}

// IOWrt TLP: Memory write request.
type IOWrt struct {
	RequestHeader
	Address Address
	Data    []byte
}

// ToBytes encodes IOWrt to wire format.
func (tlp *IOWrt) ToBytes() []byte {
	buf := new(bytes.Buffer)
	tlp.RequestHeader.toBuffer(buf)
	tlp.Address.toBuffer(buf)
	buf.Write(tlp.Data)
	return buf.Bytes()
}

// NewIOWrt builds memory write request.
// len(data) must be DW aligned.
func NewIOWrt(reqID DeviceID, addr uint64, data []byte) (*IOWrt, error) {
	tlp := &IOWrt{}
	tlp.Address = Address(addr)
	if tlp.Address.is64() {
		return nil, fmt.Errorf("%w: 64bit address %x is not supported", ErrBadAddress, tlp.Address)
	}
	tlp.Type = IOWrtT

	if err := tlp.setLength(len(data)); err != nil {
		return nil, err
	}

	tlp.ReqID = reqID
	tlp.Tag = 0
	tlp.setByteEnables()

	tlp.Data = make([]byte, len(data))
	copy(tlp.Data, data)
	return tlp, nil
}
