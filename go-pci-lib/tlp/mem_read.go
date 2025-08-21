package tlp

import (
	"bytes"
	"fmt"
)

// MRd TLP: Memory read request.
type MRd struct {
	RequestHeader
	Address Address
}

// ToBytes encodes MRd to wire format.
func (tlp *MRd) ToBytes() []byte {
	buf := new(bytes.Buffer)
	tlp.RequestHeader.toBuffer(buf)
	tlp.Address.toBuffer(buf)
	return buf.Bytes()
}

// NewMRd builds memory read request.
// |length| is the number of BYTES to read and must be DWORD aligned.
func NewMRd(reqID DeviceID, tag uint8, addr uint64, length uint32) (*MRd, error) {
	tlp := &MRd{}
	tlp.Address = Address(addr)
	if tlp.Address.is64() {
		tlp.Type = MRd4
	} else {
		tlp.Type = MRd3
	}

	if err := tlp.setLength(int(length)); err != nil {
		return nil, err
	}

	tlp.AT = DefaultUntranslated
	tlp.ReqID = reqID
	tlp.Tag = tag
	tlp.setByteEnables()
	return tlp, nil
}

// NewMRdFromBytes builds a memory read request from a TLP buffer.
func NewMRdFromBytes(b []byte) (*MRd, error) {
	if len(b) < 3*dwordLen {
		return nil, fmt.Errorf("%w: TLP buffer too short (%d), expected at least 12 bytes", ErrTooShort, len(b))
	}
	// Verify type.
	var hdr TlpHeader
	hdr.fromBuffer(bytes.NewBuffer(b))
	if hdr.Type != MRd3 && hdr.Type != MRd4 {
		return nil, fmt.Errorf("%w: type %x is not supported. supported types: MRd3, MRd4", ErrBadType, hdr.Type)
	}
	if hdr.Type == MRd3 && len(b) < 3*dwordLen {
		return nil, fmt.Errorf("%w: TLP buffer too short (%d), want at least %d", ErrTooShort, len(b), 3*dwordLen)
	}
	if hdr.Type == MRd4 && len(b) < 4*dwordLen {
		return nil, fmt.Errorf("%w: TLP buffer too short (%d), want at least %d", ErrTooShort, len(b), 4*dwordLen)
	}
	// Decode MRd.
	tlp := &MRd{}
	buf := bytes.NewBuffer(b)
	tlp.RequestHeader.fromBuffer(buf)
	is64 := hdr.Type == MRd4
	tlp.Address.fromBuffer(is64, buf)
	return tlp, nil
}
