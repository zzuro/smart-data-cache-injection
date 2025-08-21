package tlp

import (
	"encoding/binary"
	"errors"
)

// abbreviations
var (
	be = binary.BigEndian
)

// errors
var (
	ErrBadType    = errors.New("bad TLP header type")
	ErrBadLength  = errors.New("bad TLP data length")
	ErrBadAddress = errors.New("bad TLP address")
	ErrTooShort   = errors.New("TLP packet too short")
)

const (
	dwordLen   = 4
	maxDataLen = 1024 * dwordLen
	// MaxTLPBuffer is a 4 dword header + max data payload.
	MaxTLPBuffer = 4*dwordLen + maxDataLen*dwordLen
)

const (
	fmt3DWNoData   = 0b000
	fmt4DWNoData   = 0b001
	fmt3DWWithData = 0b010
	fmt4DWWithData = 0b011
	fmtTlpPrefix   = 0b100
)

// TlpType is the format and type field in the TLP header.
// See Table 2-3 in PCI EXPRESS BASE SPECIFICATION, REV. 3.1a.
type TlpType uint8

const (
	// MRd3 is a Memory Read Request encoded with 3 dwords.
	MRd3 TlpType = (fmt3DWNoData << 5) | 0b00000
	// MRd4 is a Memory Read Request encoded with 4 dwords.
	MRd4 TlpType = (fmt4DWNoData << 5) | 0b00000
	// MRdLk3 is a Memory Read Request-Locked encoded with 3 dwords.
	MRdLk3 TlpType = (fmt3DWNoData << 5) | 0b00001
	// MRdLk4 is a Memory Read Request-Locked encoded with 4 dwords.
	MRdLk4 TlpType = (fmt4DWNoData << 5) | 0b00001
	// MWr3 is a Memory Write Request encoded with 3 dwords.
	MWr3 TlpType = (fmt3DWWithData << 5) | 0b00000
	// MWr4 is a Memory Write Request encoded with 4 dwords.
	MWr4 TlpType = (fmt4DWWithData << 5) | 0b00000
	// FetchAdd is a Memory Write Request encoded with 4 dwords.
	FetchAdd TlpType = (fmt4DWWithData << 5) | 0b01100
	// IORdT is an I/O Read Request.
	IORdT TlpType = (fmt3DWNoData << 5) | 0b00010
	// IOWrtT is an I/O Write Request.
	IOWrtT TlpType = (fmt3DWWithData << 5) | 0b00010
	// CfgRd0 is a Configuration Read of Type 0.
	CfgRd0 TlpType = (fmt3DWNoData << 5) | 0b00100
	// CfgWr0 is a Configuration Write of Type 0.
	CfgWr0 TlpType = (fmt3DWWithData << 5) | 0b00100
	// CfgRd1 is a Configuration Read of Type 1.
	CfgRd1 TlpType = (fmt3DWNoData << 5) | 0b00101
	// CfgWr1 is a Configuration Write of Type 1.
	CfgWr1 TlpType = (fmt3DWWithData << 5) | 0b00101
	// CplE is a Completion without Data. Used for I/O and
	// Configuration Write Completions with any
	// Completion Status.
	CplE TlpType = (fmt3DWNoData << 5) | 0b01010
	// CplD is a Completion with Data. Used for Memory,
	// I/O, and Configuration Read Completions.
	CplD TlpType = (fmt3DWWithData << 5) | 0b01010
	// CplLk is a Completion for Locked Memory Read without
	// Data. Used only in error case.
	CplLk TlpType = (fmt3DWNoData << 5) | 0b01011
	// CplLkD is a Completion for Locked Memory Read –
	// otherwise like CplD.
	CplLkD TlpType = (fmt3DWWithData << 5) | 0b01011
	// MRIOV is a Multi-Root I/O Virtualization and Sharing (MR-IOV) TLP prefix.
	MRIOV TlpType = (fmtTlpPrefix << 5) | 0b00000
	// LocalVendPrefix is a Local TLP prefix with vendor sub-field.
	LocalVendPrefix TlpType = (fmtTlpPrefix << 5) | 0b01110
	// ExtTPH is an Extended TPH TLP prefix.
	ExtTPH TlpType = (fmtTlpPrefix << 5) | 0b10000
	// PASID is a Process Address Space ID (PASID) TLP Prefix.
	PASID TlpType = (fmtTlpPrefix << 5) | 0b10001
	// EndEndVendPrefix is an End-to-End TLP prefix with vendor sub-field.
	EndEndVendPrefix TlpType = (fmtTlpPrefix << 5) | 0b11110
)

// AddressType is the address type field in the request header.
type AddressType uint8

// Supported address types.
const (
	DefaultUntranslated AddressType = 0b00
	TranslationRequest  AddressType = 0b01
	Translated          AddressType = 0b10
	AddressTypeReserved AddressType = 0b11
)

// TrafficClass is the traffic class field in the request header and used
// to set quality of service (QoS).
type TrafficClass uint8

// Supported traffic classes.
const (
	TC0 TrafficClass = iota
	TC1
	TC2
	TC3
	TC4
	TC5
	TC6
	TC7
)

// CompletionStatus is the completion status field in the completion header.
type CompletionStatus uint8

// Supported completion status.
const (
	SuccessfulCompletion      CompletionStatus = 0b000
	UnsupportedRequest        CompletionStatus = 0b001
	ConfigurationRequestRetry CompletionStatus = 0b010
	CompleterAbort            CompletionStatus = 0b100
)
