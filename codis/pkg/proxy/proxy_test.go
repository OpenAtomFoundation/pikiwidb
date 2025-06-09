// Copyright 2016 CodisLabs. All Rights Reserved.
// Licensed under the MIT (MIT-LICENSE.txt) license.

package proxy

import (
	"strconv"
	"testing"
	"time"

	"pika/codis/v2/pkg/models"
	"pika/codis/v2/pkg/utils/assert"
	"pika/codis/v2/pkg/utils/log"
)

var config = newProxyConfig()

func init() {
	log.SetLevel(log.LevelError)
}

func newProxyConfig() *Config {
	config := NewDefaultConfig()
	config.ProxyHeapPlaceholder = 0
	config.ProxyMaxOffheapBytes = 0
	return config
}

func openProxy(proxy_port, admin_port int) (*Proxy, string) {
	config.ProxyAddr = "0.0.0.0:" + strconv.Itoa(1024+proxy_port)
	config.AdminAddr = "0.0.0.0:" + strconv.Itoa(1024+admin_port)

	models.SetMaxSlotNum(config.MaxSlotNum)
	s, err := New(config)
	assert.MustNoError(err)

	var c = NewApiClient(s.Model().AdminAddr)
	for retry := 0; retry < 10; retry++ {
		_, err = c.Model()
		if err == nil {
			break
		}
		time.Sleep(100 * time.Millisecond)
	}
	return s, s.Model().AdminAddr
}

func TestModel(x *testing.T) {
	s, addr := openProxy(0, 1)
	defer s.Close()

	var c = NewApiClient(addr)

	p, err := c.Model()
	assert.MustNoError(err)
	assert.Must(p.Token == s.Model().Token)
	assert.Must(p.ProductName == config.ProductName)
}

func TestStats(x *testing.T) {
	s, addr := openProxy(2, 3)
	defer s.Close()

	var c = NewApiClient(addr)

	c.SetXAuth(config.ProductName, config.ProductAuth, "")
	_, err1 := c.StatsSimple()
	assert.Must(err1 != nil)

	c.SetXAuth(config.ProductName, config.ProductAuth, s.Model().Token)
	_, err2 := c.Stats(0)
	assert.MustNoError(err2)
}

func verifySlots(c *ApiClient, expect map[int]*models.Slot) {
	slots, err := c.Slots()
	assert.MustNoError(err)

	assert.Must(len(slots) == models.GetMaxSlotNum())

	for i, slot := range expect {
		if slot != nil {
			assert.Must(slots[i].Id == i)
			assert.Must(slot.Locked == slots[i].Locked)
			assert.Must(slot.BackendAddr == slots[i].BackendAddr)
			assert.Must(slot.MigrateFrom == slots[i].MigrateFrom)
		}
	}
}

func TestFillSlot(x *testing.T) {
	s, addr := openProxy(3, 4)
	defer s.Close()

	var c = NewApiClient(addr)
	c.SetXAuth(config.ProductName, config.ProductAuth, s.Model().Token)

	expect := make(map[int]*models.Slot)

	for i := 0; i < 16; i++ {
		slot := &models.Slot{
			Id:          i,
			Locked:      i%2 == 0,
			BackendAddr: "x.x.x.x:xxxx",
		}
		assert.MustNoError(c.FillSlots(slot))
		expect[i] = slot
	}
	verifySlots(c, expect)

	slots := []*models.Slot{}
	for i := 0; i < 16; i++ {
		slot := &models.Slot{
			Id:          i,
			Locked:      i%2 != 0,
			BackendAddr: "y.y.y.y:yyyy",
			MigrateFrom: "x.x.x.x:xxxx",
		}
		slots = append(slots, slot)
		expect[i] = slot
	}
	assert.MustNoError(c.FillSlots(slots...))
	verifySlots(c, expect)
}

func TestStartAndShutdown(x *testing.T) {
	s, addr := openProxy(5, 6)
	defer s.Close()

	var c = NewApiClient(addr)
	c.SetXAuth(config.ProductName, config.ProductAuth, s.Model().Token)

	expect := make(map[int]*models.Slot)

	for i := 0; i < 16; i++ {
		slot := &models.Slot{
			Id:          i,
			BackendAddr: "x.x.x.x:xxxx",
		}
		assert.MustNoError(c.FillSlots(slot))
		expect[i] = slot
	}
	verifySlots(c, expect)

	err1 := c.Start()
	assert.MustNoError(err1)

	err2 := c.Shutdown()
	assert.MustNoError(err2)

	err3 := c.Start()
	assert.Must(err3 != nil)
}
