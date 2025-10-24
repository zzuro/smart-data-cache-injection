package main

import (
	"crypto/rand"
	"fmt"
	"log"

	"C"

	"github.com/Kakashiiiiy/go-pci-lib/screamer"
	genTLP "github.com/Kakashiiiiy/go-pci-lib/tlp"
	"go.uber.org/zap"
)
import (
	"encoding/hex"
	"os"
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

		if len(data.Data) >= 4 && data.Data[0] == 0xef && data.Data[1] == 0xbe && data.Data[2] == 0xad && data.Data[3] == 0xde {
			fmt.Printf("Reading Address: 0x%x\n", addr)
			fmt.Println("Data starts with deadbeef")
			fmt.Print(hex.Dump(data.Data[:data.DataLength()]))
			fmt.Print("\n")
			os.Exit(0)

		}

		//foundNonFF := false
		//for _, v := range data.Data[:data.DataLength()] {
		//	if v != 0xFF && v != 0x00 {
		//		foundNonFF = true
		//		break
		//	}
		//}
		//if foundNonFF {
		//	fmt.Printf("Non-0xFF value found at address: 0x%x\n", addr)
		//	fmt.Print(hex.Dump(data.Data[:data.DataLength()]))
		//	//os.Exit(0)
		//}
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

func perform_sdci(reqID genTLP.DeviceID, rw screamer.TLPController, tag uint16, addr uint64, data []byte) {

	tlp, err := genTLP.NewMWr(reqID, addr, data)
	if err != nil {
		log.Fatal(err)
	}

	tlp.Address = genTLP.Address((addr &^ 0x3) | 0x2)
	tlp.TH = true
	tlp.Tag = uint8(tag)
	rw.Write(tlp.ToBytes())

}

//export sdci_addr
func sdci_addr(tag uint16, addr uint64) uint32 {
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

	data := make([]byte, 64)
	_, err = rand.Read(data)
	if err != nil {
		log.Fatal(err)
	}

	//value := uint32(data[3])<<24 | uint32(data[2])<<16 | uint32(data[1])<<8 | uint32(data[0])

	tlp, err := genTLP.NewMWr(reqID, addr, data)
	if err != nil {
		log.Fatal(err)
	}

	tlp.Address = genTLP.Address((addr &^ 0x3) | 0x2) // Indicates frequent read and/or write access to data by device
	tlp.NS = true
	tlp.TH = true
	tlp.Tag = uint8(tag)
	rw.Write(tlp.ToBytes())
	return uint32(0)
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

	/*data := make([]byte, 64)
	_, err = rand.Read(data)
	if err != nil {
		log.Fatal(err)
	}

	for i := 0; i < 256; i++ {
		for j := 0; j < 1; j++ {
			addr := uint64(0)
			if j == 0 {
				addr = uint64(0xb0000000)
			} else {
				addr = uint64(0xfd790000 + j*0x1000)
			}

			tlp, err := genTLP.NewMWr(reqID, addr, data)
			if err != nil {
				log.Fatal(err)
			}

			tlp.Address = genTLP.Address((addr &^ 0x3) | 0x2)
			tlp.NS = true
			tlp.TH = true
			tlp.Tag = uint8(0x1)

			refix := make([]byte, 4)
			prefix[0] = tlp.ToBytes()[0]
			prefix[1] = 0x0 //extended TLP
			tlp_bytes := append(prefix, tlp.ToBytes()...)
			fmt.Print(hex.Dump(tlp_bytes))
			rw.Write(tlp.ToBytes())
		}
	}*/

	//Intresting: 0xd1220000, 0xd1221000, 0xd1024000, 0xd1227000

	/*data := make([]byte, 0x100)
	_, err = rand.Read(data)
	if err != nil {
		log.Fatal(err)
	}

	tag := uint8(0x1)
	startAddr := uint64(0xc0000000) //0xd0000000 - 0xd02ff000, 0xd1009000, ==0xd1024000, 0xd1028000, 0xd102b000, ==0xd1220000, ==0xd1221000, 0xd1227000
	endAddr := uint64(0xcfffffff)   //uint64(0x100000000)  // 4 GB (0xc0000000 + 100*1024*1024)
	for addr := startAddr; addr < endAddr; addr += 0x1000 {
		createAndSendReadRequest(rw, reqID, addr, tag, 0x100)
		time.Sleep(5 * time.Microsecond)
		tag++

		/*tlp, err := genTLP.NewMWr(reqID, addr, data)
		if err != nil {
			log.Fatal(err)
		}

		tlp.Tag = uint8(0x1)
		tlp.NS = true
		tlp.Address = genTLP.Address(addr)
		rw.Write(tlp.ToBytes())
	}
	fmt.Println("Done")*/

	tag := uint8(0x1)
	addr := uint64(0xc0000000)
	createAndSendReadRequest(rw, reqID, addr, tag, 0x100)
	return

	// data := make([]byte, 0x100)
	// _, err = rand.Read(data)
	// if err != nil {
	// 	log.Fatal(err)
	// }

	// for i := 0; i < 1; i++ {
	// 	tlp, err := genTLP.NewMWr(reqID, addr, data)
	// 	if err != nil {
	// 		log.Fatal(err)
	// 	}

	// 	tlp.Tag = uint8(i)
	// 	tlp.Address = genTLP.Address((addr &^ 0x3) | 0x2) // Indicates frequent read and/or write access to data by device
	// 	//tlp.NS = true
	// 	//tlp.TH = true
	// 	rw.Write(tlp.ToBytes())
	// }

	// // tag = uint8(0x2)
	// // createAndSendReadRequest(rw, reqID, addr, tag, 0x100)

}
