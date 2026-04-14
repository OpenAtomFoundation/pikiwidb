package exporter

import (
	"fmt"

	"github.com/pelletier/go-toml"
	log "github.com/sirupsen/logrus"
)

var InfoConf *InfoConfig

var InfoConfigPath string

type InfoConfig struct {
	Server       bool `toml:"server"`
	Data         bool `toml:"data"`
	Clients      bool `toml:"clients"`
	Stats        bool `toml:"stats"`
	CPU          bool `toml:"cpu"`
	Replication  bool `toml:"replication"`
	Keyspace     bool `toml:"keyspace"`
	Execcount    bool `toml:"execcount"`
	Commandstats bool `toml:"commandstats"`
	Rocksdb      bool `toml:"rocksdb"`
	Cache        bool `toml:"cache"`

	Info    bool
	InfoAll bool
}

func LoadConfig() error {
	log.Debugln("Update configuration")
	
	// Initialize default configuration
	InfoConf = &InfoConfig{
		Server:       true,
		Data:         true,
		Clients:      true,
		Stats:        true,
		CPU:          true,
		Replication:  true,
		Keyspace:     true,
		Execcount:    true,
		Commandstats: true,
		Rocksdb:      false,
		Cache:        true,
	}
	
	// Try to load config file if path is provided
	if InfoConfigPath != "" {
		err := readConfig(InfoConfigPath)
		if err != nil {
			log.Warnf("Failed to load config file %s: %s, using default configuration", InfoConfigPath, err)
		}
	}

	InfoConf.CheckInfo()
	//InfoConf.Display()

	return nil
}

func readConfig(filePath string) error {
	conf := InfoConfig{}
	tree, err := toml.LoadFile(filePath)
	if err != nil {
		return fmt.Errorf("unable to load toml file %s: %s", filePath, err)
	}

	// Unmarshal the TOML data into the Config struct
	err = tree.Unmarshal(&conf)
	if err != nil {
		return fmt.Errorf("unable to parse toml file %s: %s", filePath, err)
	}

	InfoConf = &conf

	return nil
}

// Display config
func (c *InfoConfig) Display() {
	log.Println("Server:", c.Server)
	log.Println("Data:", c.Data)
	log.Println("Clients:", c.Clients)
	log.Println("Stats:", c.Stats)
	log.Println("CPU:", c.CPU)
	log.Println("Replication:", c.Replication)
	log.Println("Keyspace:", c.Keyspace)
	log.Println("Execcount:", c.Execcount)
	log.Println("Commandstats:", c.Commandstats)
	log.Println("Rocksdb:", c.Rocksdb)
	log.Println("Cache:", c.Cache)
	log.Println("Info:", c.Info)
	log.Println("InfoAll:", c.InfoAll)
}

func (c *InfoConfig) CheckInfo() {
	c.InfoAll = false
	c.Info = false

	// For Pika versions, we need to enable Info if any of the core modules are enabled
	// This ensures basic metrics are collected
	if c.Server || c.Data || c.Clients || c.Stats || c.CPU || c.Replication || c.Keyspace {
		c.Info = true
	}

	// InfoAll should only be enabled if all modules are enabled
	// For Pika 3.2.x versions, we should NOT use InfoAll because INFO ALL command
	// has different output format compared to newer versions
	// The version detection will be handled in the exporter, but here we ensure
	// that Info is enabled when needed
	if c.Info && c.Execcount && c.Commandstats && c.Rocksdb && c.Cache {
		c.InfoAll = true
	}
}
