package pikiwidb_test

import (
	"context"
	"strconv"

	. "github.com/onsi/ginkgo/v2"
	. "github.com/onsi/gomega"
	"github.com/redis/go-redis/v9"

	"github.com/OpenAtomFoundation/pikiwidb/tests/util"
)

var _ = Describe("Set", func() {
	var (
		ctx = context.TODO()
		s   *util.Server
		rdb *redis.Client
	)

	BeforeEach(func() {
		config := util.GetConfPath(false, 0)
		s = util.StartServer(config, map[string]string{"port": strconv.Itoa(7777)}, true)
		if s == nil {
			return
		}
		rdb = s.NewClient()
	})

	AfterEach(func() {
		err := s.Close()
		if err != nil {
			return
		}
	})

	//TODO(dingxiaoshuai) Add more test cases.
	It("CmdSAdd", func() {
		Expect(rdb.SAdd(ctx, "myset", "one", "two").Val()).NotTo(Equal("FooBar"))
	})
})
