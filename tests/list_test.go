/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

package pikiwidb_test

import (
	"context"
	"log"
	"strconv"
	"time"

	. "github.com/onsi/ginkgo/v2"
	. "github.com/onsi/gomega"
	"github.com/redis/go-redis/v9"

	"github.com/OpenAtomFoundation/pikiwidb/tests/util"
)

var _ = Describe("List", Ordered, func() {
	var (
		ctx    = context.TODO()
		s      *util.Server
		client *redis.Client
	)

	// BeforeAll closures will run exactly once before any of the specs
	// within the Ordered container.
	BeforeAll(func() {
		config := util.GetConfPath(false, 0)

		s = util.StartServer(config, map[string]string{"port": strconv.Itoa(7777)}, true)
		Expect(s).NotTo(Equal(nil))
	})

	// AfterAll closures will run exactly once after the last spec has
	// finished running.
	AfterAll(func() {
		err := s.Close()
		if err != nil {
			log.Println("Close Server fail.", err.Error())
			return
		}
	})

	// When running each spec Ginkgo will first run the BeforeEach
	// closure and then the subject closure.Doing so ensures that
	// each spec has a pristine, correctly initialized, copy of the
	// shared variable.
	BeforeEach(func() {
		client = s.NewClient()
		Expect(client.FlushDB(ctx).Err()).NotTo(HaveOccurred())
		time.Sleep(1 * time.Second)
	})

	// nodes that run after the spec's subject(It).
	AfterEach(func() {
		err := client.Close()
		if err != nil {
			log.Println("Close client conn fail.", err.Error())
			return
		}
	})
	//TODO(dingxiaoshuai) Add more test cases.
	It("Cmd LPUSH", func() {
		log.Println("Cmd LPUSH Begin")
		Expect(client.LPush(ctx, "mylistLPUSH", "world").Val()).To(Equal(int64(1)))
		Expect(client.LPush(ctx, "mylistLPUSH", "hello").Val()).To(Equal(int64(2)))

		Expect(client.LRange(ctx, "mylistLPUSH", 0, -1).Val()).To(Equal([]string{"hello", "world"}))

		//del
		del := client.Del(ctx, "mylistLPUSH")
		Expect(del.Err()).NotTo(HaveOccurred())
	})
	It("Cmd RPUSH", func() {
		log.Println("Cmd RPUSH Begin")
		Expect(client.RPush(ctx, "mylistRPUSH", "hello").Val()).To(Equal(int64(1)))
		Expect(client.RPush(ctx, "mylistRPUSH", "world").Val()).To(Equal(int64(2)))

		Expect(client.LRange(ctx, "mylistRPUSH", 0, -1).Val()).To(Equal([]string{"hello", "world"}))
		//del
		del := client.Del(ctx, "mylistRPUSH")
		Expect(del.Err()).NotTo(HaveOccurred())
	})

	It("should RPop", func() {
		rPush := client.RPush(ctx, "list", "one")
		Expect(rPush.Err()).NotTo(HaveOccurred())
		rPush = client.RPush(ctx, "list", "two")
		Expect(rPush.Err()).NotTo(HaveOccurred())
		rPush = client.RPush(ctx, "list", "three")
		Expect(rPush.Err()).NotTo(HaveOccurred())

		rPop := client.RPop(ctx, "list")
		Expect(rPop.Err()).NotTo(HaveOccurred())
		Expect(rPop.Val()).To(Equal("three"))

		lRange := client.LRange(ctx, "list", 0, -1)
		Expect(lRange.Err()).NotTo(HaveOccurred())
		Expect(lRange.Val()).To(Equal([]string{"one", "two"}))

		err := client.Do(ctx, "RPOP", "list", 1, 2).Err()
		Expect(err).To(MatchError(ContainSubstring("ERR wrong number of arguments for 'rpop' command")))

		//del
		del := client.Del(ctx, "list")
		Expect(del.Err()).NotTo(HaveOccurred())
	})

	It("Cmd LRem", func() {
		rPush := client.RPush(ctx, "list", "hello")
		Expect(rPush.Err()).NotTo(HaveOccurred())
		rPush = client.RPush(ctx, "list", "hello")
		Expect(rPush.Err()).NotTo(HaveOccurred())
		rPush = client.RPush(ctx, "list", "key")
		Expect(rPush.Err()).NotTo(HaveOccurred())
		rPush = client.RPush(ctx, "list", "hello")
		Expect(rPush.Err()).NotTo(HaveOccurred())

		lRem := client.LRem(ctx, "list", -2, "hello")
		Expect(lRem.Err()).NotTo(HaveOccurred())
		Expect(lRem.Val()).To(Equal(int64(2)))

		lRange := client.LRange(ctx, "list", 0, -1)
		Expect(lRange.Err()).NotTo(HaveOccurred())
		Expect(lRange.Val()).To(Equal([]string{"hello", "key"}))

		//del
		del := client.Del(ctx, "list")
		Expect(del.Err()).NotTo(HaveOccurred())
	})
})
