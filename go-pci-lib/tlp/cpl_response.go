package tlp

import (
	"bytes"
	"fmt"
)

// Cpl TLP: Completion response.
type Cpl struct {
	CplHeader
	Data []byte
}

// ToBytes encodes Cpl to wire format.
func (tlp *Cpl) ToBytes() []byte {
	buf := new(bytes.Buffer)
	tlp.CplHeader.toBuffer(buf)
	buf.Write(tlp.Data)
	return buf.Bytes()
}

// NewCplFromBytes builds completion response from TLP buffer.
func NewCplFromBytes(b []byte) (*Cpl, error) {
	if len(b) < 3*dwordLen {
		return nil, fmt.Errorf("%w: TLP buffer too short (%d), expected at least 12 bytes", ErrTooShort, len(b))
	}

	// Verify type.
	var hdr TlpHeader
	hdr.fromBuffer(bytes.NewBuffer(b))
	if hdr.Type != CplE && hdr.Type != CplD {
		return nil, fmt.Errorf("%w: type %x is not supported. Supported types: CplE, CplD", ErrBadType, hdr.Type)
	}

	// Decode CPL.
	tlp := &Cpl{}
	buf := bytes.NewBuffer(b)
	tlp.CplHeader.fromBuffer(buf)
	if tlp.Type == CplD {
		tlp.Data = make([]byte, tlp.DataLength())
		if buf.Len() < len(tlp.Data) {
			return nil, fmt.Errorf("%w: TLP data too short (%d), expected %d bytes", ErrTooShort, buf.Len(), len(tlp.Data))
		}
		copy(tlp.Data, buf.Next(len(tlp.Data)))
	}
	return tlp, nil
}

// NewCpl builds completion response.
func NewCpl(cplID DeviceID, bc int, status CompletionStatus, reqID DeviceID, tag, addressLow uint8, data []byte) (*Cpl, error) {
	tlp := &Cpl{}
	tlp.CplID = cplID
	tlp.BC = bc
	tlp.Status = status
	tlp.ReqID = reqID
	tlp.Tag = tag
	tlp.AddressLow = addressLow

	if len(data) > 0 {
		tlp.Type = CplD
		if err := tlp.setLength(len(data)); err != nil {
			return nil, err
		}
		tlp.Data = make([]byte, len(data))
		copy(tlp.Data, data)
	} else {
		tlp.Type = CplE
	}
	return tlp, nil
}

// See Table 2-37: Calculating Byte Count from Length and Byte Enables.
func CplCalcByteCount(firstBE, lastBE, length int) int {
	if firstBE&0b1001 == 0b1001 && lastBE == 0b0000 {
		return 4
	}
	if firstBE&0b1101 == 0b0101 && lastBE == 0b0000 {
		return 3
	}
	if firstBE&0b1011 == 0b1010 && lastBE == 0b0000 {
		return 3
	}
	if firstBE == 0b0011 && lastBE == 0b0000 {
		return 2
	}
	if firstBE == 0b0110 && lastBE == 0b0000 {
		return 2
	}
	if firstBE == 0b1100 && lastBE == 0b0000 {
		return 2
	}
	if firstBE == 0b0001 && lastBE == 0b0000 {
		return 1
	}
	if firstBE == 0b0010 && lastBE == 0b0000 {
		return 1
	}
	if firstBE == 0b0100 && lastBE == 0b0000 {
		return 1
	}
	if firstBE == 0b1000 && lastBE == 0b0000 {
		return 1
	}
	if firstBE == 0b0000 && lastBE == 0b0000 {
		return 1
	}
	if firstBE&0b0001 == 0b0001 && lastBE&0b1000 == 0b1000 {
		return length * 4
	}
	if firstBE&0b0001 == 0b0001 && lastBE&0b1100 == 0b0100 {
		return length*4 - 1
	}
	if firstBE&0b0001 == 0b0001 && lastBE&0b1110 == 0b0010 {
		return length*4 - 2
	}
	if firstBE&0b0001 == 0b0001 && lastBE == 0b0001 {
		return length*4 - 3
	}
	if firstBE&0b0011 == 0b0010 && lastBE&0b1000 == 0b1000 {
		return length*4 - 1
	}
	if firstBE&0b0011 == 0b0010 && lastBE&0b1100 == 0b0100 {
		return length*4 - 2
	}
	if firstBE&0b0011 == 0b0010 && lastBE&0b1110 == 0b0010 {
		return length*4 - 3
	}
	if firstBE&0b0011 == 0b0010 && lastBE == 0b0001 {
		return length*4 - 4
	}
	if firstBE&0b0111 == 0b0100 && lastBE&0b1000 == 0b1000 {
		return length*4 - 2
	}
	if firstBE&0b0111 == 0b0100 && lastBE&0b1100 == 0b0100 {
		return length*4 - 3
	}
	if firstBE&0b0111 == 0b0100 && lastBE&0b1110 == 0b0010 {
		return length*4 - 4
	}
	if firstBE&0b0111 == 0b0100 && lastBE == 0b0001 {
		return length*4 - 5
	}
	if firstBE == 0b1000 && lastBE&0b1000 == 0b1000 {
		return length*4 - 3
	}
	if firstBE == 0b1000 && lastBE&0b1100 == 0b0100 {
		return length*4 - 4
	}
	if firstBE == 0b1000 && lastBE&0b1110 == 0b0010 {
		return length*4 - 5
	}
	if firstBE == 0b1000 && lastBE == 0b0001 {
		return length*4 - 6
	}
	return 0
}

// Table 2-38: Calculating Lower Address from 1st DW BE.
func CplCalcLowerAddress(firstBE int, readAddress Address) byte {
	addr := byte(readAddress & 0x7c)
	if firstBE == 0b0000 {
		return addr + 0b00
	}
	if firstBE&0b0001 == 0b0001 {
		return addr + 0b00
	}
	if firstBE&0b0011 == 0b0010 {
		return addr + 0b01
	}
	if firstBE&0b0111 == 0b0100 {
		return addr + 0b10
	}
	if firstBE == 0b1000 {
		return addr + 0b11
	}
	return 0
}

// NewCplForMrd builds a completion response that matches the given memory read request.
func NewCplForMrd(cplID DeviceID, status CompletionStatus, mrd *MRd, data []byte) (*Cpl, error) {
	if len(data) != mrd.DataLength() {
		return nil, fmt.Errorf("%w: buffer size (%d) does not match expected DataLength (%d)", ErrBadLength, len(data), mrd.DataLength())
	}
	bc := CplCalcByteCount(int(mrd.FirstBE), int(mrd.LastBE), mrd.Length)
	addressLow := CplCalcLowerAddress(int(mrd.FirstBE), mrd.Address)
	return NewCpl(cplID, bc, status, mrd.ReqID, mrd.Tag, addressLow, data)
}
