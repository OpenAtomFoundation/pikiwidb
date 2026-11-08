package pika_integration

import (
	"context"
	"fmt"
	"time"

	. "github.com/bsm/ginkgo/v2"
	. "github.com/bsm/gomega"
	"github.com/redis/go-redis/v9"
)

var _ = Describe("PacificA Consistency Tests", func() {
	ctx := context.TODO()
	var masterClient *redis.Client
	var slave1Client *redis.Client
	var slave2Client *redis.Client

	// Initialize client connections
	initClients := func() {
		masterClient = redis.NewClient(PikaOption(PACIFICA_MASTER_ADDR))
		slave1Client = redis.NewClient(PikaOption(PACIFICA_SLAVE1_ADDR))
		slave2Client = redis.NewClient(PikaOption(PACIFICA_SLAVE2_ADDR))

		Expect(masterClient.FlushDB(ctx).Err()).NotTo(HaveOccurred())
		time.Sleep(1 * time.Second)
	}

	// Configure slaves in strong consistency mode
	setupSlavesForConsistency := func() {
		// Configure slave1
		slaveof1 := slave1Client.Do(ctx, "slaveof", LOCALHOST, PACIFICA_MASTER_PORT, "strong")
		Expect(slaveof1.Err()).NotTo(HaveOccurred())

		// Configure slave2
		slaveof2 := slave2Client.Do(ctx, "slaveof", LOCALHOST, PACIFICA_MASTER_PORT, "strong")
		Expect(slaveof2.Err()).NotTo(HaveOccurred())

		time.Sleep(2 * time.Second)
	}

	BeforeEach(func() {
		initClients()
		setupSlavesForConsistency()
	})

	AfterEach(func() {
		if masterClient != nil {
			masterClient.Close()
		}
		if slave1Client != nil {
			slave1Client.Close()
		}
		if slave2Client != nil {
			slave2Client.Close()
		}
	})

	Context("Basic Consistency Tests", func() {
		It("should establish master-slave relationship correctly", func() {
			// Check slave1 replication status
			info1 := slave1Client.Info(ctx, "Replication")
			Expect(info1.Err()).NotTo(HaveOccurred())
			Expect(info1.Val()).To(ContainSubstring("role:slave"))
			Expect(info1.Val()).To(ContainSubstring(fmt.Sprintf("master_port:%s", PACIFICA_MASTER_PORT)))

			// Check slave2 replication status
			info2 := slave2Client.Info(ctx, "Replication")
			Expect(info2.Err()).NotTo(HaveOccurred())
			Expect(info2.Val()).To(ContainSubstring("role:slave"))
			Expect(info2.Val()).To(ContainSubstring(fmt.Sprintf("master_port:%s", PACIFICA_MASTER_PORT)))
		})

		It("should maintain data consistency across all nodes", func() {
			// Write data to master node
			Expect(masterClient.Set(ctx, "test_key", "test_value", 0).Err()).NotTo(HaveOccurred())
			time.Sleep(5 * time.Second)
			// Verify data consistency on both slave nodes
			get1 := slave1Client.Get(ctx, "test_key")
			Expect(get1.Err()).NotTo(HaveOccurred())
			Expect(get1.Val()).To(Equal("test_value"))

			get2 := slave2Client.Get(ctx, "test_key")
			Expect(get2.Err()).NotTo(HaveOccurred())
			Expect(get2.Val()).To(Equal("test_value"))
		})
	})

	Context("Advanced Consistency Tests", func() {
		It("should maintain consistency during concurrent writes", func() {
			// Concurrent writes of multiple key-value pairs
			for i := 0; i < 10; i++ {
				i := i
				go func() {
					key := fmt.Sprintf("concurrent_key_%d", i)
					value := fmt.Sprintf("value_%d", i)
					Expect(masterClient.Set(ctx, key, value, 0).Err()).NotTo(HaveOccurred())
				}()
			}

			// Wait for data synchronization
			time.Sleep(10 * time.Second)

			// Verify data consistency across all nodes
			for i := 0; i < 10; i++ {
				key := fmt.Sprintf("concurrent_key_%d", i)
				expectedValue := fmt.Sprintf("value_%d", i)

				get1 := slave1Client.Get(ctx, key)
				Expect(get1.Err()).NotTo(HaveOccurred())
				Expect(get1.Val()).To(Equal(expectedValue))

				get2 := slave2Client.Get(ctx, key)
				Expect(get2.Err()).NotTo(HaveOccurred())
				Expect(get2.Val()).To(Equal(expectedValue))
			}
		})

		It("should maintain consistency after network partition recovery", func() {
			// Write initial data
			Expect(masterClient.Set(ctx, "partition_test", "before_partition", 0).Err()).NotTo(HaveOccurred())

			// Simulate network partition (by temporarily disconnecting slave1)
			time.Sleep(3 * time.Second)
			slave1Client.Close()

			// Continue writing new data to master
			Expect(masterClient.Set(ctx, "partition_test", "after_partition", 0).Err()).NotTo(HaveOccurred())

			// Restore connection
			time.Sleep(5 * time.Second)
			slave1Client = redis.NewClient(PikaOption(PACIFICA_SLAVE1_ADDR))
			slave1Client.Do(ctx, "slaveof", LOCALHOST, PACIFICA_MASTER_PORT, "strong")

			// Wait for data synchronization
			time.Sleep(5 * time.Second)

			// Verify data consistency across all nodes
			get1 := slave1Client.Get(ctx, "partition_test")
			Expect(get1.Err()).NotTo(HaveOccurred())
			Expect(get1.Val()).To(Equal("after_partition"))

			get2 := slave2Client.Get(ctx, "partition_test")
			Expect(get2.Err()).NotTo(HaveOccurred())
			Expect(get2.Val()).To(Equal("after_partition"))
		})
	})

	Context("Dynamic Node Addition Tests", func() {
		It("should sync both historical and new data to newly added slave", func() {
			// Start with only one slave
			slave2Client.Close()
			slave2Client = nil

			// Configure slave1
			slaveof1 := slave1Client.Do(ctx, "slaveof", LOCALHOST, PACIFICA_MASTER_PORT, "strong")
			Expect(slaveof1.Err()).NotTo(HaveOccurred())
			time.Sleep(2 * time.Second)

			// Write historical data
			for i := 0; i < 10; i++ {
				key := fmt.Sprintf("historical_key_%d", i)
				value := fmt.Sprintf("historical_value_%d", i)
				Expect(masterClient.Set(ctx, key, value, 0).Err()).NotTo(HaveOccurred())
			}

			// Verify slave1 has historical data
			time.Sleep(8 * time.Second)
			for i := 0; i < 10; i++ {
				key := fmt.Sprintf("historical_key_%d", i)
				expectedValue := fmt.Sprintf("historical_value_%d", i)
				get := slave1Client.Get(ctx, key)
				Expect(get.Err()).NotTo(HaveOccurred())
				Expect(get.Val()).To(Equal(expectedValue))
			}

			// Add slave2 to the cluster
			slave2Client = redis.NewClient(PikaOption(PACIFICA_SLAVE2_ADDR))
			slaveof2 := slave2Client.Do(ctx, "slaveof", LOCALHOST, PACIFICA_MASTER_PORT, "strong")
			Expect(slaveof2.Err()).NotTo(HaveOccurred())

			// Wait for initial sync
			time.Sleep(10 * time.Second)

			// Write new data after slave2 joined
			for i := 0; i < 10; i++ {
				key := fmt.Sprintf("new_key_%d", i)
				value := fmt.Sprintf("new_value_%d", i)
				Expect(masterClient.Set(ctx, key, value, 0).Err()).NotTo(HaveOccurred())
			}

			// Wait for data synchronization
			time.Sleep(8 * time.Second)

			// Verify both historical and new data on slave2
			// Check historical data
			for i := 0; i < 10; i++ {
				key := fmt.Sprintf("historical_key_%d", i)
				expectedValue := fmt.Sprintf("historical_value_%d", i)
				get := slave2Client.Get(ctx, key)
				Expect(get.Err()).NotTo(HaveOccurred())
				Expect(get.Val()).To(Equal(expectedValue))
			}

			// Check new data
			for i := 0; i < 10; i++ {
				key := fmt.Sprintf("new_key_%d", i)
				expectedValue := fmt.Sprintf("new_value_%d", i)
				get := slave2Client.Get(ctx, key)
				Expect(get.Err()).NotTo(HaveOccurred())
				Expect(get.Val()).To(Equal(expectedValue))
			}

			// Verify data is consistent across all nodes
			for i := 0; i < 10; i++ {
				// Check historical data consistency
				historicalKey := fmt.Sprintf("historical_key_%d", i)
				historicalValue := fmt.Sprintf("historical_value_%d", i)

				get1 := slave1Client.Get(ctx, historicalKey)
				Expect(get1.Err()).NotTo(HaveOccurred())
				Expect(get1.Val()).To(Equal(historicalValue))

				get2 := slave2Client.Get(ctx, historicalKey)
				Expect(get2.Err()).NotTo(HaveOccurred())
				Expect(get2.Val()).To(Equal(historicalValue))

				// Check new data consistency
				newKey := fmt.Sprintf("new_key_%d", i)
				newValue := fmt.Sprintf("new_value_%d", i)

				get3 := slave1Client.Get(ctx, newKey)
				Expect(get3.Err()).NotTo(HaveOccurred())
				Expect(get3.Val()).To(Equal(newValue))

				get4 := slave2Client.Get(ctx, newKey)
				Expect(get4.Err()).NotTo(HaveOccurred())
				Expect(get4.Val()).To(Equal(newValue))
			}
		})
	})

	Context("Node Failure and Recovery Tests", func() {
		It("should maintain cluster operation and data consistency during node failure and recovery", func() {
			// Initial setup - all nodes running
			// Write initial data
			for i := 0; i < 10; i++ {
				key := fmt.Sprintf("initial_key_%d", i)
				value := fmt.Sprintf("initial_value_%d", i)
				Expect(masterClient.Set(ctx, key, value, 0).Err()).NotTo(HaveOccurred())
			}

			// Verify initial data synchronization
			time.Sleep(8 * time.Second)
			for i := 0; i < 10; i++ {
				key := fmt.Sprintf("initial_key_%d", i)
				expectedValue := fmt.Sprintf("initial_value_%d", i)

				// Check slave1
				get1 := slave1Client.Get(ctx, key)
				Expect(get1.Err()).NotTo(HaveOccurred())
				Expect(get1.Val()).To(Equal(expectedValue))

				// Check slave2
				get2 := slave2Client.Get(ctx, key)
				Expect(get2.Err()).NotTo(HaveOccurred())
				Expect(get2.Val()).To(Equal(expectedValue))
			}

			// Simulate slave1 failure
			slave1Client.Close()
			slave1Client = nil
			time.Sleep(3 * time.Second)

			// Verify cluster still operates with remaining nodes
			// Write new data during failure
			for i := 0; i < 10; i++ {
				key := fmt.Sprintf("during_failure_key_%d", i)
				value := fmt.Sprintf("during_failure_value_%d", i)
				Expect(masterClient.Set(ctx, key, value, 0).Err()).NotTo(HaveOccurred())
			}

			// Verify slave2 still receives updates
			time.Sleep(8 * time.Second)
			for i := 0; i < 10; i++ {
				key := fmt.Sprintf("during_failure_key_%d", i)
				expectedValue := fmt.Sprintf("during_failure_value_%d", i)

				get := slave2Client.Get(ctx, key)
				Expect(get.Err()).NotTo(HaveOccurred())
				Expect(get.Val()).To(Equal(expectedValue))
			}

			// Restore slave1
			slave1Client = redis.NewClient(PikaOption(PACIFICA_SLAVE1_ADDR))
			slaveof := slave1Client.Do(ctx, "slaveof", LOCALHOST, PACIFICA_MASTER_PORT, "strong")
			Expect(slaveof.Err()).NotTo(HaveOccurred())

			// Wait for recovery and synchronization
			time.Sleep(10 * time.Second)

			// Write new data after recovery
			for i := 0; i < 10; i++ {
				key := fmt.Sprintf("after_recovery_key_%d", i)
				value := fmt.Sprintf("after_recovery_value_%d", i)
				Expect(masterClient.Set(ctx, key, value, 0).Err()).NotTo(HaveOccurred())
			}

			time.Sleep(8 * time.Second)

			// Verify recovered node has all data:
			// 1. Initial data before failure
			for i := 0; i < 10; i++ {
				key := fmt.Sprintf("initial_key_%d", i)
				expectedValue := fmt.Sprintf("initial_value_%d", i)

				get := slave1Client.Get(ctx, key)
				Expect(get.Err()).NotTo(HaveOccurred())
				Expect(get.Val()).To(Equal(expectedValue))
			}

			// 2. Data written during failure
			for i := 0; i < 10; i++ {
				key := fmt.Sprintf("during_failure_key_%d", i)
				expectedValue := fmt.Sprintf("during_failure_value_%d", i)

				get := slave1Client.Get(ctx, key)
				Expect(get.Err()).NotTo(HaveOccurred())
				Expect(get.Val()).To(Equal(expectedValue))
			}

			// 3. Data written after recovery
			for i := 0; i < 10; i++ {
				key := fmt.Sprintf("after_recovery_key_%d", i)
				expectedValue := fmt.Sprintf("after_recovery_value_%d", i)

				get := slave1Client.Get(ctx, key)
				Expect(get.Err()).NotTo(HaveOccurred())
				Expect(get.Val()).To(Equal(expectedValue))
			}

			// Verify final consistency across all nodes
			for i := 0; i < 10; i++ {
				// Check all three sets of data on both slaves
				keys := []string{
					fmt.Sprintf("initial_key_%d", i),
					fmt.Sprintf("during_failure_key_%d", i),
					fmt.Sprintf("after_recovery_key_%d", i),
				}
				values := []string{
					fmt.Sprintf("initial_value_%d", i),
					fmt.Sprintf("during_failure_value_%d", i),
					fmt.Sprintf("after_recovery_value_%d", i),
				}

				for j, key := range keys {
					get1 := slave1Client.Get(ctx, key)
					Expect(get1.Err()).NotTo(HaveOccurred())
					Expect(get1.Val()).To(Equal(values[j]))

					get2 := slave2Client.Get(ctx, key)
					Expect(get2.Err()).NotTo(HaveOccurred())
					Expect(get2.Val()).To(Equal(values[j]))
				}
			}
		})
	})
})
