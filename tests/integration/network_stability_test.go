package pika_integration

import (
	"net"
	. "github.com/bsm/ginkgo/v2"
	. "github.com/bsm/gomega"
)

var _ = Describe("Telnet", func() {
	Describe("core dump fix", func() {
		It("should handle empty commands without crashing (telnet core dump fix)", func() {
			conn, err := net.Dial("tcp", SINGLEADDR)
			Expect(err).NotTo(HaveOccurred())
			defer conn.Close()

			_, err = conn.Write([]byte("\n"))
			Expect(err).NotTo(HaveOccurred())

			_, err = conn.Write([]byte("*1\r\n$4\r\nPING\r\n"))
			Expect(err).NotTo(HaveOccurred())

			buf := make([]byte, 1024)
			n, err := conn.Read(buf)
			Expect(err).NotTo(HaveOccurred())
			response := string(buf[:n])
			Expect(response).To(ContainSubstring("+PONG"))

			_, err = conn.Write([]byte("*2\r\n$4\r\nECHO\r\n$4\r\nTEST\r\n"))
			Expect(err).NotTo(HaveOccurred())

			n, err = conn.Read(buf)
			Expect(err).NotTo(HaveOccurred())
			response = string(buf[:n])
			Expect(response).To(ContainSubstring("$4\r\nTEST"))
		})

		It("should handle multiple empty commands without crashing", func() {
			conn, err := net.Dial("tcp", SINGLEADDR)
			Expect(err).NotTo(HaveOccurred())
			defer conn.Close()

			for i := 0; i < 5; i++ {
				_, err = conn.Write([]byte("\r\n"))
				Expect(err).NotTo(HaveOccurred())
			}

			_, err = conn.Write([]byte("*1\r\n$4\r\nPING\r\n"))
			Expect(err).NotTo(HaveOccurred())

			buf := make([]byte, 1024)
			n, err := conn.Read(buf)
			Expect(err).NotTo(HaveOccurred())
			response := string(buf[:n])
			Expect(response).To(ContainSubstring("+PONG"))
		})
	})
})