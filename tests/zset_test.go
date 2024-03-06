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

var _ = Describe("Zset", Ordered, func() {
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
	It("Cmd ZADD", func() {
		log.Println("Cmd ZADD Begin")
		Expect(client.ZAdd(ctx, "myset", redis.Z{Score: 1, Member: "one"}).Val()).NotTo(Equal("FooBar"))
	})

	It("should ZRemRangeByScore", func() {
		err := client.ZAdd(ctx, "zset", redis.Z{Score: 1, Member: "one"}).Err()
		Expect(err).NotTo(HaveOccurred())
		err = client.ZAdd(ctx, "zset", redis.Z{Score: 2, Member: "two"}).Err()
		Expect(err).NotTo(HaveOccurred())
		err = client.ZAdd(ctx, "zset", redis.Z{Score: 3, Member: "three"}).Err()
		Expect(err).NotTo(HaveOccurred())

		zRemRangeByScore := client.ZRemRangeByScore(ctx, "zset", "-inf", "(2")
		Expect(zRemRangeByScore.Err()).NotTo(HaveOccurred())
		Expect(zRemRangeByScore.Val()).To(Equal(int64(1)))

		vals, err := client.ZRangeWithScores(ctx, "zset", 0, -1).Result()
		Expect(err).NotTo(HaveOccurred())
		Expect(vals).To(Equal([]redis.Z{{
			Score:  2,
			Member: "two",
		}, {
			Score:  3,
			Member: "three",
		}}))
	})
})
