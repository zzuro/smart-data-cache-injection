package tlp

import (
	"bytes"
)

func computeBit(value bool, pos int) int {
	if value {
		return 1 << pos
	}
	return 0
}

func (h *TlpHeader) toBuffer(buf *bytes.Buffer) {
	dw := make([]byte, dwordLen)
	dw[0] = byte(h.Type)
	dw[1] = byte(
		(int(h.TC) << 4) |
			computeBit(h.IBO, 2) |
			computeBit(h.LN, 1) |
			computeBit(h.TH, 0))
	dw[2] = byte(
		computeBit(h.TD, 7) |
			computeBit(h.EP, 6) |
			computeBit(h.RO, 5) |
			computeBit(h.NS, 4) |
			(int(h.AT) << 2) |
			((h.Length >> 8) & 3))
	dw[3] = byte(h.Length & 0xff)
	buf.Write(dw)
}

func getSubField(input byte, shift, mask int) int {
	return (int(input) >> shift) & mask
}

func getBit(input byte, pos int) bool {
	return int(input>>pos)&1 > 0
}
