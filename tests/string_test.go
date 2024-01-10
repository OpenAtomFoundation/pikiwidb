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

var _ = Describe("String", Ordered, func() {
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

	s2i := map[string]int{
		"ikey_1": 1, "ikey_2": 2, "ikey_3": 3,
		"ikey_4": 4, "ikey_5": 5, "ikey_6": 6,
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
		log.Println("before")
		client = s.NewClient()
	})

	// nodes that run after the spec's subject(It).
	AfterEach(func() {
		log.Println("after")
		err := client.Close()
		if err != nil {
			log.Println("Close client conn fail.", err.Error())
			return
		}
	})

	//TODO(dingxiaoshuai) Add more test cases.
	It("Cmd SET & GET", func() {
		log.Println("Cmd SET & GET Test Begin")
		{
			for k := range s2s {
				r, e := client.Get(ctx, k).Result()
				Expect(e).To(MatchError(redis.Nil))
				Expect(r).To(Equal(Nil))
			}

			for k := range s2i {
				r, e := client.Get(ctx, k).Result()
				Expect(e).To(MatchError(redis.Nil))
				Expect(r).To(Equal(Nil))
			}
		}

		{
			for k, v := range s2s {
				r, e := client.Set(ctx, k, v, 0).Result()
				Expect(e).NotTo(HaveOccurred())
				Expect(r).To(Equal(OK))
			}

			for k, v := range s2i {
				r, e := client.Set(ctx, k, v, 0).Result()
				Expect(e).NotTo(HaveOccurred())
				Expect(r).To(Equal(OK))
			}
		}

		{
			for k, v := range s2s {
				r, e := client.Get(ctx, k).Result()
				Expect(e).NotTo(HaveOccurred())
				Expect(r).To(Equal(v))
			}

			for k, v := range s2i {
				r, e := client.Get(ctx, k).Result()
				Expect(e).NotTo(HaveOccurred())
				Expect(r).To(Equal(strconv.Itoa(v)))
			}
		}

		{
			for k := range s2s {
				_, e := client.Del(ctx, k).Result()
				Expect(e).NotTo(HaveOccurred())
				//TODO(dingxiaoshuai) delete key
				//Expect(r).To(Equal(1))
			}

			for k := range s2i {
				_, e := client.Del(ctx, k).Result()
				Expect(e).NotTo(HaveOccurred())
				//TODO(dingxiaoshuai) delete key
				//Expect(r).To(Equal(1))
			}
		}
	})

	It("Cmd Append", func() {
		log.Println("Cmd Append Test Begin")
	})

	It("Cmd INCR", func() {
		log.Println("Cmd INCR Test Begin")
	})
})
