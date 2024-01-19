/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

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

func GetConfPath(copy bool, t int64) string {
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

func checkCondition(c *redis.Client) bool {
	ctx := context.TODO()
	//TODO(dingxiaoshuai) use Cmd PING
	r, e := c.Set(ctx, "key", "value", 0).Result()
	return r == "OK" && e == nil
}

type Server struct {
	cmd     *exec.Cmd
	addr    *net.TCPAddr
	delete  bool
	dbDir   string
	config  string
	outfile *os.File
}

func (s *Server) getAddr() string {
	return s.addr.String()
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

func (s *Server) Close() error {
	defer s.outfile.Close()
	err := s.cmd.Process.Signal(syscall.SIGINT)

	done := make(chan error, 1)
	go func() {
		done <- s.cmd.Wait()
	}()

	timeout := time.After(30 * time.Second)

	select {
	case <-timeout:
		log.Println("exec.cmd.Wait() timeout. Kill it.")
		if err = s.cmd.Process.Kill(); err != nil {
			log.Println("exec.cmd.Process.kill() fail.", err.Error())
			return err
		}
	case err = <-done:
		break
	}

	if s.delete {
		log.Println("Clean env....")
		err := os.RemoveAll(s.dbDir)
		if err != nil {
			log.Println("Remove dbDir fail.", err.Error())
			return err
		}
		log.Println("Remove dbDir success. ", s.dbDir)
		err = os.Remove(s.config)
		if err != nil {
			log.Println("Remove config file fail.", err.Error())
			return err
		}
		log.Println("Remove config file success.", s.config)
	}

	log.Println("Close Server Success.")

	return nil
}

func StartServer(config string, options map[string]string, delete bool) *Server {
	var (
		p     = 9221
		d     = ""
		n     = ""
		count = 0
	)

	b := getBinPath()
	c := exec.Command(b)

	outfile, err := os.Create("test.log")
	if err != nil {
		panic(err)
	}
	defer outfile.Close()

	c.Stdout = outfile
	c.Stderr = outfile
	log.SetOutput(outfile)

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

	err = c.Start()
	if err != nil {
		log.Println("pikiwidb startup failed.", err.Error())
		return nil
	}

	addr := &net.TCPAddr{
		IP:   net.ParseIP("127.0.0.1"),
		Port: p,
	}
	rdb := redis.NewClient(&redis.Options{
		Addr: addr.String(),
	})

	ticker := time.NewTicker(10 * time.Second)
	defer ticker.Stop()
	for {
		<-ticker.C
		count++

		if checkCondition(rdb) {
			log.Println("Successfully ping the service " + addr.String())
			break
		} else if count == 12 {
			log.Println("Failed to start the service " + addr.String())
			return nil
		} else {
			log.Println("Failed to ping the service "+addr.String()+" Retry Count =", count)
		}
	}

	log.Println("Start server success.")

	return &Server{
		cmd:     c,
		addr:    addr,
		delete:  delete,
		dbDir:   d,
		config:  n,
		outfile: outfile,
	}
}
