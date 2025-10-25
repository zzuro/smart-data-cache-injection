package main

import (
	"crypto/rand"
	"fmt"
	"log"

	"C"

	"github.com/Kakashiiiiy/go-pci-lib/screamer"
	genTLP "github.com/Kakashiiiiy/go-pci-lib/tlp"
	"go.uber.org/zap"

	"encoding/hex"
)

func print_status(status genTLP.CompletionStatus) {
	switch status {
	case genTLP.SuccessfulCompletion:
		fmt.Println("Successful Completion")
	case genTLP.UnsupportedRequest:
		fmt.Println("Unsupported Request")
	case genTLP.ConfigurationRequestRetry:
		fmt.Println("Completion Timeout")
	case genTLP.CompleterAbort:
		fmt.Println("Completion Aborted")
	default:
		fmt.Printf("Unknown Status: %x\n", status)
	}
}

func print_packet(tlp []byte) {
	for i := 0; i < len(tlp); i += 4 {
		end := i + 4
		if end > len(tlp) {
			end = len(tlp)
		}
		fmt.Printf("Byte %2d -> ", i)
		for j := i; j < end; j++ {
			// Print bits for each byte from bit7 to bit0
			for b := 7; b >= 0; b-- {
				bit := (tlp[j] >> b) & 1
				fmt.Print(bit)
			}
			fmt.Print(" ")
		}
		fmt.Println()
	}
}

func createAndSendReadRequest(rw *screamer.TLPController, reqID genTLP.DeviceID, addr uint64, tag uint8, maxLen uint32) {
	tlp, err := genTLP.NewMRd(reqID, tag, addr, maxLen)
	if err != nil {
		log.Fatal(err)
	}

	rw.Write(tlp.ToBytes())

	b := make([]byte, 4096)

	rw.Read(b)

	// Parse response completion
	data, err := genTLP.NewCplFromBytes(b)
	if err != nil {
		fmt.Println(addr)
		log.Fatal(err)
	}

	print_status(data.Status)

	if data.Status == genTLP.SuccessfulCompletion {

		fmt.Printf("Reading Address: 0x%x\n", addr)
		fmt.Println("Data:")
		fmt.Print(hex.Dump(data.Data[:data.DataLength()]))
		fmt.Print("\n")
	}
}

func createAndSendWriteRequest(rw *screamer.TLPController, reqID genTLP.DeviceID, addr uint64, data []byte) {
	tlp, err := genTLP.NewMWr(reqID, addr, data)
	if err != nil {
		log.Fatal(err)
	}
	rw.Write(tlp.ToBytes())
}

/*
To export use: go build -buildmode=c-shared -o libgopci.so
*/

//export writeRandomData
func writeRandomData() uint32 {
	logger, _ := zap.NewDevelopment(
		zap.IncreaseLevel(zap.ErrorLevel), // only logs Error or higher
	)
	defer logger.Sync() // flushes buffer

	// Enumerates and opens all screamer devices
	devs, err := screamer.OpenScreamers(logger.Named("Screamer"))
	if err != nil {
		logger.Error("failed to open devices", zap.Error(err))
	}
	// Closes screammaers on exit.
	defer func() {
		for _, d := range devs {
			d.Close()
		}
	}()

	if len(devs) == 0 {
		log.Fatal("device not found")
	}

	rw := screamer.NewTLPController(devs[0], logger)

	reqID := genTLP.DeviceID{
		Bus:      0xe1,
		Device:   0x00,
		Function: 0x00,
	}

	addr := uint64(0xfd790000)

	data := make([]byte, 4)
	_, err = rand.Read(data)
	if err != nil {
		log.Fatal(err)
	}

	value := uint32(data[3])<<24 | uint32(data[2])<<16 | uint32(data[1])<<8 | uint32(data[0])

	createAndSendWriteRequest(rw, reqID, addr, data)

	return value
}

//export sdci
func sdci(tag uint16) uint32 {
	logger, _ := zap.NewDevelopment(
		zap.IncreaseLevel(zap.ErrorLevel), // only logs Error or higher
	)
	defer logger.Sync() // flushes buffer

	// Enumerates and opens all screamer devices
	devs, err := screamer.OpenScreamers(logger.Named("Screamer"))
	if err != nil {
		logger.Error("failed to open devices", zap.Error(err))
	}
	// Closes screammaers on exit.
	defer func() {
		for _, d := range devs {
			d.Close()
		}
	}()

	if len(devs) == 0 {
		log.Fatal("device not found")
	}

	rw := screamer.NewTLPController(devs[0], logger)

	reqID := genTLP.DeviceID{
		Bus:      0xe1,
		Device:   0x00,
		Function: 0x00,
	}

	addr := uint64(0x1000000)

	data := make([]byte, 64)
	_, err = rand.Read(data)
	if err != nil {
		log.Fatal(err)
	}

	value := uint32(data[3])<<24 | uint32(data[2])<<16 | uint32(data[1])<<8 | uint32(data[0])

	tlp, err := genTLP.NewMWr(reqID, addr, data)
	if err != nil {
		log.Fatal(err)
	}

	tlp.Address = genTLP.Address((addr &^ 0x3) | 0x2) // Indicates frequent read and/or write access to data by device
	tlp.NS = true
	tlp.TH = true
	tlp.Tag = uint8(tag)
	rw.Write(tlp.ToBytes())

	return value
}

func main() {
	logger, _ := zap.NewDevelopment()
	defer logger.Sync() // flushes buffer, if any

	// Enumerates and opens all screamer devices on the system.
	devs, err := screamer.OpenScreamers(logger.WithOptions(zap.IncreaseLevel(zap.WarnLevel)).Named("Screamer"))
	if err != nil {
		log.Fatal(err)
	}
	// Closes screammaers on exit.
	defer func() {
		for _, d := range devs {
			d.Close()
		}
	}()
	if len(devs) == 0 {
		log.Fatal("device not found")
	}

	// Creates an io.ReadWriter for reading and writing TLPs.
	rw := screamer.NewTLPController(devs[0], logger)

	// Build memory read TLP.
	//e1:00.0 Ethernet controller: Xilinx Corporation Device 0666 (rev 02)
	reqID := genTLP.DeviceID{
		Bus:      0xe1,
		Device:   0x00,
		Function: 0x00,
	}

	tag := uint8(0x1)
	addr := uint64(0xc0000000)
	createAndSendReadRequest(rw, reqID, addr, tag, 0x100)

	data := make([]byte, 0x100)
	_, err = rand.Read(data)
	if err != nil {
		log.Fatal(err)
	}

	for i := 0; i < 100; i++ {
		tlp, err := genTLP.NewMWr(reqID, addr, data)
		if err != nil {
			log.Fatal(err)
		}

		tlp.Tag = uint8(i)
		tlp.Address = genTLP.Address((addr &^ 0x3) | 0x2)
		tlp.NS = true
		tlp.TH = true
		rw.Write(tlp.ToBytes())
	}

	tag = uint8(0x2)
	createAndSendReadRequest(rw, reqID, addr, tag, 0x100)
}
