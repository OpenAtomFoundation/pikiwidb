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

	s2s := map[string]string{
		"key_1": "value_1",
		"key_2": "value_2",
		"key_3": "value_3",
		"key_4": "value_4",
		"key_5": "value_5",
		"key_6": "value_6",
	}

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

		Expect(client.LPush(ctx, DefaultKey, s2s["key_2"]).Val()).To(Equal(int64(1)))
		Expect(client.LPush(ctx, DefaultKey, s2s["key_1"]).Val()).To(Equal(int64(2)))

		Expect(client.LRange(ctx, DefaultKey, 0, -1).Val()).To(Equal([]string{s2s["key_1"], s2s["key_2"]}))

		//del
		del := client.Del(ctx, DefaultKey)
		Expect(del.Err()).NotTo(HaveOccurred())
	})
	It("Cmd RPUSH", func() {
		log.Println("Cmd RPUSH Begin")
		Expect(client.RPush(ctx, DefaultKey, s2s["key_1"]).Val()).To(Equal(int64(1)))
		Expect(client.RPush(ctx, DefaultKey, s2s["key_2"]).Val()).To(Equal(int64(2)))

		Expect(client.LRange(ctx, DefaultKey, 0, -1).Val()).To(Equal([]string{s2s["key_1"], s2s["key_2"]}))
		//del
		del := client.Del(ctx, DefaultKey)
		Expect(del.Err()).NotTo(HaveOccurred())
	})

	It("should RPop", func() {
		rPush := client.RPush(ctx, DefaultKey, s2s["key_1"])
		Expect(rPush.Err()).NotTo(HaveOccurred())
		rPush = client.RPush(ctx, DefaultKey, s2s["key_2"])
		Expect(rPush.Err()).NotTo(HaveOccurred())
		rPush = client.RPush(ctx, DefaultKey, s2s["key_3"])
		Expect(rPush.Err()).NotTo(HaveOccurred())

		rPop := client.RPop(ctx, DefaultKey)
		Expect(rPop.Err()).NotTo(HaveOccurred())
		Expect(rPop.Val()).To(Equal(s2s["key_3"]))

		lRange := client.LRange(ctx, DefaultKey, 0, -1)
		Expect(lRange.Err()).NotTo(HaveOccurred())
		Expect(lRange.Val()).To(Equal([]string{s2s["key_1"], s2s["key_2"]}))

		err := client.Do(ctx, "RPOP", DefaultKey, 1, 2).Err()

		Expect(err).To(MatchError(ContainSubstring("ERR wrong number of arguments for 'rpop' command")))
		//del
		del := client.Del(ctx, DefaultKey)
		Expect(del.Err()).NotTo(HaveOccurred())
	})

	It("Cmd LRem", func() {
		rPush := client.RPush(ctx, DefaultKey, s2s["key_1"])
		Expect(rPush.Err()).NotTo(HaveOccurred())
		rPush = client.RPush(ctx, DefaultKey, s2s["key_1"])
		Expect(rPush.Err()).NotTo(HaveOccurred())
		rPush = client.RPush(ctx, DefaultKey, s2s["key_2"])
		Expect(rPush.Err()).NotTo(HaveOccurred())
		rPush = client.RPush(ctx, DefaultKey, s2s["key_1"])
		Expect(rPush.Err()).NotTo(HaveOccurred())

		lRem := client.LRem(ctx, DefaultKey, -2, s2s["key_1"])
		Expect(lRem.Err()).NotTo(HaveOccurred())
		Expect(lRem.Val()).To(Equal(int64(2)))

		lRange := client.LRange(ctx, DefaultKey, 0, -1)
		Expect(lRange.Err()).NotTo(HaveOccurred())
		Expect(lRange.Val()).To(Equal([]string{s2s["key_1"], s2s["key_2"]}))

		//del
		del := client.Del(ctx, DefaultKey)
		Expect(del.Err()).NotTo(HaveOccurred())
	})

	It("should LTrim", func() {
		rPush := client.RPush(ctx, DefaultKey, s2s["key_1"])
		Expect(rPush.Err()).NotTo(HaveOccurred())
		rPush = client.RPush(ctx, DefaultKey, s2s["key_2"])
		Expect(rPush.Err()).NotTo(HaveOccurred())
		rPush = client.RPush(ctx, DefaultKey, s2s["key_3"])
		Expect(rPush.Err()).NotTo(HaveOccurred())

		lTrim := client.LTrim(ctx, DefaultKey, 1, -1)
		Expect(lTrim.Err()).NotTo(HaveOccurred())
		Expect(lTrim.Val()).To(Equal(OK))

		lRange := client.LRange(ctx, DefaultKey, 0, -1)
		Expect(lRange.Err()).NotTo(HaveOccurred())
		Expect(lRange.Val()).To(Equal([]string{s2s["key_2"], s2s["key_3"]}))
		// del
		del := client.Del(ctx, DefaultKey)
		Expect(del.Err()).NotTo(HaveOccurred())
	})

	It("should LSet", func() {
		rPush := client.RPush(ctx, DefaultKey, s2s["key_1"])
		Expect(rPush.Err()).NotTo(HaveOccurred())
		rPush = client.RPush(ctx, DefaultKey, s2s["key_2"])
		Expect(rPush.Err()).NotTo(HaveOccurred())
		rPush = client.RPush(ctx, DefaultKey, s2s["key_3"])
		Expect(rPush.Err()).NotTo(HaveOccurred())

		lSet := client.LSet(ctx, DefaultKey, 0, s2s["key_4"])
		Expect(lSet.Err()).NotTo(HaveOccurred())
		Expect(lSet.Val()).To(Equal(OK))

		lSet = client.LSet(ctx, DefaultKey, -2, s2s["key_5"])
		Expect(lSet.Err()).NotTo(HaveOccurred())
		Expect(lSet.Val()).To(Equal(OK))

		lRange := client.LRange(ctx, DefaultKey, 0, -1)
		Expect(lRange.Err()).NotTo(HaveOccurred())
		Expect(lRange.Val()).To(Equal([]string{s2s["key_4"], s2s["key_5"], s2s["key_3"]}))

		// del
		del := client.Del(ctx, DefaultKey)
		Expect(del.Err()).NotTo(HaveOccurred())
	})

	It("should LInsert", func() {
		rPush := client.RPush(ctx, DefaultKey, s2s["key_1"])
		Expect(rPush.Err()).NotTo(HaveOccurred())
		rPush = client.RPush(ctx, DefaultKey, s2s["key_2"])
		Expect(rPush.Err()).NotTo(HaveOccurred())

		lInsert := client.LInsert(ctx, DefaultKey, "BEFORE", s2s["key_2"], s2s["key_3"])
		Expect(lInsert.Err()).NotTo(HaveOccurred())
		Expect(lInsert.Val()).To(Equal(int64(3)))

		lRange := client.LRange(ctx, DefaultKey, 0, -1)
		Expect(lRange.Err()).NotTo(HaveOccurred())
		Expect(lRange.Val()).To(Equal([]string{s2s["key_1"], s2s["key_3"], s2s["key_2"]}))

		// del
		del := client.Del(ctx, DefaultKey)
		Expect(del.Err()).NotTo(HaveOccurred())
	})
})
