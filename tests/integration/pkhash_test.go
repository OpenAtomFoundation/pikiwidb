package pika_integration

import (
	"context"
	"time"

	. "github.com/bsm/ginkgo/v2"
	. "github.com/bsm/gomega"

	"github.com/redis/go-redis/v9"
)

var _ = Describe("PKHASH Commands", func() {
	ctx := context.TODO()
	var client *redis.Client

	BeforeEach(func() {
		client = redis.NewClient(PikaOption(SINGLEADDR))
		Expect(client.FlushDB(ctx).Err()).NotTo(HaveOccurred())
		if GlobalBefore != nil {
			GlobalBefore(ctx, client)
		}
		time.Sleep(1 * time.Second)
	})

	AfterEach(func() {
		Expect(client.Close()).NotTo(HaveOccurred())
	})

	Describe("pkhash", func() {

		It("should PKHExpire", func() {
			pkhsetCmd := client.Do(ctx, "pkhset", "pkhash", "key", "hello")
			Expect(pkhsetCmd.Err()).NotTo(HaveOccurred())

			// 2000 ms
			ttl := 2 * 1000
			pkHExpireCmd := client.Do(ctx, "pkhexpire", "pkhash", ttl, "FIELDS", 1, "key")
			Expect(pkHExpireCmd.Err()).NotTo(HaveOccurred())
			Expect(pkHExpireCmd.Val()).To(Equal([]interface{}{int64(1)}))

			// sleep for ttl + 100 ms
			time.Sleep(time.Duration(ttl+100) * time.Millisecond)

			// pkhget
			pkHGetCmd1 := client.Do(ctx, "pkhget", "pkhash", "key")
			s2, err := pkHGetCmd1.Text()

			Expect(err).To(Equal(redis.Nil))
			Expect(s2).To(Equal(""))
		})

		It("should PKHExpireat", func() {
			pkhsetCmd := client.Do(ctx, "pkhset", "pkhash", "pkhash_key", "hello")
			Expect(pkhsetCmd.Err()).NotTo(HaveOccurred())

			pkHGetCmd3 := client.Do(ctx, "pkhget", "pkhash", "pkhash_key")

			Expect(pkHGetCmd3.Err()).NotTo(HaveOccurred())
			Expect(pkHGetCmd3.Val()).To(Equal("hello"))

			// 2000 ms
			expireTime := time.Now().Add(time.Millisecond * 2000).UnixMilli()
			ttl := 2 * 1000

			pkHExpireatCmd := client.Do(ctx, "pkhexpireat", "pkhash", int64(expireTime), "FIELDS", 1, "pkhash_key")
			Expect(pkHExpireatCmd.Err()).NotTo(HaveOccurred())
			Expect(pkHExpireatCmd.Val()).To(Equal([]interface{}{int64(1)}))

			// sleep for ttl + 100 ms
			time.Sleep(time.Duration(ttl+100) * time.Millisecond)
			// pkhget
			pkHGetCmd1 := client.Do(ctx, "pkhget", "pkhash", "pkhash_key")
			s2, err := pkHGetCmd1.Text()

			Expect(err).To(Equal(redis.Nil))
			Expect(s2).To(Equal(""))
		})

		It("should PKHExpiretime", func() {
			pkhsetCmd := client.Do(ctx, "pkhset", "pkhash", "key", "hello")
			Expect(pkhsetCmd.Err()).NotTo(HaveOccurred())

			// 2000 ms
			expireTime := time.Now().Add(time.Millisecond * 2000).UnixMilli()

			ttl := 2 * 1000

			pkHExpireatCmd := client.Do(ctx, "pkhexpireat", "pkhash", expireTime, "FIELDS", 1, "key")
			Expect(pkHExpireatCmd.Err()).NotTo(HaveOccurred())
			Expect(pkHExpireatCmd.Val()).To(Equal([]interface{}{int64(1)}))

			pkHExpiretimeCmd := client.Do(ctx, "pkhexpiretime", "pkhash", "FIELDS", 1, "key")
			pkHExpiretime, pkHExpiretimeErr := pkHExpiretimeCmd.Int64Slice()

			Expect(pkHExpiretimeErr).NotTo(HaveOccurred())
			Expect(pkHExpiretime[0]).To(Equal(expireTime))

			// sleep for ttl + 100 ms
			time.Sleep(time.Duration(ttl+100) * time.Millisecond)

			// pkhget
			pkHGetCmd1 := client.Do(ctx, "pkhget", "pkhash", "key")
			s2, err := pkHGetCmd1.Text()

			Expect(err).To(Equal(redis.Nil))
			Expect(s2).To(Equal(""))
		})

		// 	It("should PKHTTL", func() {
		// 		pkhsetCmd := client.Do(ctx, "pkhset", "pkhash", "key", "hello")
		// 		Expect(pkhsetCmd.Err()).NotTo(HaveOccurred())

		// 		// 2000 ms
		// 		ttl := 2 * 1000

		// 		pkHExpireatCmd := client.Do(ctx, "pkhexpire", "pkhash", ttl, "FIELDS", 1, "key")
		// 		Expect(pkHExpireatCmd.Err()).NotTo(HaveOccurred())
		// 		Expect(pkHExpireatCmd.Val()).To(Equal([]interface{}{int64(1)}))

		// 		pkHTTLCmd := client.Do(ctx, "pkhttl", "pkhash", "FIELDS", 1, "key")
		// 		pkHTTL, pkHTTLErr := pkHTTLCmd.Int64Slice()

		// 		Expect(pkHTTLErr).NotTo(HaveOccurred())
		// 		Expect(pkHTTL[0]).To(BeNumerically("~", ttl, 2000))

		// 		// sleep for ttl + 100 ms
		// 		time.Sleep(time.Duration(ttl+100) * time.Millisecond)

		// 		// pkhget
		// 		pkHGetCmd1 := client.Do(ctx, "pkhget", "pkhash", "key")
		// 		s2, err := pkHGetCmd1.Text()

		// 		Expect(err).To(Equal(redis.Nil))
		// 		Expect(s2).To(Equal(""))
		// 	})

		// 	It("should PKHPersist", func() {
		// 		pkhsetCmd := client.Do(ctx, "pkhset", "pkhash", "key", "hello")
		// 		Expect(pkhsetCmd.Err()).NotTo(HaveOccurred())

		// 		// 2000 ms
		// 		ttl := 3 * 1000

		// 		pkHExpireCmd := client.Do(ctx, "pkhexpire", "pkhash", ttl, "FIELDS", 1, "key")
		// 		Expect(pkHExpireCmd.Err()).NotTo(HaveOccurred())
		// 		Expect(pkHExpireCmd.Val()).To(Equal([]interface{}{int64(1)}))

		// 		pkHTTLCmd := client.Do(ctx, "pkhttl", "pkhash", "FIELDS", 1, "key")
		// 		pkHTTL, pkHTTLErr := pkHTTLCmd.Int64Slice()

		// 		Expect(pkHTTLErr).NotTo(HaveOccurred())
		// 		Expect(pkHTTL[0]).To(BeNumerically("~", ttl, ttl))

		// 		// sleep for ttl + 100 ms
		// 		time.Sleep(time.Duration(ttl+100) * time.Millisecond)

		// 		// pkhget
		// 		pkHGetCmd1 := client.Do(ctx, "pkhget", "pkhash", "key")
		// 		s2, err := pkHGetCmd1.Text()

		// 		Expect(err).To(Equal(redis.Nil))
		// 		Expect(s2).To(Equal(""))

		// 		pkhsetCmd1 := client.Do(ctx, "pkhset", "pkhash", "key", "hello")
		// 		Expect(pkhsetCmd1.Err()).NotTo(HaveOccurred())
		// 		// set ttl
		// 		pkHExpireCmd = client.Do(ctx, "pkhexpire", "pkhash", ttl, "FIELDS", 1, "key")
		// 		Expect(pkHExpireCmd.Err()).NotTo(HaveOccurred())
		// 		Expect(pkHExpireCmd.Val()).To(Equal([]interface{}{int64(1)}))

		// 		// use pkhpersist to persist filed
		// 		pkHPersistCmd := client.Do(ctx, "pkhpersist", "pkhash", "FIELDS", 1, "key")
		// 		Expect(pkHPersistCmd.Err()).NotTo(HaveOccurred())
		// 		Expect(pkHPersistCmd.Val()).To(Equal([]interface{}{int64(1)}))

		// 		pkHGetCmd := client.Do(ctx, "pkhget", "pkhash", "key")
		// 		Expect(pkHGetCmd.Err()).NotTo(HaveOccurred())
		// 		Expect(pkHGetCmd.Val()).To(Equal("hello"))
		// 	})

		// 	It("should PKHDel", func() {
		// 		pkhsetCmd := client.Do(ctx, "pkhset", "pkhash", "key", "hello")
		// 		Expect(pkhsetCmd.Err()).NotTo(HaveOccurred())

		// 		hDel := client.Do(ctx, "pkhdel", "pkhash", "key")
		// 		Expect(hDel.Err()).NotTo(HaveOccurred())
		// 		Expect(hDel.Val()).To(Equal(int64(1)))

		// 		hDel = client.Do(ctx, "pkhdel", "pkhash", "key")
		// 		Expect(hDel.Err()).NotTo(HaveOccurred())
		// 		Expect(hDel.Val()).To(Equal(int64(0)))
		// 	})

		// 	It("should PKHExists", func() {
		// 		pkHSet := client.Do(ctx, "pkhset", "pkhash", "key", "hello")
		// 		Expect(pkHSet.Err()).NotTo(HaveOccurred())

		// 		pkHExists := client.Do(ctx, "pkhexists", "pkhash", "key")
		// 		Expect(pkHExists.Err()).NotTo(HaveOccurred())
		// 		Expect(pkHExists.Val()).To(Equal(int64(1)))

		// 		pkHExists = client.Do(ctx, "pkhexists", "pkhash", "key1")
		// 		Expect(pkHExists.Err()).NotTo(HaveOccurred())
		// 		Expect(pkHExists.Val()).To(Equal(int64(0)))
		// 	})

		// 	It("should PKHGet", func() {
		// 		pkHSet := client.Do(ctx, "pkhset", "pkhash", "key", "hello")
		// 		Expect(pkHSet.Err()).NotTo(HaveOccurred())

		// 		pkHGet := client.Do(ctx, "pkhget", "pkhash", "key")

		// 		Expect(pkHGet.Err()).NotTo(HaveOccurred())
		// 		Expect(pkHGet.Val()).To(Equal("hello"))

		// 		pkHGet1 := client.Do(ctx, "pkhget", "pkhash", "key1")
		// 		s2, err := pkHGet1.Text()

		// 		Expect(err).To(Equal(redis.Nil))
		// 		Expect(s2).To(Equal(""))
		// 	})

		// 	It("should PKHGetAll", func() {
		// 		err := client.Do(ctx, "pkhset", "pkhash", "key1", "hello1").Err()
		// 		Expect(err).NotTo(HaveOccurred())
		// 		err = client.Do(ctx, "pkhset", "pkhash", "key2", "hello2").Err()
		// 		Expect(err).NotTo(HaveOccurred())

		// 		m, err := client.Do(ctx, "pkhgetall", "pkhash").StringSlice()

		// 		Expect(err).NotTo(HaveOccurred())
		// 		Expect(m).To(Equal([]string{"key1", "hello1", "key2", "hello2"}))
		// 	})

		// 	It("should HKeys", func() {
		// 		pkHKeys := client.Do(ctx, "pkhkeys", "pkhash")
		// 		Expect(pkHKeys.Err()).NotTo(HaveOccurred())
		// 		Expect(pkHKeys.Val()).To(Equal([]interface{}{}))

		// 		hset := client.Do(ctx, "pkhset", "pkhash", "key1", "hello1")
		// 		Expect(hset.Err()).NotTo(HaveOccurred())
		// 		hset = client.Do(ctx, "pkhset", "pkhash", "key2", "hello2")
		// 		Expect(hset.Err()).NotTo(HaveOccurred())

		// 		pkHKeys = client.Do(ctx, "pkhkeys", "pkhash")
		// 		Expect(pkHKeys.Err()).NotTo(HaveOccurred())
		// 		Expect(pkHKeys.Val()).To(Equal([]interface{}{"key1", "key2"}))
		// 	})

		// 	It("should HLen", func() {
		// 		pkHSet := client.Do(ctx, "pkhset", "pkhash", "key1", "hello1")
		// 		Expect(pkHSet.Err()).NotTo(HaveOccurred())
		// 		pkHSet = client.Do(ctx, "pkhset", "pkhash", "key2", "hello2")
		// 		Expect(pkHSet.Err()).NotTo(HaveOccurred())

		// 		pkHLen := client.Do(ctx, "pkhlen", "pkhash")
		// 		Expect(pkHLen.Err()).NotTo(HaveOccurred())
		// 		Expect(pkHLen.Val()).To(Equal(int64(2)))
		// 	})

		// 	It("should HMGet", func() {
		// 		err := client.Do(ctx, "pkhset", "pkhash", "key1", "hello1").Err()
		// 		Expect(err).NotTo(HaveOccurred())

		// 		vals, err := client.Do(ctx, "pkhmget", "pkhash", "key1").Result()
		// 		Expect(err).NotTo(HaveOccurred())
		// 		Expect(vals).To(Equal([]interface{}{"hello1"}))
		// 	})

		// 	It("should HVals", func() {
		// 		err := client.Do(ctx, "pkhset", "hash121", "key1", "hello1").Err()
		// 		Expect(err).NotTo(HaveOccurred())
		// 		err = client.Do(ctx, "pkhset", "hash121", "key2", "hello2").Err()
		// 		Expect(err).NotTo(HaveOccurred())

		// 		v, err := client.Do(ctx, "pkhvals", "hash121").Result()
		// 		Expect(err).NotTo(HaveOccurred())
		// 		Expect(v).To(Equal([]interface{}{"hello1", "hello2"}))
		// 	})

		// 	It("should PKHSTRLEN", func() {
		// 		pkHSet := client.Do(ctx, "pkhset", "pkhash", "key1", "hello1")
		// 		Expect(pkHSet.Err()).NotTo(HaveOccurred())

		// 		pkHGet := client.Do(ctx, "pkhget", "pkhash", "key1")
		// 		Expect(pkHGet.Err()).NotTo(HaveOccurred())
		// 		length, err := client.Do(ctx, "pkhstrlen", "pkhash", "key1").Int64()
		// 		Expect(err).NotTo(HaveOccurred())

		// 		Expect(length).To(Equal(int64(len("hello1"))))
		// 	})
	})
})
