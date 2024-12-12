package metrics

import (
	"fmt"
	"reflect"
	"regexp"
	"strconv"
	"strings"
	"time"

	"github.com/Masterminds/semver"
	log "github.com/sirupsen/logrus"
)

const (
	defaultValue = 0
)

type statusToGaugeParser struct {
    statusMapping map[string]int
}

func (p *statusToGaugeParser) Parse(m MetricMeta, c Collector, opt ParseOption) {
    m.Lookup(func(m MetaData) {
        metric := Metric{
            MetaData:    m,
            LabelValues: make([]string, len(m.Labels)),
            Value:       defaultValue,
        }

        for i, labelName := range m.Labels {
            labelValue, ok := findInMap(labelName, opt.Extracts)
            if !ok {
                log.Debugf("statusToGaugeParser::Parse not found label value. metricName:%s labelName:%s",
                    m.Name, labelName)
            }

            metric.LabelValues[i] = labelValue
        }

        if m.ValueName != "" {
            if v, ok := findInMap(m.ValueName, opt.Extracts); !ok {
                log.Warnf("statusToGaugeParser::Parse not found value. metricName:%s valueName:%s", m.Name, m.ValueName)
                return
            } else {
                mappedValue, exists := p.statusMapping[v]
                if !exists {
                    log.Warnf("statusToGaugeParser::Parse unknown status value. metricName:%s valueName:%s rawValue:%s",
                        m.Name, m.ValueName, v)
                    mappedValue = defaultValue
                }
                metric.Value = float64(mappedValue)
            }
        }

        if err := c.Collect(metric); err != nil {
            log.Errorf("statusToGaugeParser::Parse metric collect failed. metric:%#v err:%s",
                m, m.ValueName)
        }
    })
}



type ParseOption struct {
	Version        *semver.Version
	Extracts       map[string]string
	ExtractsProxy  map[string][]int64
	Info           string
	CurrentVersion VersionChecker
}
type VersionChecker interface {
	CheckContainsEmptyValueName(key string) bool
	CheckContainsEmptyRegexName(key string) bool
	InitVersionChecker()
}
type VersionChecker336 struct {
	EmptyValueName []string
	EmptyRegexName []string
}

func (v *VersionChecker336) InitVersionChecker() {
	if v.EmptyValueName == nil {
		v.EmptyValueName = []string{
			"instantaneous_output_repl_kbps",
			"total_net_output_bytes",
			"cache_db_num",
			"hits_per_sec",
			"cache_status",
			"total_net_input_bytes",
			"instantaneous_output_kbps",
			"instantaneous_input_kbps",
			"total_net_repl_input_bytes",
			"instantaneous_input_repl_kbps",
			"slow_logs_count",
			"total_net_repl_output_bytes",
			"cache_memory",
		}
	}
	if v.EmptyRegexName == nil {

		v.EmptyRegexName = []string{
			"hitratio_per_sec",
			"total_blob_file_size",
			"block_cache_capacity",
			"background_errors",
			"num_running_flushes",
			"mem_table_flush_pending",
			"estimate_pending_compaction_bytes",
			"block_cache_pinned_usage",
			"pending_compaction_bytes_stops",
			"estimate_live_data_size",
			"pending_compaction_bytes_delays",
			"num_running_compactions",
			"live_blob_file_size",
			"cur_size_active_mem_table",
			"block_cache_usage",
			"cf_l0_file_count_limit_stops_with_ongoing_compaction",
			"cur_size_all_mem_tables",
			"num_immutable_mem_table",
			"compaction_pending",
			"live_sst_files_size",
			"memtable_limit_stops",
			"total_delays",
			"l0_file_count_limit_delays",
			"estimate_table_readers_mem",
			"num_immutable_mem_table_flushed",
			"compaction_Sum",
			"size_all_mem_tables",
			"total_sst_files_size",
			"commandstats_info",
			"num_snapshots",
			"current_super_version_number",
			"memtable_limit_delays",
			"estimate_num_keys",
			"num_blob_files",
			"total_stops",
			"cf_l0_file_count_limit_delays_with_ongoing_compaction",
			"num_live_versions",
			"l0_file_count_limit_stops",
			"compaction",
			"blob_stats",
		}

	}
}
func (v *VersionChecker336) CheckContainsEmptyValueName(key string) bool {
	for _, str := range v.EmptyValueName {
		if str == key {
			return true
		}
	}
	return false
}
func (v *VersionChecker336) CheckContainsEmptyRegexName(key string) bool {
	for _, str := range v.EmptyRegexName {
		if str == key {
			return true
		}
	}
	return false
}

