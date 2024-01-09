package util

import (
	"context"
	"fmt"
	"io"
	"net"
	"os"
	"os/exec"
	"path"
	"runtime"
	"strconv"
	"syscall"
	"time"

	"github.com/redis/go-redis/v9"
)

func getRootPathByCaller() string {
	var rPath string
	_, filename, _, ok := runtime.Caller(1)
	if ok {
		rPath = path.Dir(filename)
		rPath = path.Dir(rPath)
		rPath = path.Dir(rPath)
	}
	return rPath
}

func getBinPath() string {
	rPath := getRootPathByCaller()
	var bPath string
	if len(rPath) != 0 {
		bPath = path.Join(rPath, "bin", "pikiwidb")
	}
	return bPath
}

func getConfPath(copy bool, t int64) string {
	rPath := getRootPathByCaller()
	var (
		cPath string
		nPath string
	)
	if len(rPath) != 0 && copy {
		nPath = path.Join(rPath, fmt.Sprintf("pikiwidb_%d.conf", t))
		return nPath
	}
	if len(rPath) != 0 {
		cPath = path.Join(rPath, "pikiwidb.conf")
		return cPath
	}
	return rPath
}

func GetConfPath(copy bool, t int64) string {
	return getConfPath(copy, t)
}

func checkCondition(addr string) bool {
	ctx := context.TODO()
	rdb := redis.NewClient(&redis.Options{
		Addr:         addr,
		DB:           0,
		DialTimeout:  10 * time.Second,
		ReadTimeout:  30 * time.Second,
		WriteTimeout: 30 * time.Second,
		MaxRetries:   -1,
		PoolSize:     30,
		PoolTimeout:  60 * time.Second,
	})
	return (rdb.Set(ctx, "key", "value", 0).Val()) == "OK"
}

type Server struct {
	cmd    *exec.Cmd
	addr   *net.TCPAddr
	delete bool
	dbDir  string
	config string
}

func (s *Server) getAddr() string {
	return s.addr.String()
}

func (s *Server) GetAddr() string {
	return s.getAddr()
}

func (s *Server) NewClient() *redis.Client {
	return redis.NewClient(&redis.Options{
		Addr:         s.getAddr(),
		DB:           0,
		DialTimeout:  10 * time.Second,
		ReadTimeout:  30 * time.Second,
		WriteTimeout: 30 * time.Second,
		MaxRetries:   -1,
		PoolSize:     30,
		PoolTimeout:  60 * time.Second,
	})
}

func (s *Server) Close() {
	err := s.cmd.Process.Signal(syscall.SIGTERM)
	if err != nil || s.delete {
		if err != nil {
			s.cmd.Process.Kill()
		}
		s.cmd.Wait()
		os.RemoveAll(s.dbDir)
		os.Remove(s.config)
		return
	}
	s.cmd.Wait()
}

func StartServer(config string, options map[string]string, delete bool) *Server {
	b := getBinPath()
	c := exec.Command(b)
	var (
		p = 9221
		d = ""
		n = ""
	)

	if len(config) != 0 {
		t := time.Now().UnixMilli()
		d = path.Join(getRootPathByCaller(), fmt.Sprintf("db_%d", t))
		n = GetConfPath(true, t)

		src, _ := os.Open(config)
		defer src.Close()
		dest, _ := os.Create(n)
		defer dest.Close()
		_, _ = io.Copy(dest, src)

		cmd := exec.Command("sed", "-i", "", "-e", "s|db-path ./db|db-path "+d+"/db"+"|g", n)
		_ = cmd.Run()

		c.Args = append(c.Args, n)
	}
	for k, v := range options {
		c.Args = append(c.Args, fmt.Sprintf("--%s ", k), v)
	}

	if options["port"] != "" {
		p, _ = strconv.Atoi(options["port"])
	}

	err := c.Start()
	if err != nil {
		return nil
	}

	ticker := time.NewTicker(5 * time.Second)
	defer ticker.Stop()
	for {
		if checkCondition("127.0.0.1:" + strconv.Itoa(p)) {
			break
		}
		<-ticker.C
	}
	return &Server{
		cmd: c,
		addr: &net.TCPAddr{
			IP:   net.ParseIP("127.0.0.1"),
			Port: p,
		},
		delete: delete,
		dbDir:  d,
		config: n,
	}
}
