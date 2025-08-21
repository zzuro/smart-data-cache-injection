package main

import (
	"encoding/hex"
	"fmt"
	"log"

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

func createAndSendTLP(rw *screamer.TLPController, reqID genTLP.DeviceID, addr uint64, tag uint8, maxLen uint32) {
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
	tag := uint8(0x15)
	createAndSendTLP(rw, reqID, addr, tag, 0x100)

	/*
		tlp, err := genTLP.NewMWr(reqID, i, []byte{0xff, 0xff, 0xff, 0xff})
			if err != nil {
				log.Fatal(err)
			}
			rw.Write(tlp.ToBytes())
	*/

}