type VersionChecker350 struct {
	EmptyValueName []string
	EmptyRegexName []string
}

func (v *VersionChecker350) InitVersionChecker() {
	if v.EmptyValueName == nil {
		v.EmptyValueName = []string{
			"cache_db_num",
			"cache_status",
			"cache_memory",
			"hits_per_sec",
			"slow_logs_count",
		}
	}
	if v.EmptyRegexName == nil {
		v.EmptyRegexName = []string{
			"hitratio_per_sec",
		}
	}
}
func (v *VersionChecker350) CheckContainsEmptyValueName(key string) bool {
	for _, str := range v.EmptyValueName {
		if str == key {
			return true
		}
	}
	return false
}
func (v *VersionChecker350) CheckContainsEmptyRegexName(key string) bool {
	for _, str := range v.EmptyRegexName {
		if str == key {
			return true
		}
	}
	return false
}

type VersionChecker355 struct {
	EmptyValueName []string
	EmptyRegexName []string
}

func (v *VersionChecker355) InitVersionChecker() {
	if v.EmptyValueName == nil {
		v.EmptyValueName = []string{
			"cache_db_num",
			"cache_status",
			"cache_memory",
			"hits_per_sec",
		}
	}
	if v.EmptyRegexName == nil {
		v.EmptyRegexName = []string{
			"hitratio_per_sec",
			"keyspace_info_>=3.1.0",
			"keyspace_info_all_>=3.3.3",
			"binlog_>=3.2.0",
			"keyspace_last_start_time",
			"is_scaning_keyspace",
		}
	}
}
func (v *VersionChecker355) CheckContainsEmptyValueName(key string) bool {
	for _, str := range v.EmptyValueName {
		if str == key {
			return true
		}
	}
	return false
}
func (v *VersionChecker355) CheckContainsEmptyRegexName(key string) bool {
	for _, str := range v.EmptyRegexName {
		if str == key {
			return true
		}
	}
	return false
}

type Parser interface {
	Parse(m MetricMeta, c Collector, opt ParseOption)
}

type Parsers []Parser

func (ps Parsers) Parse(m MetricMeta, c Collector, opt ParseOption) {
	for _, p := range ps {
		p.Parse(m, c, opt)
	}
}

type versionMatchParser struct {
	verC *semver.Constraints
	Parser
}

func (p *versionMatchParser) Parse(m MetricMeta, c Collector, opt ParseOption) {
	if opt.Version == nil || !p.verC.Check(opt.Version) {
		return
	}
	p.Parser.Parse(m, c, opt)
}

type Matcher interface {
	Match(v string) bool
}

type equalMatcher struct {
	v string
}

func (m *equalMatcher) Match(v string) bool {
	return strings.ToLower(v) == strings.ToLower(m.v)
}

type intMatcher struct {
	condition string
	v         int
}

func (m *intMatcher) Match(v string) bool {
	nv, err := strconv.Atoi(v)
	if err != nil {
		return false
	}

	switch m.condition {
	case ">":
		return nv > m.v
	case "<":
		return nv < m.v
	case ">=":
		return nv >= m.v
	case "<=":
		return nv <= m.v
	}
	return false
}

type keyMatchParser struct {
	matchers map[string]Matcher
	Parser
}

func (p *keyMatchParser) Parse(m MetricMeta, c Collector, opt ParseOption) {
	for key, matcher := range p.matchers {
		if v, _ := opt.Extracts[key]; !matcher.Match(v) {
			return
		}
	}
	p.Parser.Parse(m, c, opt)
}

type regexParser struct {
	name   string
	source string
	reg    *regexp.Regexp
	Parser
}

func (p *regexParser) Parse(m MetricMeta, c Collector, opt ParseOption) {
	s := opt.Info
	if p.source != "" {
		s = opt.Extracts[p.source]
	}

	matchMaps := p.regMatchesToMap(s)
	if len(matchMaps) == 0 {
		if opt.CurrentVersion == nil || !opt.CurrentVersion.CheckContainsEmptyRegexName(p.name) {
			log.Warnf("regexParser::Parse reg find sub match nil. name:%s", p.name)
		}
	}
	extracts := make(map[string]string)
	for k, v := range opt.Extracts {
		extracts[k] = v
	}

	opt.Extracts = extracts
	for _, matches := range matchMaps {
		for k, v := range matches {
			extracts[k] = v
		}
		p.Parser.Parse(m, c, opt)
	}
}

