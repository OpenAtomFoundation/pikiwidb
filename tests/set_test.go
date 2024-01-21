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

var _ = Describe("Set", Ordered, func() {
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
	It("SUnion", func() {
        sAdd := client.SAdd(ctx, "set1", "a")
        Expect(sAdd.Err()).NotTo(HaveOccurred())
        sAdd = client.SAdd(ctx, "set1", "b")
        Expect(sAdd.Err()).NotTo(HaveOccurred())
        sAdd = client.SAdd(ctx, "set1", "c")
        Expect(sAdd.Err()).NotTo(HaveOccurred())

        sAdd = client.SAdd(ctx, "set2", "c")
        Expect(sAdd.Err()).NotTo(HaveOccurred())
        sAdd = client.SAdd(ctx, "set2", "d")
        Expect(sAdd.Err()).NotTo(HaveOccurred())
        sAdd = client.SAdd(ctx, "set2", "e")
        Expect(sAdd.Err()).NotTo(HaveOccurred())

        sUnion := client.SUnion(ctx, "set1", "set2")
        Expect(sUnion.Err()).NotTo(HaveOccurred())
        Expect(sUnion.Val()).To(HaveLen(5))

        sUnion = client.SUnion(ctx, "nonexistent_set1", "nonexistent_set2")
        Expect(sUnion.Err()).NotTo(HaveOccurred())
        Expect(sUnion.Val()).To(HaveLen(0))
    })

    It("should SUnionStore", func() {
        sAdd := client.SAdd(ctx, "set1", "a")
        Expect(sAdd.Err()).NotTo(HaveOccurred())
        sAdd = client.SAdd(ctx, "set1", "b")
        Expect(sAdd.Err()).NotTo(HaveOccurred())
        sAdd = client.SAdd(ctx, "set1", "c")
        Expect(sAdd.Err()).NotTo(HaveOccurred())

        sAdd = client.SAdd(ctx, "set2", "c")
        Expect(sAdd.Err()).NotTo(HaveOccurred())
        sAdd = client.SAdd(ctx, "set2", "d")
        Expect(sAdd.Err()).NotTo(HaveOccurred())
        sAdd = client.SAdd(ctx, "set2", "e")
        Expect(sAdd.Err()).NotTo(HaveOccurred())

        sUnionStore := client.SUnionStore(ctx, "set", "set1", "set2")
        Expect(sUnionStore.Err()).NotTo(HaveOccurred())
        Expect(sUnionStore.Val()).To(Equal(int64(5)))

        //sMembers := client.SMembers(ctx, "set")
        //Expect(sMembers.Err()).NotTo(HaveOccurred())
        //Expect(sMembers.Val()).To(HaveLen(5))
    })
})
