package tlp

import (
	"bytes"
	"fmt"
)

// MWr TLP: Memory write request.
type MWr struct {
	RequestHeader
	Address Address
	Data    []byte
}

// ToBytes encodes MWr to wire format.
func (tlp *MWr) ToBytes() []byte {
	buf := new(bytes.Buffer)
	tlp.RequestHeader.toBuffer(buf)
	tlp.Address.toBuffer(buf)
	buf.Write(tlp.Data)
	return buf.Bytes()
}

// NewMWr builds memory write request.
// len(data) must be DWORD aligned.
func NewMWr(reqID DeviceID, addr uint64, data []byte) (*MWr, error) {
	tlp := &MWr{}
	tlp.Address = Address(addr)
	if tlp.Address.is64() {
		tlp.Type = MWr4
	} else {
		tlp.Type = MWr3
	}

	if err := tlp.setLength(len(data)); err != nil {
		return nil, err
	}

	tlp.ReqID = reqID
	tlp.Tag = 0 //When HT set, than TAG -> Steering Tag
	tlp.TH = false

	tlp.setByteEnables()

	tlp.AT = DefaultUntranslated

	//fmt.Printf("Write to Address: %x\n", tlp.Address)

	tlp.Data = make([]byte, len(data))
	copy(tlp.Data, data)

	return tlp, nil
}

// NewMWr builds memory write request.
// len(data) must be DWORD aligned.
func NewFetchAndADD(reqID DeviceID, addr uint64, data []byte, tag uint8) (*MWr, error) {
	tlp := &MWr{}
	tlp.Address = Address(addr)
	tlp.Type = FetchAdd

	if err := tlp.setLength(len(data)); err != nil {
		return nil, err
	}
	tlp.ReqID = reqID
	tlp.Tag = tag
	tlp.setByteEnables()

	tlp.Data = make([]byte, len(data))
	copy(tlp.Data, data)
	return tlp, nil
}

// NewMWrFromBytes builds a memory write request from a TLP buffer.
func NewMWrFromBytes(b []byte) (*MWr, error) {
	if len(b) < 3*dwordLen {
		return nil, fmt.Errorf("%w: TLP buffer too short (%d), expected at least 12 bytes", ErrTooShort, len(b))
	}
	// Verify type.
	var hdr TlpHeader
	hdr.fromBuffer(bytes.NewBuffer(b))
	if hdr.Type != MWr3 && hdr.Type != MWr4 {
		return nil, fmt.Errorf("%w: type %x is not supported. supported types: MWr3, MWr4", ErrBadType, hdr.Type)
	}
	if hdr.Type == MWr3 && len(b) < 3*dwordLen {
		return nil, fmt.Errorf("%w: TLP buffer too short (%d), want at least %d", ErrTooShort, len(b), 3*dwordLen)
	}
	if hdr.Type == MWr4 && len(b) < 4*dwordLen {
		return nil, fmt.Errorf("%w: TLP buffer too short (%d), want at least %d", ErrTooShort, len(b), 4*dwordLen)
	}
	// Decode MWr.
	tlp := &MWr{}
	buf := bytes.NewBuffer(b)
	tlp.RequestHeader.fromBuffer(buf)
	is64 := hdr.Type == MWr4
	tlp.Address.fromBuffer(is64, buf)

	tlp.Data = make([]byte, tlp.DataLength())
	if buf.Len() < len(tlp.Data) {
		return nil, fmt.Errorf("%w: TLP data too short (%d), expected %d bytes", ErrTooShort, buf.Len(), len(tlp.Data))
	}
	copy(tlp.Data, buf.Next(len(tlp.Data)))
	return tlp, nil
}
