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

var _ = Describe("Hash", Ordered, func() {
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
	It("Cmd HSET", func() {
		log.Println("Cmd HSET Begin")
		Expect(client.HSet(ctx, "myhash", "one").Val()).NotTo(Equal("FooBar"))
	})

	It("Cmd HRandField Test", func() {
		// set test data
		key := "hrandfield"
		kvs := map[string]string{
			"field0": "value0",
			"field1": "value1",
			"field2": "value2",
		}
		for f, v := range kvs {
			client.HSet(ctx, key, f, v)
		}

		num_test := 10
		for i := 0; i < num_test; i++ {
			// count < hlen
			{
				// without values
				fields := client.HRandField(ctx, key, 2).Val()
				Expect(len(fields)).To(Equal(2))
				Expect(fields[0]).ToNot(Equal(fields[1]))
				for _, field := range fields {
					_, ok := kvs[field]
					Expect(ok).To(BeTrue())
				}

				// with values
				fvs := client.HRandFieldWithValues(ctx, key, 2).Val()
				Expect(len(fvs)).To(Equal(2))
				Expect(fvs[0].Key).ToNot(Equal(fvs[1].Key))
				for _, fv := range fvs {
					val, ok := kvs[fv.Key]
					Expect(ok).To(BeTrue())
					Expect(val).To(Equal(fv.Value))
				}
			}

			// count > hlen
			{
				fields := client.HRandField(ctx, key, 10).Val()
				Expect(len(fields)).To(Equal(3))
				Expect(fields[0]).ToNot(Equal(fields[1]))
				Expect(fields[2]).ToNot(Equal(fields[1]))
				Expect(fields[0]).ToNot(Equal(fields[2]))
				for _, field := range fields {
					_, ok := kvs[field]
					Expect(ok).To(BeTrue())
				}

				fvs := client.HRandFieldWithValues(ctx, key, 10).Val()
				Expect(len(fvs)).To(Equal(3))
				Expect(fvs[0].Key).ToNot(Equal(fvs[1].Key))
				Expect(fvs[2].Key).ToNot(Equal(fvs[1].Key))
				Expect(fvs[0].Key).ToNot(Equal(fvs[2].Key))
				for _, fv := range fvs {
					val, ok := kvs[fv.Key]
					Expect(ok).To(BeTrue())
					Expect(val).To(Equal(fv.Value))
				}
			}

			// count < 0
			{
				fields := client.HRandField(ctx, key, -10).Val()
				Expect(len(fields)).To(Equal(10))
				for _, field := range fields {
					_, ok := kvs[field]
					Expect(ok).To(BeTrue())
				}

				fvs := client.HRandFieldWithValues(ctx, key, -10).Val()
				Expect(len(fvs)).To(Equal(10))
				for _, fv := range fvs {
					val, ok := kvs[fv.Key]
					Expect(ok).To(BeTrue())
					Expect(val).To(Equal(fv.Value))
				}
			}
		}

		// the key not exist
		res1 := client.HRandField(ctx, "not_exist_key", 1).Val()
		Expect(len(res1)).To(Equal(0))
	})
})
