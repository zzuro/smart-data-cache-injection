package tlp

import "bytes"

// CfgHeader extends RequestHeader and includes the third header dword
// for configuration read TLPs.
type CfgHeader struct {
	RequestHeader
	Target DeviceID
	// Register number (6b)
	RegisterNumber int
	// Extended register number (4b)
	ExtRegisterNumber int
}

func (h *CfgHeader) toBuffer(buf *bytes.Buffer) {
	h.RequestHeader.toBuffer(buf)

	dw := make([]byte, dwordLen)
	copy(dw[0:2], h.Target.toBytes())
	dw[2] = byte(h.ExtRegisterNumber & 0xf)
	dw[3] = byte(h.RegisterNumber << 2)
	buf.Write(dw)
}

func (h *CfgHeader) fromBuffer(buf *bytes.Buffer) {
	h.RequestHeader.fromBuffer(buf)
	dw := buf.Next(dwordLen)
	h.Target.fromBytes(dw[0:2])
	h.ExtRegisterNumber = int(dw[2] & 0xf)
	h.RegisterNumber = int(dw[3]>>2) & 0x3f
}