func (p *regexParser) regMatchesToMap(s string) []map[string]string {
	if s == "" {
		return nil
	}

	multiMatches := p.reg.FindAllStringSubmatch(s, -1)
	if len(multiMatches) == 0 {
		return nil
	}

	ms := make([]map[string]string, len(multiMatches))
	for i, matches := range multiMatches {
		ms[i] = make(map[string]string)
		for j, name := range p.reg.SubexpNames() {
			ms[i][name] = trimSpace(matches[j])
		}
	}
	return ms
}

type normalParser struct{}

func (p *normalParser) Parse(m MetricMeta, c Collector, opt ParseOption) {
	m.Lookup(func(m MetaData) {
		metric := Metric{
			MetaData:    m,
			LabelValues: make([]string, len(m.Labels)),
			Value:       defaultValue,
		}

		for i, labelName := range m.Labels {
			labelValue, ok := findInMap(labelName, opt.Extracts)
			if !ok {
				log.Debugf("normalParser::Parse not found label value. metricName:%s labelName:%s",
					m.Name, labelName)
			}

			metric.LabelValues[i] = labelValue
		}

		if m.ValueName != "" {
			if v, ok := findInMap(m.ValueName, opt.Extracts); !ok {
				if opt.CurrentVersion == nil || !opt.CurrentVersion.CheckContainsEmptyValueName(m.ValueName) {
					log.Warnf("normalParser::Parse not found value. metricName:%s valueName:%s", m.Name, m.ValueName)
				}
				return

			} else {
				metric.Value = convertToFloat64(v)
			}
		}

		if err := c.Collect(metric); err != nil {
			log.Errorf("normalParser::Parse metric collect failed. metric:%#v err:%s",
				m, m.ValueName)
		}
	})
}

type timeParser struct{}

func (p *timeParser) Parse(m MetricMeta, c Collector, opt ParseOption) {
	m.Lookup(func(m MetaData) {
		metric := Metric{
			MetaData:    m,
			LabelValues: make([]string, len(m.Labels)),
			Value:       defaultValue,
		}

		for i, labelName := range m.Labels {
			labelValue, ok := findInMap(labelName, opt.Extracts)
			if !ok {
				log.Debugf("timeParser::Parse not found label value. metricName:%s labelName:%s",
					m.Name, labelName)
			}

			metric.LabelValues[i] = labelValue
		}

		if m.ValueName != "" {
			if v, ok := findInMap(m.ValueName, opt.Extracts); !ok {
				return
			} else {
				t, err := convertTimeToUnix(v)
				if err != nil {
					log.Warnf("time is '0' and cannot be parsed", err)
				}
				metric.Value = float64(t)
			}
		}

		if err := c.Collect(metric); err != nil {
			log.Errorf("timeParser::Parse metric collect failed. metric:%#v err:%s",
				m, m.ValueName)
		}
	})
}

func findInMap(key string, ms ...map[string]string) (string, bool) {

	for _, m := range ms {
		if v, ok := m[key]; ok {
			return v, true
		}
	}
	return "", false
}
func trimSpace(s string) string {
	return strings.TrimRight(strings.TrimLeft(s, " "), " ")
}

func convertToFloat64(s string) float64 {
	s = strings.ToLower(s)

	switch s {
	case "yes", "up", "online", "true", "ok":
		return 1
	case "no", "down", "offline", "null", "false":
		return 0
	}

	n, err := strconv.ParseFloat(s, 64)
	if err != nil {
		return 0
	}
	return n
}

func mustNewVersionConstraint(version string) *semver.Constraints {
	c, err := semver.NewConstraint(version)
	if err != nil {
		panic(err)
	}
	return c
}

const TimeLayout = "2006-01-02 15:04:05"

func convertTimeToUnix(ts string) (int64, error) {
	t, err := time.Parse(TimeLayout, ts)
	if err != nil {
		log.Warnf("format time failed, ts: %s, err: %v", ts, err)
		return 0, nil
	}
	return t.Unix(), nil
}

type proxyParser struct{}

