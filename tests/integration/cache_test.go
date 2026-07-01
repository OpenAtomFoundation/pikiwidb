package pika_integration

import (
	"context"
	"time"

	. "github.com/bsm/ginkgo/v2"
	. "github.com/bsm/gomega"
	"github.com/redis/go-redis/v9"
)

var _ = Describe("Cache test", func() {
	ctx := context.TODO()
	var client *redis.Client

	BeforeEach(func() {
		client = redis.NewClient(PikaOption(SINGLEADDR))
		Expect(client.FlushDB(ctx).Err()).NotTo(HaveOccurred())
		time.Sleep(1 * time.Second)
	})

	AfterEach(func() {
		Expect(client.Close()).NotTo(HaveOccurred())
	})

	It("should Exists", func() {
		set := client.Set(ctx, "key1", "a", 0)
		Expect(set.Err()).NotTo(HaveOccurred())
		Expect(set.Val()).To(Equal("OK"))

		lPush := client.LPush(ctx, "key2", "b")
		Expect(lPush.Err()).NotTo(HaveOccurred())

		sAdd := client.SAdd(ctx, "key3", "c")
		Expect(sAdd.Err()).NotTo(HaveOccurred())
		Expect(sAdd.Val()).To(Equal(int64(1)))

		n, err := client.Exists(ctx, "key1", "key2", "key3").Result()
		Expect(err).NotTo(HaveOccurred())
		Expect(n).To(Equal(int64(3)))

		get := client.Get(ctx, "key1")
		Expect(get.Err()).NotTo(HaveOccurred())
		Expect(get.Val()).To(Equal("a"))

		n1, err1 := client.Exists(ctx, "key1", "key2", "key3").Result()
		Expect(err1).NotTo(HaveOccurred())
		Expect(n1).To(Equal(int64(3)))
	})

	It("should TTL", func() {
		set := client.Set(ctx, "key1", "bcd", 10*time.Minute)
		Expect(set.Err()).NotTo(HaveOccurred())
		Expect(set.Val()).To(Equal("OK"))
		Expect(client.TTL(ctx, "key1").Val()).NotTo(Equal(int64(-2)))

		get := client.Get(ctx, "key1")
		Expect(get.Err()).NotTo(HaveOccurred())
		Expect(get.Val()).To(Equal("bcd"))
		Expect(client.TTL(ctx, "key1").Val()).NotTo(Equal(int64(-2)))

		_, err := client.Del(ctx, "key1").Result()
		Expect(err).NotTo(HaveOccurred())

		set1 := client.Set(ctx, "key1", "bcd", 10*time.Minute)
		Expect(set1.Err()).NotTo(HaveOccurred())
		Expect(set1.Val()).To(Equal("OK"))
		Expect(client.TTL(ctx, "key1").Val()).NotTo(Equal(int64(-2)))

		mGet := client.MGet(ctx, "key1")
		Expect(mGet.Err()).NotTo(HaveOccurred())
		Expect(mGet.Val()).To(Equal([]interface{}{"bcd"}))

		Expect(client.TTL(ctx, "key1").Val()).NotTo(Equal(int64(-2)))
	})

	It("should TTL effective", func() {
		set := client.Set(ctx, "key1", "a", 10*time.Minute)
		Expect(set.Err()).NotTo(HaveOccurred())
		Expect(set.Val()).To(Equal("OK"))

		set1 := client.Set(ctx, "key2", "b", 10*time.Minute)
		Expect(set1.Err()).NotTo(HaveOccurred())
		Expect(set1.Val()).To(Equal("OK"))

		set2 := client.Set(ctx, "key3", "c", 10*time.Minute)
		Expect(set2.Err()).NotTo(HaveOccurred())
		Expect(set2.Val()).To(Equal("OK"))

		set3 := client.Set(ctx, "key4", "d", 10*time.Minute)
		Expect(set3.Err()).NotTo(HaveOccurred())
		Expect(set3.Val()).To(Equal("OK"))

		get := client.Get(ctx, "key1")
		Expect(get.Err()).NotTo(HaveOccurred())
		Expect(get.Val()).To(Equal("a"))
		Expect(client.TTL(ctx, "key1").Val()).NotTo(Equal(int64(-2)))

		mGet := client.MGet(ctx, "key2")
		Expect(mGet.Err()).NotTo(HaveOccurred())
		Expect(mGet.Val()).To(Equal([]interface{}{"b"}))

		Expect(client.TTL(ctx, "key1").Val()).NotTo(Equal(int64(-2)))
		Expect(client.TTL(ctx, "key2").Val()).NotTo(Equal(int64(-2)))
		Expect(client.TTL(ctx, "key3").Val()).NotTo(Equal(int64(-2)))
		Expect(client.TTL(ctx, "key3").Val()).NotTo(Equal(int64(-2)))

		get1 := client.Get(ctx, "key1")
		Expect(get1.Err()).NotTo(HaveOccurred())
		Expect(get1.Val()).To(Equal("a"))

		get2 := client.Get(ctx, "key2")
		Expect(get2.Err()).NotTo(HaveOccurred())
		Expect(get2.Val()).To(Equal("b"))

		Expect(client.TTL(ctx, "key1").Val()).NotTo(Equal(int64(-2)))
		Expect(client.TTL(ctx, "key2").Val()).NotTo(Equal(int64(-2)))
	})

	It("should mget", func() {
		set := client.Set(ctx, "key1", "a", 10*time.Minute)
		Expect(set.Err()).NotTo(HaveOccurred())
		Expect(set.Val()).To(Equal("OK"))

		set1 := client.Set(ctx, "key2", "b", 10*time.Minute)
		Expect(set1.Err()).NotTo(HaveOccurred())
		Expect(set1.Val()).To(Equal("OK"))

		set2 := client.Set(ctx, "key3", "c", 10*time.Minute)
		Expect(set2.Err()).NotTo(HaveOccurred())
		Expect(set2.Val()).To(Equal("OK"))

		set3 := client.Set(ctx, "key4", "d", 10*time.Minute)
		Expect(set3.Err()).NotTo(HaveOccurred())
		Expect(set3.Val()).To(Equal("OK"))

		get := client.Get(ctx, "key1")
		Expect(get.Err()).NotTo(HaveOccurred())
		Expect(get.Val()).To(Equal("a"))
		Expect(client.TTL(ctx, "key1").Val()).NotTo(Equal(int64(-2)))

		mGet := client.MGet(ctx, "key2")
		Expect(mGet.Err()).NotTo(HaveOccurred())
		Expect(mGet.Val()).To(Equal([]interface{}{"b"}))

		mGet2 := client.MGet(ctx, "key1", "key2", "key3", "key4")
		Expect(mGet2.Err()).NotTo(HaveOccurred())
		Expect(mGet2.Val()).To(Equal([]interface{}{"a", "b", "c", "d"}))
	})

	It("should mget with ttl", func() {
		set := client.Set(ctx, "key1", "a", 3000*time.Millisecond)
		Expect(set.Err()).NotTo(HaveOccurred())
		Expect(set.Val()).To(Equal("OK"))

		set1 := client.Set(ctx, "key2", "b", 3000*time.Millisecond)
		Expect(set1.Err()).NotTo(HaveOccurred())
		Expect(set1.Val()).To(Equal("OK"))

		set2 := client.Set(ctx, "key3", "c", 3000*time.Millisecond)
		Expect(set2.Err()).NotTo(HaveOccurred())
		Expect(set2.Val()).To(Equal("OK"))

		set3 := client.Set(ctx, "key4", "d", 3000*time.Millisecond)
		Expect(set3.Err()).NotTo(HaveOccurred())
		Expect(set3.Val()).To(Equal("OK"))

		mget := client.MGet(ctx, "key1")
		Expect(mget.Err()).NotTo(HaveOccurred())
		Expect(mget.Val()).To(Equal([]interface{}{"a"}))

		mGet := client.MGet(ctx, "key2")
		Expect(mGet.Err()).NotTo(HaveOccurred())
		Expect(mGet.Val()).To(Equal([]interface{}{"b"}))

		mGet1 := client.MGet(ctx, "key3")
		Expect(mGet1.Err()).NotTo(HaveOccurred())
		Expect(mGet1.Val()).To(Equal([]interface{}{"c"}))

		mGet2 := client.MGet(ctx, "key4")
		Expect(mGet2.Err()).NotTo(HaveOccurred())
		Expect(mGet2.Val()).To(Equal([]interface{}{"d"}))

		mGet3 := client.MGet(ctx, "key1", "key2", "key3", "key4")
		Expect(mGet3.Err()).NotTo(HaveOccurred())
		Expect(mGet3.Val()).To(Equal([]interface{}{"a", "b", "c", "d"}))

		Expect(client.TTL(ctx, "key1").Val()).NotTo(Equal(time.Duration(-2)))
		Expect(client.TTL(ctx, "key2").Val()).NotTo(Equal(time.Duration(-2)))
		Expect(client.TTL(ctx, "key3").Val()).NotTo(Equal(time.Duration(-2)))
		Expect(client.TTL(ctx, "key4").Val()).NotTo(Equal(time.Duration(-2)))

		time.Sleep(4 * time.Second)

		Expect(client.TTL(ctx, "key1").Val()).To(Equal(time.Duration(-2)))
		Expect(client.TTL(ctx, "key2").Val()).To(Equal(time.Duration(-2)))
		Expect(client.TTL(ctx, "key3").Val()).To(Equal(time.Duration(-2)))
		Expect(client.TTL(ctx, "key4").Val()).To(Equal(time.Duration(-2)))

		mGet4 := client.MGet(ctx, "key1", "key2", "key3", "key4")
		Expect(mGet4.Err()).NotTo(HaveOccurred())
		Expect(mGet4.Val()).To(Equal([]interface{}{nil, nil, nil, nil}))
	})
	It("should mget for multi key in cache and db", func() {
		multiset1 := client.Set(ctx, "key1", "a", 3000*time.Millisecond)
		Expect(multiset1.Err()).NotTo(HaveOccurred())
		Expect(multiset1.Val()).To(Equal("OK"))

		multiset2 := client.Set(ctx, "key2", "b", 3000*time.Millisecond)
		Expect(multiset2.Err()).NotTo(HaveOccurred())
		Expect(multiset2.Val()).To(Equal("OK"))

		multiset3 := client.Set(ctx, "key3", "c", 3000*time.Millisecond)
		Expect(multiset3.Err()).NotTo(HaveOccurred())
		Expect(multiset3.Val()).To(Equal("OK"))

		multiset4 := client.Set(ctx, "key4", "d", 3000*time.Millisecond)
		Expect(multiset4.Err()).NotTo(HaveOccurred())
		Expect(multiset4.Val()).To(Equal("OK"))

		multikey1 := client.MGet(ctx, "key1")
		Expect(multikey1.Err()).NotTo(HaveOccurred())
		Expect(multikey1.Val()).To(Equal([]interface{}{"a"}))

		MultiKey2 := client.Get(ctx, "key1")
		Expect(MultiKey2.Err()).NotTo(HaveOccurred())
		Expect(MultiKey2.Val()).To(Equal("a"))

		MultiMget := client.MGet(ctx, "key1", "key2", "key3", "key4")
		Expect(MultiMget.Err()).NotTo(HaveOccurred())
		Expect(MultiMget.Val()).To(Equal([]interface{}{"a", "b", "c", "d"}))
	})

	It("should mget for multi key in cache", func() {
		multiset1 := client.Set(ctx, "key1", "a", 3000*time.Millisecond)
		Expect(multiset1.Err()).NotTo(HaveOccurred())
		Expect(multiset1.Val()).To(Equal("OK"))

		multiset2 := client.Set(ctx, "key2", "b", 3000*time.Millisecond)
		Expect(multiset2.Err()).NotTo(HaveOccurred())
		Expect(multiset2.Val()).To(Equal("OK"))

		multiset3 := client.Set(ctx, "key3", "c", 3000*time.Millisecond)
		Expect(multiset3.Err()).NotTo(HaveOccurred())
		Expect(multiset3.Val()).To(Equal("OK"))

		multiset4 := client.Set(ctx, "key4", "d", 3000*time.Millisecond)
		Expect(multiset4.Err()).NotTo(HaveOccurred())
		Expect(multiset4.Val()).To(Equal("OK"))

		multikey1 := client.MGet(ctx, "key1")
		Expect(multikey1.Err()).NotTo(HaveOccurred())
		Expect(multikey1.Val()).To(Equal([]interface{}{"a"}))

		MultiKey2 := client.Get(ctx, "key1")
		Expect(MultiKey2.Err()).NotTo(HaveOccurred())
		Expect(MultiKey2.Val()).To(Equal("a"))

		MultiMget := client.MGet(ctx, "key1", "key2", "key3", "key4")
		Expect(MultiMget.Err()).NotTo(HaveOccurred())
		Expect(MultiMget.Val()).To(Equal([]interface{}{"a", "b", "c", "d"}))
	})

	It("should mget for multi key in db", func() {
		multiset1 := client.Set(ctx, "key1", "a", 3000*time.Millisecond)
		Expect(multiset1.Err()).NotTo(HaveOccurred())
		Expect(multiset1.Val()).To(Equal("OK"))

		multiset2 := client.Set(ctx, "key2", "b", 3000*time.Millisecond)
		Expect(multiset2.Err()).NotTo(HaveOccurred())
		Expect(multiset2.Val()).To(Equal("OK"))

		multiset3 := client.Set(ctx, "key3", "c", 3000*time.Millisecond)
		Expect(multiset3.Err()).NotTo(HaveOccurred())
		Expect(multiset3.Val()).To(Equal("OK"))

		multiset4 := client.Set(ctx, "key4", "d", 3000*time.Millisecond)
		Expect(multiset4.Err()).NotTo(HaveOccurred())
		Expect(multiset4.Val()).To(Equal("OK"))

		multikey1 := client.MGet(ctx, "key1")
		Expect(multikey1.Err()).NotTo(HaveOccurred())
		Expect(multikey1.Val()).To(Equal([]interface{}{"a"}))

		MultiKey2 := client.Get(ctx, "key1")
		Expect(MultiKey2.Err()).NotTo(HaveOccurred())
		Expect(MultiKey2.Val()).To(Equal("a"))

		multikey3 := client.MGet(ctx, "key2")
		Expect(multikey3.Err()).NotTo(HaveOccurred())
		Expect(multikey3.Val()).To(Equal([]interface{}{"b"}))

		multikey4 := client.MGet(ctx, "key3")
		Expect(multikey4.Err()).NotTo(HaveOccurred())
		Expect(multikey4.Val()).To(Equal([]interface{}{"c"}))

		multikey5 := client.MGet(ctx, "key4")
		Expect(multikey5.Err()).NotTo(HaveOccurred())
		Expect(multikey5.Val()).To(Equal([]interface{}{"d"}))

		MultiMget := client.MGet(ctx, "key1", "key2", "key3", "key4")
		Expect(MultiMget.Err()).NotTo(HaveOccurred())
		Expect(MultiMget.Val()).To(Equal([]interface{}{"a", "b", "c", "d"}))
	})

	It("should mget for multi key in db", func() {
		multiset1 := client.Set(ctx, "key1", "a", 3000*time.Millisecond)
		Expect(multiset1.Err()).NotTo(HaveOccurred())
		Expect(multiset1.Val()).To(Equal("OK"))

		multiset2 := client.Set(ctx, "key2", "b", 3000*time.Millisecond)
		Expect(multiset2.Err()).NotTo(HaveOccurred())
		Expect(multiset2.Val()).To(Equal("OK"))

		multiset3 := client.Set(ctx, "key3", "c", 3000*time.Millisecond)
		Expect(multiset3.Err()).NotTo(HaveOccurred())
		Expect(multiset3.Val()).To(Equal("OK"))

		multiset4 := client.Set(ctx, "key4", "d", 3000*time.Millisecond)
		Expect(multiset4.Err()).NotTo(HaveOccurred())
		Expect(multiset4.Val()).To(Equal("OK"))

		MultiMget := client.MGet(ctx, "key1", "key2", "key3", "key4")
		Expect(MultiMget.Err()).NotTo(HaveOccurred())
		Expect(MultiMget.Val()).To(Equal([]interface{}{"a", "b", "c", "d"}))
	})

	It("MGET against non existing key", func() {
		multiset1 := client.Set(ctx, "key1", "a", 3000*time.Millisecond)
		Expect(multiset1.Err()).NotTo(HaveOccurred())
		Expect(multiset1.Val()).To(Equal("OK"))

		multiset3 := client.Set(ctx, "key3", "c", 3000*time.Millisecond)
		Expect(multiset3.Err()).NotTo(HaveOccurred())
		Expect(multiset3.Val()).To(Equal("OK"))

		multiset4 := client.Set(ctx, "key4", "d", 3000*time.Millisecond)
		Expect(multiset4.Err()).NotTo(HaveOccurred())
		Expect(multiset4.Val()).To(Equal("OK"))

		MultiMget := client.MGet(ctx, "key1", "key2", "key3", "key4")
		Expect(MultiMget.Err()).NotTo(HaveOccurred())
		Expect(MultiMget.Val()).To(Equal([]interface{}{"a", nil, "c", "d"}))
	})
	It("MGET against non-string key", func() {
		SetMultiKey := client.Set(ctx, "foo{t}", "BAR", 3000*time.Millisecond)
		Expect(SetMultiKey.Err()).NotTo(HaveOccurred())
		Expect(SetMultiKey.Val()).To(Equal("OK"))

		SetMultiKey1 := client.Set(ctx, "bar{t}", "FOO", 3000*time.Millisecond)
		Expect(SetMultiKey1.Err()).NotTo(HaveOccurred())
		Expect(SetMultiKey1.Val()).To(Equal("OK"))

		SaddMultiKey := client.SAdd(ctx, "myset{t}", "ciao")
		Expect(SaddMultiKey.Err()).NotTo(HaveOccurred())
		Expect(SaddMultiKey.Val()).To(Equal(int64(1)))

		SaddMultiKey1 := client.SAdd(ctx, "myset{t}", "bau")
		Expect(SaddMultiKey1.Err()).NotTo(HaveOccurred())
		Expect(SaddMultiKey1.Val()).To(Equal(int64(1)))

		MultiMget := client.MGet(ctx, "foo{t}", "baazz{t}", "bar{t}", "myset{t}")
		Expect(MultiMget.Err()).NotTo(HaveOccurred())
		Expect(MultiMget.Val()).To(Equal([]interface{}{"BAR", nil, "FOO", nil}))
	})

	// The following cases guard the zset range-by-score / range-by-lex cache
	// read paths. They assert that the cache-hit result (second query, after the
	// async cache warm-up) matches the db-sourced result, especially with a
	// LIMIT offset/count applied. See ZRangebyscore/ZRevrangebyscore cache flags
	// and ZRevrangebylex::ReadCache offset handling.
	It("should ZRangeByScore hit cache consistently with LIMIT", func() {
		zsetKey := "zbyscore{t}"
		members := []redis.Z{
			{Score: 1, Member: "a"},
			{Score: 2, Member: "b"},
			{Score: 3, Member: "c"},
			{Score: 4, Member: "d"},
			{Score: 5, Member: "e"},
		}
		Expect(client.ZAdd(ctx, zsetKey, members...).Err()).NotTo(HaveOccurred())

		fullOpt := &redis.ZRangeBy{Min: "1", Max: "5"}
		limitOpt := &redis.ZRangeBy{Min: "1", Max: "5", Offset: 1, Count: 2}

		// First query goes through the DB and triggers async cache load.
		dbFull, err := client.ZRangeByScore(ctx, zsetKey, fullOpt).Result()
		Expect(err).NotTo(HaveOccurred())
		Expect(dbFull).To(Equal([]string{"a", "b", "c", "d", "e"}))

		dbLimit, err := client.ZRangeByScore(ctx, zsetKey, limitOpt).Result()
		Expect(err).NotTo(HaveOccurred())
		Expect(dbLimit).To(Equal([]string{"b", "c"}))

		// Wait for the key to be loaded into the cache, then read again so the
		// ReadCache path is exercised.
		time.Sleep(2 * time.Second)

		cacheFull, err := client.ZRangeByScore(ctx, zsetKey, fullOpt).Result()
		Expect(err).NotTo(HaveOccurred())
		Expect(cacheFull).To(Equal(dbFull))

		cacheLimit, err := client.ZRangeByScore(ctx, zsetKey, limitOpt).Result()
		Expect(err).NotTo(HaveOccurred())
		Expect(cacheLimit).To(Equal(dbLimit))
	})

	It("should ZRevRangeByScore hit cache consistently with LIMIT", func() {
		zsetKey := "zrevbyscore{t}"
		members := []redis.Z{
			{Score: 1, Member: "a"},
			{Score: 2, Member: "b"},
			{Score: 3, Member: "c"},
			{Score: 4, Member: "d"},
			{Score: 5, Member: "e"},
		}
		Expect(client.ZAdd(ctx, zsetKey, members...).Err()).NotTo(HaveOccurred())

		fullOpt := &redis.ZRangeBy{Min: "1", Max: "5"}
		limitOpt := &redis.ZRangeBy{Min: "1", Max: "5", Offset: 1, Count: 2}

		dbFull, err := client.ZRevRangeByScore(ctx, zsetKey, fullOpt).Result()
		Expect(err).NotTo(HaveOccurred())
		Expect(dbFull).To(Equal([]string{"e", "d", "c", "b", "a"}))

		dbLimit, err := client.ZRevRangeByScore(ctx, zsetKey, limitOpt).Result()
		Expect(err).NotTo(HaveOccurred())
		Expect(dbLimit).To(Equal([]string{"d", "c"}))

		time.Sleep(2 * time.Second)

		cacheFull, err := client.ZRevRangeByScore(ctx, zsetKey, fullOpt).Result()
		Expect(err).NotTo(HaveOccurred())
		Expect(cacheFull).To(Equal(dbFull))

		cacheLimit, err := client.ZRevRangeByScore(ctx, zsetKey, limitOpt).Result()
		Expect(err).NotTo(HaveOccurred())
		Expect(cacheLimit).To(Equal(dbLimit))
	})

	It("should ZRevRangeByLex hit cache consistently with LIMIT", func() {
		zsetKey := "zrevbylex{t}"
		// Members share the same score so ordering is purely lexicographical.
		members := []redis.Z{
			{Score: 0, Member: "a"},
			{Score: 0, Member: "b"},
			{Score: 0, Member: "c"},
			{Score: 0, Member: "d"},
			{Score: 0, Member: "e"},
		}
		Expect(client.ZAdd(ctx, zsetKey, members...).Err()).NotTo(HaveOccurred())

		fullOpt := &redis.ZRangeBy{Min: "-", Max: "+"}
		limitOpt := &redis.ZRangeBy{Min: "-", Max: "+", Offset: 1, Count: 2}

		dbFull, err := client.ZRevRangeByLex(ctx, zsetKey, fullOpt).Result()
		Expect(err).NotTo(HaveOccurred())
		Expect(dbFull).To(Equal([]string{"e", "d", "c", "b", "a"}))

		// offset 1, count 2 in reverse order -> skip "e", take "d", "c".
		dbLimit, err := client.ZRevRangeByLex(ctx, zsetKey, limitOpt).Result()
		Expect(err).NotTo(HaveOccurred())
		Expect(dbLimit).To(Equal([]string{"d", "c"}))

		time.Sleep(2 * time.Second)

		cacheFull, err := client.ZRevRangeByLex(ctx, zsetKey, fullOpt).Result()
		Expect(err).NotTo(HaveOccurred())
		Expect(cacheFull).To(Equal(dbFull))

		cacheLimit, err := client.ZRevRangeByLex(ctx, zsetKey, limitOpt).Result()
		Expect(err).NotTo(HaveOccurred())
		Expect(cacheLimit).To(Equal(dbLimit))
	})
})
