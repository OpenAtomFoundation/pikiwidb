/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

package pikiwidb_test

import (
	"math"
	"testing"

	. "github.com/onsi/ginkgo/v2"
	. "github.com/onsi/gomega"
)

const (
	DefaultKey   = "key"
	DefaultValue = "value"
	Max64        = math.MaxUint64
	Min64        = math.MinInt64
	OK           = "OK"
	Nil          = ""
)

// cmd
const (
	kCmdSelect = "select"
)

// err
const (
	kInvalidIndex = "ERR invalid DB index for 'select DB index is out of range'"
)

func TestPikiwidb(t *testing.T) {
	RegisterFailHandler(Fail)
	RunSpecs(t, "Tests Suite")
}
