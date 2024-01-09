package util

import (
	"context"
	"fmt"
	"log"
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
		Addr:         s.addr.String(),
		DB:           0,
		DialTimeout:  10 * time.Second,
		ReadTimeout:  30 * time.Second,
		WriteTimeout: 30 * time.Second,
		MaxRetries:   -1,
		PoolSize:     30,
		PoolTimeout:  60 * time.Second,
	})
}

func (s *Server) Close() error {
	err := s.cmd.Process.Signal(syscall.SIGINT)
	done := make(chan error, 1)
	go func() {
		done <- s.cmd.Wait()
	}()

	timeout := time.After(30 * time.Second)

	select {
	case <-timeout:
		log.Println("Wait timeout. Kill it.")
		if err = s.cmd.Process.Kill(); err != nil {
			log.Println("kill fail.", err.Error())
			return err
		}
	case err = <-done:
		break
	}
	if s.delete {
		err := os.RemoveAll(s.dbDir)
		if err != nil {
			log.Println("Remove dbDir fail.", err.Error())
			return err
		}
		err = os.Remove(s.config)
		if err != nil {
			log.Println("Remove config file fail.", err.Error())
			return err
		}
	}
	return nil

}

func StartServer(config string, options map[string]string, delete bool) *Server {
	b := getBinPath()
	c := exec.Command(b)
	var (
		p     = 9221
		d     = ""
		n     = ""
		count = 0
	)

	if len(config) != 0 {
		t := time.Now().UnixMilli()
		d = path.Join(getRootPathByCaller(), fmt.Sprintf("db_%d", t))
		n = GetConfPath(true, t)

		cmd := exec.Command("cp", config, n)
		err := cmd.Run()
		if err != nil {
			log.Println("Cmd cp error.", err.Error())
			return nil
		}
		if runtime.GOOS == "darwin" {
			cmd = exec.Command("sed", "-i", "", "s|db-path ./db|db-path "+d+"/db"+"|", n)
		} else {
			cmd = exec.Command("sed", "-i", "s|db-path ./db|db-path "+d+"/db"+"|", n)
		}
		err = cmd.Run()
		if err != nil {
			log.Println("The configuration file cannot be used.", err.Error())
			return nil
		}

		c.Args = append(c.Args, n)
	}
	for k, v := range options {
		c.Args = append(c.Args, fmt.Sprintf("--%s", k), v)
	}

	if options["port"] != "" {
		p, _ = strconv.Atoi(options["port"])
	}

	err := c.Start()
	if err != nil {
		log.Println("Pikiwidb startup failed.", err.Error())
		return nil
	}

	ticker := time.NewTicker(6 * time.Second)
	defer ticker.Stop()
	for {
		count++
		if checkCondition("127.0.0.1:" + strconv.Itoa(p)) {
			break
		}
		if count == 10 {
			log.Println("Unable to establish a valid connection.", err.Error())
			return nil
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
