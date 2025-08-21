package main

import (
	"encoding/hex"
	"fmt"
	"log"
	"time"

	"github.com/Kakashiiiiy/go-pci-lib/screamer"
	genTLP "github.com/Kakashiiiiy/go-pci-lib/tlp"
	"go.uber.org/zap"
)

const (
	PSP_C2PMSG_ARGUMENT_0_ADDRESS = 0x03b10700
	PSP_C2PMSG_MESSAGE_ADDRESS    = 0x03b10718
	PSP_C2PMSG_RESPONSE_ADDRESS   = 0x03b10704

	MP1_C2PMSG_ARGUMENT_0_ADDRESS = 0x3B109C4
	MP1_C2PMSG_MESSAGE_ADDRESS    = 0x3B10530
	MP1_C2PMSG_RESPONSE_ADDRESS   = 0x3B1057C
)

func createAndSendTLP(rw *screamer.TLPController, reqID genTLP.DeviceID, addr uint64, tag uint8, maxLen uint32) {
	tlp, err := genTLP.NewMRd(reqID, tag, addr, maxLen)
	if err != nil {
		log.Fatal(err)
	}

	// fmt.Printf("Reading Address: 0x%x\n", addr)
	rw.Write(tlp.ToBytes())

	b := make([]byte, 4096)

	time.Sleep(5 * time.Microsecond)
	rw.Read(b)
	// Parse response completion
	data, err := genTLP.NewCplFromBytes(b)
	if err != nil {
		fmt.Println(addr)
		log.Fatal(err)
	}
	fmt.Printf("Status: %x\n", data.Status)

	if data.Status == 0 {
		for i := 0; i < data.DataLength(); i++ {
			if data.Data[i] != 0 && data.Data[i] != 0xff {
				fmt.Printf("Reading Address: 0x%x\n", addr)
				fmt.Println("Data:")
				fmt.Printf(hex.Dump(data.Data[:data.DataLength()]))
				break
			}
		}
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
	// Onw device id (from lspci)
	reqID := genTLP.DeviceID{
		Bus:      0x81,
		Device:   0x00,
		Function: 0x00,
	}

	/*
		We cant read the config space from DMA???? It returns FFFFFFFF
		addr := uint64(0xe00c4000)
	*/

	/*
		     * Reads from 0x3FFD00000000 give status 1 aka UnsupportedRequest
			 * Reads from 0xFD00000000 give status 4 aka CompleterAbort
			 * 0x3FFD00000000 is the Hyper Transport Area
			 *   0xFD00000000 is the Legacy Hyper Transport Area

			 * 0xFD00000000 gives
			 * [ 2456.404072] iommu ivhd0: AMD-Vi: Event logged [INVALID_DEVICE_REQUEST device=0000:81:00.0 pasid=0x00000 address=0xfd00000000 flags=0x0000]
	*/

	addr := uint64(0xFFFFdf9108000)
	addr = uint64(0xFDF9100000)
	addr = addr + 0x40
	addr = uint64(0x00084eb00000)

	/* 	tag := uint8(0x80)
	   	for i := addr; i < 0x00084ec00000; i += 0x100 {
	   		createAndSendTLP(rw, reqID, i, tag, 0x100)
	   		tag++
	   	} */

	tag := uint8(0x15)
	addrOfOneCore := uint64(0x84eb00000)
	// addrOfOneCore := uint64(0xffffdf9108000)
	for i := addrOfOneCore; i < 0x84eb01000; i += 0x1000 {
		createAndSendTLP(rw, reqID, i, tag, 0x100)
		tag++
		tlp, err := genTLP.NewMWr(reqID, i, []byte{0xff, 0xff, 0xff, 0xff})
		if err != nil {
			log.Fatal(err)
		}
		rw.Write(tlp.ToBytes())
	}
	// createAndSendTLP(rw, reqID, 0xFdf9108000, tag, 0x100)
	// createAndSendTLP(rw, reqID, 0x75d30000, tag, 0x100)
}

//[  139.679646] intel_sgx INT0E0C:00: EPC bank 0x2000c00000-0x207f800000
//[  139.679656] intel_sgx INT0E0C:00: EPC bank 0x4000c00000-0x4080000000
//[  139.679662] intel_sgx INT0E0C:00: EPC bank 0x6000c00000-0x6080000000
//[  139.679668] intel_sgx INT0E0C:00: EPC bank 0x8000c00000-0x8080000000
