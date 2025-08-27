package main

import (
	"crypto/rand"
	"encoding/hex"
	"fmt"
	"log"

	"C"

	"github.com/Kakashiiiiy/go-pci-lib/screamer"
	genTLP "github.com/Kakashiiiiy/go-pci-lib/tlp"
	"go.uber.org/zap"
)

func print_status(status genTLP.CompletionStatus) {
	switch status {
	case genTLP.SuccessfulCompletion:
		fmt.Println("Successful Completion")
	case genTLP.UnsupportedRequest:
		fmt.Println("Data Block Received")
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

	//print_packet(tlp.ToBytes())

	rw.Write(tlp.ToBytes())

	// No response for write requests
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
		Bus:      0x81,
		Device:   0x00,
		Function: 0x00,
	}

	addr := uint64(0xfd990000)

	data := make([]byte, 4)
	_, err = rand.Read(data)
	if err != nil {
		log.Fatal(err)
	}

	value := uint32(data[3])<<24 | uint32(data[2])<<16 | uint32(data[1])<<8 | uint32(data[0])

	createAndSendWriteRequest(rw, reqID, addr, data)

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
	//81:00.0 Ethernet controller: Xilinx Corporation Device 0666 (rev 02)
	reqID := genTLP.DeviceID{
		Bus:      0x81,
		Device:   0x00,
		Function: 0x00,
	}

	addr := uint64(0xfd990000)
	//tag := uint8(0x15)
	//createAndSendReadRequest(rw, reqID, addr, tag, 0x100)

	data := make([]byte, 4)
	_, err = rand.Read(data)
	if err != nil {
		log.Fatal(err)
	}

	// Invert the order of the bytes before printing
	inv := make([]byte, len(data))
	for i := range data {
		inv[i] = data[len(data)-1-i]
	}

	fmt.Printf("Random Data (hex): 0x%s\n", hex.EncodeToString(inv))

	createAndSendWriteRequest(rw, reqID, addr, data)

}
