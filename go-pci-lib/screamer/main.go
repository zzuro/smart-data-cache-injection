// (c) Ulf Frisk, 2017-2020
// Copyright (C) 2020 Google LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

// Package screamer provides low-level TLP interface for a PCIe screamer device.
// Based on:
// https://github.com/ufrisk/LeechCore/blob/master/leechcore/fpga_libusb.c
// https://github.com/ufrisk/LeechCore/blob/master/leechcore/device_fpga.c

package screamer

import (
	"fmt"

	"go.uber.org/zap"
)

// OpenScreamers enumerates and opens all PCIe screamer devices on the system.
// Caller should Close() all returned Screamers.
func OpenScreamers(logger *zap.Logger) ([]*Screamer, error) {
	ctx := newUSBContext(logger)
	defer ctx.Close()

	devs, err := ctx.usbCtx.OpenDevices(func(vendor, product uint16) bool {
		return vendor == ftdiVendorID && product == ftdiProductID
	})
	if err != nil {
		return nil, fmt.Errorf("%w: OpenDevices failed: %v", ErrUSBOpenFailed, err)
	}

	var ret []*Screamer
	for _, dev := range devs {
		d, err := openScreamer(dev, ctx, logger)
		if err != nil {
			return nil, err
		}
		ret = append(ret, d)
	}
	return ret, nil
}
