package tlp

import "bytes"

// CplHeader extends TlpHeader and includes the second and third header dwords
// for Completion TLPs.
// See section 2.2.9. Completion Rules
type CplHeader struct {
	TlpHeader
	// Completer ID.
	CplID DeviceID
	// Byte count: the number of bytes left for transmission, including those in
	// the current packet (12b).
	BC int
	// Completion status.
	Status CompletionStatus
	// Requester ID.
	ReqID DeviceID
	// Unique tag for all outstanding requests.
	Tag uint8
	// Lower Byte Address for starting byte of Completion (7b).
	AddressLow uint8
}

func (h *CplHeader) toBuffer(buf *bytes.Buffer) {
	h.TlpHeader.toBuffer(buf)

	dw1 := make([]byte, dwordLen)
	copy(dw1[0:2], h.CplID.toBytes())
	dw1[2] = byte(
		(int(h.Status) << 5) |
			(h.BC>>8)&0xf)
	dw1[3] = byte(h.BC & 0xff)
	buf.Write(dw1)

	dw2 := make([]byte, dwordLen)
	copy(dw2[0:2], h.ReqID.toBytes())
	dw2[2] = byte(h.Tag)
	dw2[3] = byte(h.AddressLow & 0x7f)
	buf.Write(dw2)
}

func (h *CplHeader) fromBuffer(buf *bytes.Buffer) {
	h.TlpHeader.fromBuffer(buf)

	dw1 := buf.Next(dwordLen)
	h.CplID.fromBytes(dw1[0:2])
	h.Status = CompletionStatus(getSubField(dw1[2], 5, 7))
	h.BC = getSubField(dw1[2], 0, 0xf)<<8 | int(dw1[3])

	dw2 := buf.Next(dwordLen)
	h.ReqID.fromBytes(dw2[0:2])
	h.Tag = uint8(dw2[2])
	h.AddressLow = uint8(dw2[3] & 0x7f)
}
