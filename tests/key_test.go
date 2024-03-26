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

var _ = Describe("Keyspace", Ordered, func() {
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
	It("Exists", func() {
		n, err := client.Exists(ctx, "key1").Result()
		Expect(err).NotTo(HaveOccurred())
		Expect(n).To(Equal(int64(0)))

		set := client.Set(ctx, "key1", "value1", 0)
		Expect(set.Err()).NotTo(HaveOccurred())
		Expect(set.Val()).To(Equal("OK"))

		n, err = client.Exists(ctx, "key1").Result()
		Expect(err).NotTo(HaveOccurred())
		Expect(n).To(Equal(int64(1)))

		set = client.Set(ctx, "key2", "value2", 0)
		Expect(set.Err()).NotTo(HaveOccurred())
		Expect(set.Val()).To(Equal("OK"))

		n, err = client.Exists(ctx, "key1", "key2", "notExistKey").Result()
		Expect(err).NotTo(HaveOccurred())
		Expect(n).To(Equal(int64(2)))

		_, err = client.Del(ctx, "key1", "key2").Result()
		Expect(err).NotTo(HaveOccurred())
	})

	It("Del", func() {
		set := client.Set(ctx, "key1", "value1", 0)
		Expect(set.Err()).NotTo(HaveOccurred())
		Expect(set.Val()).To(Equal("OK"))

		set = client.Set(ctx, "key2", "value2", 0)
		Expect(set.Err()).NotTo(HaveOccurred())
		Expect(set.Val()).To(Equal("OK"))

		n, err := client.Del(ctx, "key1", "key2", "notExistKey").Result()
		Expect(err).NotTo(HaveOccurred())
		Expect(n).To(Equal(int64(2)))

		n, err = client.Exists(ctx, "key1", "key2").Result()
		Expect(err).NotTo(HaveOccurred())
		Expect(n).To(Equal(int64(0)))
	})

	// pikiwidb should treat numbers other than base-10 as strings
	It("base", func() {
		set := client.Set(ctx, "key", "0b1", 0)
		Expect(set.Err()).NotTo(HaveOccurred())
		Expect(set.Val()).To(Equal("OK"))

		get := client.Get(ctx, "key")
		Expect(get.Err()).NotTo(HaveOccurred())
		Expect(get.Val()).To(Equal("0b1"))

		set = client.Set(ctx, "key", "011", 0)
		Expect(set.Err()).NotTo(HaveOccurred())
		Expect(set.Val()).To(Equal("OK"))

		get = client.Get(ctx, "key")
		Expect(get.Err()).NotTo(HaveOccurred())
		Expect(get.Val()).To(Equal("011"))

		set = client.Set(ctx, "key", "0xA", 0)
		Expect(set.Err()).NotTo(HaveOccurred())
		Expect(set.Val()).To(Equal("OK"))

		get = client.Get(ctx, "key")
		Expect(get.Err()).NotTo(HaveOccurred())
		Expect(get.Val()).To(Equal("0xA"))

		del := client.Del(ctx, "key")
		Expect(del.Err()).NotTo(HaveOccurred())
	})

	It("should pexpire", func() {
		Expect(client.Set(ctx, DefaultKey, DefaultValue, 0).Val()).To(Equal(OK))
		Expect(client.PExpire(ctx, DefaultKey, 3000*time.Millisecond).Val()).To(Equal(true))
		// Expect(client.PTTL(ctx, DefaultKey).Val()).NotTo(Equal(time.Duration(-2)))

		time.Sleep(4 * time.Second)
		// Expect(client.PTTL(ctx, DefaultKey).Val()).To(Equal(time.Duration(-2)))
		Expect(client.Get(ctx, DefaultKey).Err()).To(MatchError(redis.Nil))
		Expect(client.Exists(ctx, DefaultKey).Val()).To(Equal(int64(0)))

		Expect(client.Do(ctx, "pexpire", DefaultKey, "err").Err()).To(MatchError("ERR value is not an integer or out of range"))
	})

	It("should expireat", func() {
		Expect(client.Set(ctx, DefaultKey, DefaultValue, 0).Val()).To(Equal(OK))
		Expect(client.ExpireAt(ctx, DefaultKey, time.Now().Add(time.Second*-1)).Val()).To(Equal(true))
		Expect(client.Exists(ctx, DefaultKey).Val()).To(Equal(int64(0)))

	})

	It("should expireat", func() {
		Expect(client.Set(ctx, DefaultKey, DefaultValue, 0).Val()).To(Equal(OK))
		Expect(client.ExpireAt(ctx, DefaultKey, time.Now().Add(time.Second*3)).Val()).To(Equal(true))
		Expect(client.Exists(ctx, DefaultKey).Val()).To(Equal(int64(1)))

		time.Sleep(4 * time.Second)

		Expect(client.Get(ctx, DefaultKey).Err()).To(MatchError(redis.Nil))
		Expect(client.Exists(ctx, DefaultKey).Val()).To(Equal(int64(0)))
	})

	It("should pexpirat", func() {
		Expect(client.Set(ctx, DefaultKey, DefaultValue, 0).Val()).To(Equal(OK))
		Expect(client.PExpireAt(ctx, DefaultKey, time.Now().Add(time.Second*-1)).Val()).To(Equal(true))
		Expect(client.Exists(ctx, DefaultKey).Val()).To(Equal(int64(0)))

	})

	It("should pexpirat", func() {
		Expect(client.Set(ctx, DefaultKey, DefaultValue, 0).Val()).To(Equal(OK))
		Expect(client.PExpireAt(ctx, DefaultKey, time.Now().Add(time.Second*3)).Val()).To(Equal(true))
		Expect(client.Exists(ctx, DefaultKey).Val()).To(Equal(int64(1)))

		time.Sleep(4 * time.Second)

		Expect(client.Get(ctx, DefaultKey).Err()).To(MatchError(redis.Nil))
		Expect(client.Exists(ctx, DefaultKey).Val()).To(Equal(int64(0)))

	})

	It("persist", func() {
		// return 0 if key does not exist
		Expect(client.Persist(ctx, DefaultKey).Val()).To(Equal(false))

		// return 0 if key does not have an associated timeout
		Expect(client.Set(ctx, DefaultKey, DefaultValue, 0).Val()).To(Equal(OK))
		Expect(client.Persist(ctx, DefaultKey).Val()).To(Equal(false))

		// return 1 if the timueout was set
		Expect(client.PExpireAt(ctx, DefaultKey, time.Now().Add(time.Second*3)).Val()).To(Equal(true))
		Expect(client.Persist(ctx, DefaultKey).Val()).To(Equal(true))
		time.Sleep(5 * time.Second)
		Expect(client.Exists(ctx, DefaultKey).Val()).To(Equal(int64(1)))

		// multi data type
		Expect(client.LPush(ctx, DefaultKey, "l").Err()).NotTo(HaveOccurred())
		Expect(client.HSet(ctx, DefaultKey, "h", "h").Err()).NotTo(HaveOccurred())
		Expect(client.SAdd(ctx, DefaultKey, "s").Err()).NotTo(HaveOccurred())
		Expect(client.ZAdd(ctx, DefaultKey, redis.Z{Score: 1, Member: "z"}).Err()).NotTo(HaveOccurred())
		Expect(client.Set(ctx, DefaultKey, DefaultValue, 0).Val()).To(Equal(OK))
		Expect(client.PExpireAt(ctx, DefaultKey, time.Now().Add(time.Second*1000)).Err()).NotTo(HaveOccurred())
		Expect(client.Persist(ctx, DefaultKey).Err()).NotTo(HaveOccurred())

		// del keys
		Expect(client.PExpireAt(ctx, DefaultKey, time.Now().Add(time.Second*1)).Err()).NotTo(HaveOccurred())
		time.Sleep(2 * time.Second)
	})

	It("keys", func() {
		// empty
		Expect(client.Keys(ctx, "*").Val()).To(Equal([]string{}))
		Expect(client.Keys(ctx, "dummy").Val()).To(Equal([]string{}))
		Expect(client.Keys(ctx, "dummy*").Val()).To(Equal([]string{}))

		Expect(client.Set(ctx, "a1", "v1", 0).Val()).To(Equal(OK))
		Expect(client.Set(ctx, "k1", "v1", 0).Val()).To(Equal(OK))
		Expect(client.SAdd(ctx, "k2", "v2").Val()).To(Equal(int64(1)))
		Expect(client.HSet(ctx, "k3", "k3", "v3").Val()).To(Equal(int64(1)))
		Expect(client.LPush(ctx, "k4", "v4").Val()).To(Equal(int64(1)))
		Expect(client.ZAdd(ctx, "k5", redis.Z{Score: 1, Member: "v5"}).Val()).To(Equal(int64(1)))

		// all
		Expect(client.Keys(ctx, "*").Val()).To(Equal([]string{"a1", "k1", "k3", "k4", "k5", "k2"}))

		// pattern
		Expect(client.Keys(ctx, "k*").Val()).To(Equal([]string{"k1", "k3", "k4", "k5", "k2"}))
		Expect(client.Keys(ctx, "k1").Val()).To(Equal([]string{"k1"}))

		// del keys
		Expect(client.Del(ctx, "a1", "k1", "k2", "k3", "k4", "k5").Err()).NotTo(HaveOccurred())
	})
})
