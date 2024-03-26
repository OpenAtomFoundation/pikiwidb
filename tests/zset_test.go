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

	It("should ZAdd", func() {
		Expect(client.Del(ctx, "zset").Err()).NotTo(HaveOccurred())
		added, err := client.ZAdd(ctx, "zset", redis.Z{
			Score:  1,
			Member: "one",
		}).Result()
		Expect(err).NotTo(HaveOccurred())
		Expect(added).To(Equal(int64(1)))

		added, err = client.ZAdd(ctx, "zset", redis.Z{
			Score:  1,
			Member: "uno",
		}).Result()
		Expect(err).NotTo(HaveOccurred())
		Expect(added).To(Equal(int64(1)))

		added, err = client.ZAdd(ctx, "zset", redis.Z{
			Score:  2,
			Member: "two",
		}).Result()
		Expect(err).NotTo(HaveOccurred())
		Expect(added).To(Equal(int64(1)))

		added, err = client.ZAdd(ctx, "zset", redis.Z{
			Score:  3,
			Member: "two",
		}).Result()
		Expect(err).NotTo(HaveOccurred())
		Expect(added).To(Equal(int64(0)))

		// 		vals, err := client.ZRangeWithScores(ctx, "zset", 0, -1).Result()
		// 		Expect(err).NotTo(HaveOccurred())
		// 		Expect(vals).To(Equal([]redis.Z{{
		// 			Score:  1,
		// 			Member: "one",
		// 		}, {
		// 			Score:  1,
		// 			Member: "uno",
		// 		}, {
		// 			Score:  3,
		// 			Member: "two",
		// 		}}))
	})

	It("should ZRangeByScore", func() {
		Expect(client.Del(ctx, "zset").Err()).NotTo(HaveOccurred())
		err := client.ZAdd(ctx, "zset", redis.Z{Score: 1, Member: "one"}).Err()
		Expect(err).NotTo(HaveOccurred())
		err = client.ZAdd(ctx, "zset", redis.Z{Score: 2, Member: "two"}).Err()
		Expect(err).NotTo(HaveOccurred())
		err = client.ZAdd(ctx, "zset", redis.Z{Score: 3, Member: "three"}).Err()
		Expect(err).NotTo(HaveOccurred())

		zRangeByScore := client.ZRangeByScore(ctx, "zset", &redis.ZRangeBy{
			Min: "-inf",
			Max: "+inf",
		})
		Expect(zRangeByScore.Err()).NotTo(HaveOccurred())
		Expect(zRangeByScore.Val()).To(Equal([]string{"one", "two", "three"}))

		zRangeByScore = client.ZRangeByScore(ctx, "zset", &redis.ZRangeBy{
			Min: "1",
			Max: "2",
		})
		Expect(zRangeByScore.Err()).NotTo(HaveOccurred())
		Expect(zRangeByScore.Val()).To(Equal([]string{"one", "two"}))

		zRangeByScore = client.ZRangeByScore(ctx, "zset", &redis.ZRangeBy{
			Min: "(1",
			Max: "2",
		})
		Expect(zRangeByScore.Err()).NotTo(HaveOccurred())
		Expect(zRangeByScore.Val()).To(Equal([]string{"two"}))

		zRangeByScore = client.ZRangeByScore(ctx, "zset", &redis.ZRangeBy{
			Min: "(1",
			Max: "(2",
		})
		Expect(zRangeByScore.Err()).NotTo(HaveOccurred())
		Expect(zRangeByScore.Val()).To(Equal([]string{}))
	})

	It("should ZRevRange", func() {
		Expect(client.Del(ctx, "zset").Err()).NotTo(HaveOccurred())
		err := client.ZAdd(ctx, "zset", redis.Z{Score: 1, Member: "one"}).Err()
		Expect(err).NotTo(HaveOccurred())
		err = client.ZAdd(ctx, "zset", redis.Z{Score: 2, Member: "two"}).Err()
		Expect(err).NotTo(HaveOccurred())
		err = client.ZAdd(ctx, "zset", redis.Z{Score: 3, Member: "three"}).Err()
		Expect(err).NotTo(HaveOccurred())

		zRevRange := client.ZRevRange(ctx, "zset", 0, -1)
		Expect(zRevRange.Err()).NotTo(HaveOccurred())
		Expect(zRevRange.Val()).To(Equal([]string{"three", "two", "one"}))

		zRevRange = client.ZRevRange(ctx, "zset", 2, 3)
		Expect(zRevRange.Err()).NotTo(HaveOccurred())
		Expect(zRevRange.Val()).To(Equal([]string{"one"}))

		zRevRange = client.ZRevRange(ctx, "zset", -2, -1)
		Expect(zRevRange.Err()).NotTo(HaveOccurred())
		Expect(zRevRange.Val()).To(Equal([]string{"two", "one"}))
	})

	It("should ZRemRangeByRank", func() {
		err := client.ZAdd(ctx, "zset", redis.Z{Score: 1, Member: "one"}).Err()
		Expect(err).NotTo(HaveOccurred())
		err = client.ZAdd(ctx, "zset", redis.Z{Score: 2, Member: "two"}).Err()
		Expect(err).NotTo(HaveOccurred())
		err = client.ZAdd(ctx, "zset", redis.Z{Score: 3, Member: "three"}).Err()
		Expect(err).NotTo(HaveOccurred())

		zRemRangeByRank := client.ZRemRangeByRank(ctx, "zset", 0, 1)
		Expect(zRemRangeByRank.Err()).NotTo(HaveOccurred())
		Expect(zRemRangeByRank.Val()).To(Equal(int64(2)))

		vals, err := client.ZRangeWithScores(ctx, "zset", 0, -1).Result()
		Expect(err).NotTo(HaveOccurred())
		Expect(vals).To(Equal([]redis.Z{{
			Score:  3,
			Member: "three",
		}}))
	})

	It("should ZRevRangeByScore", func() {
		err := client.ZAdd(ctx, "zset", redis.Z{Score: 1, Member: "one"}).Err()
		Expect(err).NotTo(HaveOccurred())
		err = client.ZAdd(ctx, "zset", redis.Z{Score: 2, Member: "two"}).Err()
		Expect(err).NotTo(HaveOccurred())
		err = client.ZAdd(ctx, "zset", redis.Z{Score: 3, Member: "three"}).Err()
		Expect(err).NotTo(HaveOccurred())

		zRemRangeByRank := client.ZRemRangeByRank(ctx, "zset", 0, 1)
		Expect(zRemRangeByRank.Err()).NotTo(HaveOccurred())
		Expect(zRemRangeByRank.Val()).To(Equal(int64(2)))

		vals, err := client.ZRangeWithScores(ctx, "zset", 0, -1).Result()
		Expect(err).NotTo(HaveOccurred())
		Expect(vals).To(Equal([]redis.Z{{
			Score:  3,
			Member: "three",
		}}))
	})

	It("should ZCard", func() {
		err := client.ZAdd(ctx, "zsetZCard", redis.Z{
			Score:  1,
			Member: "one",
		}).Err()
		Expect(err).NotTo(HaveOccurred())
		err = client.ZAdd(ctx, "zsetZCard", redis.Z{
			Score:  2,
			Member: "two",
		}).Err()
		Expect(err).NotTo(HaveOccurred())

		card, err := client.ZCard(ctx, "zsetZCard").Result()
		Expect(err).NotTo(HaveOccurred())
		Expect(card).To(Equal(int64(2)))
	})

	It("should ZRange", func() {
		err := client.ZAdd(ctx, "zset", redis.Z{Score: 1, Member: "one"}).Err()
		Expect(err).NotTo(HaveOccurred())
		err = client.ZAdd(ctx, "zset", redis.Z{Score: 2, Member: "two"}).Err()
		Expect(err).NotTo(HaveOccurred())
		err = client.ZAdd(ctx, "zset", redis.Z{Score: 3, Member: "three"}).Err()
		Expect(err).NotTo(HaveOccurred())

		zRange := client.ZRange(ctx, "zset", 0, -1)
		Expect(zRange.Err()).NotTo(HaveOccurred())
		Expect(zRange.Val()).To(Equal([]string{"one", "two", "three"}))

		zRange = client.ZRange(ctx, "zset", 2, 3)
		Expect(zRange.Err()).NotTo(HaveOccurred())
		Expect(zRange.Val()).To(Equal([]string{"three"}))

		zRange = client.ZRange(ctx, "zset", -2, -1)
		Expect(zRange.Err()).NotTo(HaveOccurred())
		Expect(zRange.Val()).To(Equal([]string{"two", "three"}))
	})

	It("should ZRangeWithScores", func() {
		err := client.ZAdd(ctx, "zset", redis.Z{Score: 1, Member: "one"}).Err()
		Expect(err).NotTo(HaveOccurred())
		err = client.ZAdd(ctx, "zset", redis.Z{Score: 2, Member: "two"}).Err()
		Expect(err).NotTo(HaveOccurred())
		err = client.ZAdd(ctx, "zset", redis.Z{Score: 3, Member: "three"}).Err()
		Expect(err).NotTo(HaveOccurred())

		vals, err := client.ZRangeWithScores(ctx, "zset", 0, -1).Result()
		Expect(err).NotTo(HaveOccurred())
		Expect(vals).To(Equal([]redis.Z{{
			Score:  1,
			Member: "one",
		}, {
			Score:  2,
			Member: "two",
		}, {
			Score:  3,
			Member: "three",
		}}))

		vals, err = client.ZRangeWithScores(ctx, "zset", 2, 3).Result()
		Expect(err).NotTo(HaveOccurred())
		Expect(vals).To(Equal([]redis.Z{{Score: 3, Member: "three"}}))

		vals, err = client.ZRangeWithScores(ctx, "zset", -2, -1).Result()
		Expect(err).NotTo(HaveOccurred())
		Expect(vals).To(Equal([]redis.Z{{
			Score:  2,
			Member: "two",
		}, {
			Score:  3,
			Member: "three",
		}}))
	})

	It("should ZRangeByScoreWithScoresMap", func() {
		err := client.ZAdd(ctx, "zset", redis.Z{Score: 1, Member: "one"}).Err()
		Expect(err).NotTo(HaveOccurred())
		err = client.ZAdd(ctx, "zset", redis.Z{Score: 2, Member: "two"}).Err()
		Expect(err).NotTo(HaveOccurred())
		err = client.ZAdd(ctx, "zset", redis.Z{Score: 3, Member: "three"}).Err()
		Expect(err).NotTo(HaveOccurred())

		vals, err := client.ZRangeByScoreWithScores(ctx, "zset", &redis.ZRangeBy{
			Min: "-inf",
			Max: "+inf",
		}).Result()
		Expect(err).NotTo(HaveOccurred())
		Expect(vals).To(Equal([]redis.Z{{
			Score:  1,
			Member: "one",
		}, {
			Score:  2,
			Member: "two",
		}, {
			Score:  3,
			Member: "three",
		}}))

		vals, err = client.ZRangeByScoreWithScores(ctx, "zset", &redis.ZRangeBy{
			Min: "1",
			Max: "2",
		}).Result()
		Expect(err).NotTo(HaveOccurred())
		Expect(vals).To(Equal([]redis.Z{{
			Score:  1,
			Member: "one",
		}, {
			Score:  2,
			Member: "two",
		}}))

		vals, err = client.ZRangeByScoreWithScores(ctx, "zset", &redis.ZRangeBy{
			Min: "(1",
			Max: "2",
		}).Result()
		Expect(err).NotTo(HaveOccurred())
		Expect(vals).To(Equal([]redis.Z{{Score: 2, Member: "two"}}))

		vals, err = client.ZRangeByScoreWithScores(ctx, "zset", &redis.ZRangeBy{
			Min: "(1",
			Max: "(2",
		}).Result()
		Expect(err).NotTo(HaveOccurred())
		Expect(vals).To(Equal([]redis.Z{}))
	})

	It("should ZRangeByLex", func() {
		err := client.ZAdd(ctx, "zsetrangebylex", redis.Z{
			Score:  0,
			Member: "a",
		}).Err()
		Expect(err).NotTo(HaveOccurred())
		err = client.ZAdd(ctx, "zsetrangebylex", redis.Z{
			Score:  0,
			Member: "b",
		}).Err()
		Expect(err).NotTo(HaveOccurred())
		err = client.ZAdd(ctx, "zsetrangebylex", redis.Z{
			Score:  0,
			Member: "c",
		}).Err()
		Expect(err).NotTo(HaveOccurred())

		zRangeByLex := client.ZRangeByLex(ctx, "zsetrangebylex", &redis.ZRangeBy{
			Min: "-",
			Max: "+",
		})
		Expect(zRangeByLex.Err()).NotTo(HaveOccurred())
		Expect(zRangeByLex.Val()).To(Equal([]string{"a", "b", "c"}))

		zRangeByLex = client.ZRangeByLex(ctx, "zsetrangebylex", &redis.ZRangeBy{
			Min: "[a",
			Max: "[b",
		})
		Expect(zRangeByLex.Err()).NotTo(HaveOccurred())
		Expect(zRangeByLex.Val()).To(Equal([]string{"a", "b"}))

		zRangeByLex = client.ZRangeByLex(ctx, "zsetrangebylex", &redis.ZRangeBy{
			Min: "(a",
			Max: "[b",
		})
		Expect(zRangeByLex.Err()).NotTo(HaveOccurred())
		Expect(zRangeByLex.Val()).To(Equal([]string{"b"}))

		zRangeByLex = client.ZRangeByLex(ctx, "zsetrangebylex", &redis.ZRangeBy{
			Min: "(a",
			Max: "(b",
		})
		Expect(zRangeByLex.Err()).NotTo(HaveOccurred())
		Expect(zRangeByLex.Val()).To(Equal([]string{}))
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
