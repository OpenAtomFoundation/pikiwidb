// Copyright 2016 CodisLabs. All Rights Reserved.
// Licensed under the MIT (MIT-LICENSE.txt) license.

package proxy

import (
	"log"
	"net"
	"strconv"
	"sync"
	"testing"
	"time"

	"pika/codis/v2/pkg/proxy/redis"
	"pika/codis/v2/pkg/utils/assert"
)

func newConnPair(config *Config) (*redis.Conn, *BackendConn) {
	l, err := net.Listen("tcp", "127.0.0.1:0")
	assert.MustNoError(err)
	defer l.Close()

	const bufsize = 128 * 1024

	cc := make(chan *redis.Conn, 1)
	go func() {
		defer close(cc)
		c, err := l.Accept()
		assert.MustNoError(err)
		cc <- redis.NewConn(c, bufsize, bufsize)
	}()

	bc := NewBackendConn(l.Addr().String(), 0, config)
	return <-cc, bc
}

func TestBackend(t *testing.T) {
	config := NewDefaultConfig()
	config.BackendMaxPipeline = 0
	config.BackendSendTimeout.Set(time.Second)
	config.BackendRecvTimeout.Set(time.Minute)

	conn, bc := newConnPair(config)

	var array = make([]*Request, 16384)
	for i := range array {
		array[i] = &Request{Batch: &sync.WaitGroup{}}
	}

	go func() {
		defer conn.Close()
		time.Sleep(time.Millisecond * 300)
		for i, _ := range array {
			_, err := conn.Decode()
			assert.MustNoError(err)
			resp := redis.NewString([]byte(strconv.Itoa(i)))
			assert.MustNoError(conn.Encode(resp, true))
		}
	}()

	defer bc.Close()

	ticker := time.NewTicker(time.Second)
	defer ticker.Stop()
	go func() {
		for i := 0; i < 10; i++ {
			<-ticker.C
		}
		log.Panicf("timeout")
	}()

	for _, r := range array {
		bc.PushBack(r)
	}

	for i, r := range array {
		r.Batch.Wait()
		assert.MustNoError(r.Err)
		assert.Must(r.Resp != nil)
		assert.Must(string(r.Resp.Value) == strconv.Itoa(i))
	}
}

func TestBackendTimeOut(t *testing.T) {
	config := NewDefaultConfig()
	config.BackendMaxPipeline = 3
	config.BackendSendTimeout.Set(10 * time.Second)
	config.BackendRecvTimeout.Set(1 * time.Second)

	conn, bc := newConnPair(config)
	defer bc.Close()

	var array = make([]*Request, 5)
	for i := range array {
		array[i] = &Request{Batch: &sync.WaitGroup{}}
	}

	// mock backend server, sleep 1.1s for 50% requests
	// to simulate request timeout
	go func() {
		defer conn.Close()
		time.Sleep(time.Millisecond * 20)
		for i := range array {
			_, err := conn.Decode()
			if i%2 == 0 {
				time.Sleep(time.Millisecond * 1100)
			}
			assert.MustNoError(err)
			resp := redis.NewString([]byte(strconv.Itoa(i)))
			assert.MustNoError(conn.Encode(resp, true))
		}
	}()

	ticker := time.NewTicker(time.Second)
	defer ticker.Stop()
	go func() {
		for i := 0; i < 10; i++ {
			<-ticker.C
		}
		log.Panicf("timeout")
	}()

	for _, r := range array {
		bc.PushBack(r)
	}

	for i, r := range array {
		r.Batch.Wait()
		if i%2 == 0 {
			// request timeout
			assert.Must(r.Err != nil)
			assert.Must(r.Resp == nil)
		} else {
			// request succeed and response value matches
			assert.MustNoError(r.Err)
			assert.Must(r.Resp != nil)
			assert.Must(string(r.Resp.Value) == strconv.Itoa(i))
		}
	}
}
