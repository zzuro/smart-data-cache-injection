package tlp

import (
	"bytes"
	"fmt"
	"math/bits"
)

// CfgRd TLP: Configuration read request.
type CfgRd struct {
	CfgHeader
}

// ToBytes encodes CfgRd to wire format.
func (tlp *CfgRd) ToBytes() []byte {
	buf := new(bytes.Buffer)
	tlp.CfgHeader.toBuffer(buf)
	return buf.Bytes()
}

// NewCfgRd builds configuration read request.
func NewCfgRd(reqID DeviceID, tag uint8, target DeviceID, register int) *CfgRd {
	tlp := &CfgRd{}
	// See Figure 2-18: Request Header Format for Configuration Transactions.
	tlp.Type = CfgRd0
	tlp.Length = 1
	tlp.ReqID = reqID
	tlp.Tag = tag
	tlp.FirstBE = 0xf
	tlp.LastBE = 0
	tlp.Target = target
	tlp.RegisterNumber = (register << 2) & 0x3f
	tlp.ExtRegisterNumber = register & 0xf
	return tlp
}

// CfgWr TLP: Configuration write request.
type CfgWr struct {
	CfgHeader
	Data []byte
}

// ToBytes encodes CfgWr to wire format.
func (tlp *CfgWr) ToBytes() []byte {
	buf := new(bytes.Buffer)
	tlp.CfgHeader.toBuffer(buf)
	buf.Write(tlp.Data)
	return buf.Bytes()
}

// NewCfgWr builds configuration write request.
func NewCfgWr(reqID DeviceID, tag uint8, target DeviceID, register int, data [4]byte) *CfgWr {
	tlp := &CfgWr{}
	// See Figure 2-18: Request Header Format for Configuration Transactions.
	tlp.Type = CfgWr0
	tlp.Length = 1
	tlp.ReqID = reqID
	tlp.Tag = tag
	tlp.FirstBE = 0xf
	tlp.LastBE = 0
	tlp.Target = target
	tlp.RegisterNumber = (register << 2) & 0x3f
	tlp.ExtRegisterNumber = register & 0xf
	tlp.Data = make([]byte, len(data))
	copy(tlp.Data, data[:])
	return tlp
}

// NewCfgWrFromBytes builds a memory read request from a TLP buffer.
func NewCfgWrFromBytes(b []byte) (*CfgWr, error) {
	if len(b) < 3*dwordLen {
		return nil, fmt.Errorf("%w: TLP buffer too short (%d), expected at least 12 bytes", ErrTooShort, len(b))
	}
	// Verify type.
	var hdr TlpHeader
	hdr.fromBuffer(bytes.NewBuffer(b))
	if hdr.Type != CfgWr0 && hdr.Type != CfgWr1 {
		return nil, fmt.Errorf("%w: type %x is not supported. supported types: CfgWr0, CfgWr1", ErrBadType, hdr.Type)
	}
	// Decode CfgWr.
	tlp := &CfgWr{}
	buf := bytes.NewBuffer(b)
	tlp.CfgHeader.fromBuffer(buf)
	tlp.Data = make([]byte, tlp.DataLength())
	if buf.Len() < len(tlp.Data) {
		return nil, fmt.Errorf("%w: TLP data too short (%d), expected %d bytes", ErrTooShort, buf.Len(), len(tlp.Data))
	}
	copy(tlp.Data, buf.Next(len(tlp.Data)))
	if tlp.Length != 1 {
		return nil, fmt.Errorf("%w: TLP bad length in request (%d), expected 1", ErrBadLength, tlp.Length)
	}
	if tlp.LastBE != 0 {
		return nil, fmt.Errorf("%w: TLP bad LastBE in request (%d), expected 0", ErrBadLength, tlp.LastBE)
	}
	return tlp, nil
}

// Returns the config space memory address.
// Table 7-1: Enhanced Configuration Address Mapping.
func (tlp *CfgWr) MemoryAddress() int {
	offset := bits.TrailingZeros8(tlp.FirstBE)
	return tlp.ExtRegisterNumber<<8 + tlp.RegisterNumber<<2 + offset
}

// Returns the first enabled data.
func (tlp *CfgWr) FirstDataByte() byte {
	offset := bits.TrailingZeros8(tlp.FirstBE)
	return tlp.Data[offset]
}
