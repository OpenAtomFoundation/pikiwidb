package pikiwidb_test

import (
	"context"
	"strconv"

	. "github.com/onsi/ginkgo/v2"
	. "github.com/onsi/gomega"
	"github.com/redis/go-redis/v9"

	"github.com/OpenAtomFoundation/pikiwidb/tests/util"
)

var _ = Describe("Hash", func() {
	var (
		ctx = context.TODO()
		s   *util.Server
		rdb *redis.Client
	)
	BeforeEach(func() {
		config := util.GetConfPath(false, 0)
		s = util.StartServer(config, map[string]string{"port": strconv.Itoa(7777)}, true)
		rdb = s.NewClient()
	})

	AfterEach(func() {
		s.Close()
	})

	//TODO(dingxiaoshuai) Add more test cases.
	It("CmdHset", func() {
		Expect(rdb.HSet(ctx, "myhash", "one").Val()).NotTo(Equal("FooBar"))
	})
})
