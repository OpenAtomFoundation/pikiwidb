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

	. "github.com/onsi/ginkgo/v2"
	. "github.com/onsi/gomega"
	"github.com/redis/go-redis/v9"

	"github.com/OpenAtomFoundation/pikiwidb/tests/util"
)

var _ = Describe("Admin", Ordered, func() {
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
	It("Cmd INFO", func() {
		log.Println("Cmd INFO Begin")
		Expect(client.Info(ctx).Val()).NotTo(Equal("FooBar"))
	})

	It("Cmd Select", func() {
		var outRangeNumber = 100

		r, e := client.Set(ctx, DefaultKey, DefaultValue, 0).Result()
		Expect(e).NotTo(HaveOccurred())
		Expect(r).To(Equal(OK))

		r, e = client.Get(ctx, DefaultKey).Result()
		Expect(e).NotTo(HaveOccurred())
		Expect(r).To(Equal(DefaultValue))

		rDo, eDo := client.Do(ctx, kCmdSelect, outRangeNumber).Result()
		Expect(eDo).To(MatchError(kInvalidIndex))

		r, e = client.Get(ctx, DefaultKey).Result()
		Expect(e).NotTo(HaveOccurred())
		Expect(r).To(Equal(DefaultValue))

		rDo, eDo = client.Do(ctx, kCmdSelect, 1).Result()
		Expect(eDo).NotTo(HaveOccurred())
		Expect(rDo).To(Equal(OK))

		r, e = client.Get(ctx, DefaultKey).Result()
		Expect(e).To(MatchError(redis.Nil))
		Expect(r).To(Equal(Nil))

		rDo, eDo = client.Do(ctx, kCmdSelect, 0).Result()
		Expect(eDo).NotTo(HaveOccurred())
		Expect(rDo).To(Equal(OK))

		rDel, eDel := client.Del(ctx, DefaultKey).Result()
		Expect(eDel).NotTo(HaveOccurred())
		Expect(rDel).To(Equal(int64(1)))
	})
})
