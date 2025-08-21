package tlp

import (
	"bytes"
	"encoding/binary"
	"math"
)

// Address is the address field in the request header.
type Address uint64

func (a Address) is64() bool {
	return a > math.MaxUint32
}

func (a Address) toBuffer(buf *bytes.Buffer) {
	if a.is64() {
		binary.Write(buf, binary.BigEndian, uint32(a>>32))
	}
	// The 2 lower bits of the Address are reserved for TLP processing hint.
	// See Figure 2-8: "32-bit Address Routing" and
	//     Figure 2-7: "64-bit Address Routing".
	binary.Write(buf, binary.BigEndian, uint32(a&0xfffffffc))
}

func (a *Address) fromBuffer(is64 bool, buf *bytes.Buffer) {
	if is64 {
		var high uint32
		binary.Read(buf, binary.BigEndian, &high)
		*a = Address(uint64(high) << 32)
	}
	var low uint32
	binary.Read(buf, binary.BigEndian, &low)
	*a |= Address(low)
}