func (p *proxyParser) Parse(m MetricMeta, c Collector, opt ParseOption) {
	m.Lookup(func(m MetaData) {
		for opstr, v := range opt.ExtractsProxy {
			if len(v) < 17 {
				paddedV := make([]int64, 17)
				copy(paddedV, v)
				v = paddedV
			}
			metric := Metric{
				MetaData:    m,
				LabelValues: make([]string, len(m.Labels)),
				Value:       defaultValue,
			}

			for i := 0; i < len(m.Labels)-1; i++ {
				labelValue, ok := findInMap(m.Labels[i], opt.Extracts)
				if !ok {
					log.Debugf("normalParser::Parse not found label value. metricName:%s labelName:%s",
						m.Name, m.Labels[i])
				}

				metric.LabelValues[i] = labelValue
			}
			metric.LabelValues[len(m.Labels)-1] = opstr

			switch m.ValueName {
			case "calls":
				metric.Value = convertToFloat64(strconv.FormatInt(v[0], 10))
			case "usecs_percall":
				metric.Value = convertToFloat64(strconv.FormatInt(v[1], 10))
			case "fails":
				metric.Value = convertToFloat64(strconv.FormatInt(v[2], 10))
			case "max_delay":
				metric.Value = convertToFloat64(strconv.FormatInt(v[3], 10))
			case "tp90":
				metric.Value = convertToFloat64(strconv.FormatInt(v[4], 10))
			case "tp99":
				metric.Value = convertToFloat64(strconv.FormatInt(v[5], 10))
			case "tp999":
				metric.Value = convertToFloat64(strconv.FormatInt(v[6], 10))
			case "tp100":
				metric.Value = convertToFloat64(strconv.FormatInt(v[7], 10))
			case "delayCount":
				metric.Value = convertToFloat64(strconv.FormatInt(v[8], 10))
			case "delay50ms":
				metric.Value = convertToFloat64(strconv.FormatInt(v[9], 10))
			case "delay100ms":
				metric.Value = convertToFloat64(strconv.FormatInt(v[10], 10))
			case "delay200ms":
				metric.Value = convertToFloat64(strconv.FormatInt(v[11], 10))
			case "delay300ms":
				metric.Value = convertToFloat64(strconv.FormatInt(v[12], 10))
			case "delay500ms":
				metric.Value = convertToFloat64(strconv.FormatInt(v[13], 10))
			case "delay1s":
				metric.Value = convertToFloat64(strconv.FormatInt(v[14], 10))
			case "delay2s":
				metric.Value = convertToFloat64(strconv.FormatInt(v[15], 10))
			case "delay3s":
				metric.Value = convertToFloat64(strconv.FormatInt(v[16], 10))
			}

			if err := c.Collect(metric); err != nil {
				log.Errorf("proxyParser::Parse metric collect failed. metric:%#v err:%s",
					m, m.ValueName)
			}
		}
	})
}

func StructToMap(obj interface{}) (map[string]string, map[string][]int64, error) {
	result := make(map[string]string)
	cmdResult := make(map[string][]int64)
	objValue := reflect.ValueOf(obj)
	objType := objValue.Type()

	for i := 0; i < objValue.NumField(); i++ {
		field := objValue.Field(i)
		fieldType := objType.Field(i)
		jsonName := fieldType.Tag.Get("json")
		if jsonName == "" {
			jsonName = fieldType.Name
		}
		value := field.Interface()

		if field.Kind() == reflect.Struct {
			subMap, subCmdMap, _ := StructToMap(value)
			for k, v := range subMap {
				result[strings.ToLower(jsonName+"_"+k)] = v
			}
			for k, v := range subCmdMap {
				if v != nil {
					for index := range v {
						cmdResult[k] = append(cmdResult[k], v[index])
					}
				}
			}
		} else if field.Kind() == reflect.Slice && field.Len() > 0 {
			for j := 0; j < field.Len(); j++ {
				elemType := field.Index(j).Type()
				elemValue := field.Index(j)
				if elemType.Kind() == reflect.Struct && elemValue.NumField() > 0 {
					var key string = elemValue.Field(0).String()
					for p := 1; p < elemValue.NumField(); p++ {
						cmdResult[key] = append(cmdResult[key], elemValue.Field(p).Int())
					}
				} else {
					result[strings.ToLower(jsonName)] = fmt.Sprintf("%v", value)
				}
			}
		} else {
			result[strings.ToLower(jsonName)] = fmt.Sprintf("%v", value)
		}
	}
	return result, cmdResult, nil
}
