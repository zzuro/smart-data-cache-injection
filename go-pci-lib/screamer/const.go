package screamer

import (
	"encoding/binary"
	"errors"
	"time"

	"github.com/google/gousb"
)

const (
	// Screamer USB identifiers.
	// "ID 0403:601f Future Technology Devices International, Ltd"
	ftdiVendorID  = 0x0403
	ftdiProductID = 0x601f

	// Screamer PCIe identifiers.
	// "Ethernet controller: Xilinx Corporation Device 0666"
	screamerVID = 0x10ee
	screamerPID = 0x0666

	screamerSessionInterfaceNum        = 0
	screamerSessionInterfaceAltSetting = 0
	screamerSessionOutEndpoint         = 1

	screamerDataInterfaceNum        = 1
	screamerDataInterfaceAltSetting = 0
	screamerDataInEndpoint          = 0x82
	screamerDataOutEndpoint         = 2

	// v4 bitstream can be downloaded from
	// https://github.com/ufrisk/pcileech-fpga/tree/master/pciescreamer
	screamerFPGAVersion = 4
	screamerFPGAID      = 3
)

const (
	rTypeControlIn  uint8 = gousb.ControlIn | gousb.ControlVendor | gousb.ControlInterface
	rTypeControlOut uint8 = gousb.ControlOut | gousb.ControlVendor | gousb.ControlInterface
)

const (
	reqChipConfiguration uint8 = 0xcf
)

type ft60xConfiguration struct {
	VendorID                  uint16
	ProductID                 uint16
	StringDescriptors         [128]byte
	Reserved                  uint8
	PowerAttributes           uint8
	PowerConsumption          uint16
	Reserved2                 uint8
	FIFOClock                 uint8
	FIFOMode                  uint8
	ChannelConfig             uint8
	OptionalFeatureSupport    uint16
	BatteryChargingGPIOConfig uint8
	FlashEEPROMDetection      uint8
	MSIOControl               uint32
	GPIOControl               uint32
}

// ft60xConfiguration consts
const (
	// FIFOMode
	confFIFOMode245 uint8 = 0
	confFIFOMode600 uint8 = 1

	// ChannelConfig
	confChannelConfig4        uint8 = 0
	confChannelConfig2        uint8 = 1
	confChannelConfig1        uint8 = 2
	confChannelConfig1OutPipe uint8 = 3
	confChannelConfig1InPipe  uint8 = 4

	// OptionalFeatureSupport
	confOptionalFeatureSupprtDisableAll            uint16 = 0
	confOptionalFeatureSupprtEnableBatteryCharging uint16 = 1

	// reqChipConfiguration values
	getChipConfigurationValue     = 1
	getChipConfigurationZeroIndex = 0
	setChipConfigurationValue     = 0
	setChipConfigurationZeroIndex = 0
)

// controlRequest specifies a control command and sent over
// the session OUT endpoint.
// Based on struct ft60x_ctrlreq.
type controlRequest struct {
	idx  uint32
	pipe uint8
	cmd  uint8
	unk1 uint8
	unk2 uint8
	len  uint32
	unk4 uint32
	unk5 uint32
}

const ctrlReqReadCmd uint8 = 1

type configFlags uint16

const (
	configFlagPcie      configFlags = 0x0001
	configFlagCore      configFlags = 0x0003
	configFlagReadOnly  configFlags = 0x0000
	configFlagReadWrite configFlags = 0x8000
)

const (
	maxConfigAddr      = 0x1000
	maxConfigReadSize  = 0x1000
	maxConfigWriteSize = 0x200
	maxConfigResponse  = 0x20000
	configTargetRead   = 0x10
	configTargetWrite  = 0x20
	configRequestMagic = 0x77

	statusPropertiesMask = 0xf0000000
	statusWithDataBlocks = 0xe0000000
)

type fpgaVersion struct {
	VersionMajor uint8
	VersionMinor uint8
	FPGAID       uint8
}

const fpgaVersionAddr = 0x0008

const (
	inactivityTimerValue = 100000 // 1ms ( 100000 * 100MHz )
	inactivityTimerAddr  = 0x0008
)

const deviceIDAddr = 0x0008

func min(x, y int) int {
	if x < y {
		return x
	}
	return y
}

func max(x, y int) int {
	if x > y {
		return x
	}
	return y
}

func (ver *fpgaVersion) isSupported() bool {
	return ver.VersionMajor == screamerFPGAVersion && ver.FPGAID >= screamerFPGAID
}

const (
	readTimeout  = 100 * time.Millisecond
	writeTimeout = 100 * time.Millisecond
)

const dwordLen = 4

// Max TX/RX sizes copied from PERFORMANCE_PROFILES array.
const (
	maxReceiveSize  = 0x1c000
	maxTransmitSize = 0x1000
)

// Following values were copied from DeviceFPGA_TxTlp().
var (
	txTLPDataCommand = []byte{0x00, 0x00, 0x00, 0x77}
	txTLPValidLast   = []byte{0x00, 0x00, 0x04, 0x77}
	txTLPLoopback    = []byte{0xcc, 0xdd, 0xee, 0xff, 0x00, 0x00, 0x02, 0x77}
)

const (
	readTLPLastMask        = 7
	readTLPPCIeAndLastData = 4
)

type dword [4]uint8

type dataBlock struct {
	Status uint32
	Data   [7]dword
}

// errors
var (
	ErrUSBOpenFailed          = errors.New("failed claiming USB resources")
	ErrDeviceConfigFailed     = errors.New("failed configuring device")
	ErrUnsupportedVersion     = errors.New("unsupported FPGA version")
	ErrSettingInactivityTimer = errors.New("failed setting inactivity timer")
	ErrReadingDeviceAddress   = errors.New("failed reading device address")
)

// abbreviations
var (
	be = binary.BigEndian
	le = binary.LittleEndian
)
