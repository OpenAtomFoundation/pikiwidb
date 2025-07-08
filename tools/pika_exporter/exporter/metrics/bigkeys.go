package metrics

import (
	"regexp"
	"strconv"
)

type multiMatchRegexParser struct {
	name   string
	source string
	reg    *regexp.Regexp
}

func (p *multiMatchRegexParser) Parse(m MetricMeta, c Collector, opt ParseOption) {
	s := opt.Info
	if p.source != "" {
		s, _ = opt.Extracts[p.source]
	}

	matches := p.reg.FindAllStringSubmatch(s, -1)
	if matches == nil || len(matches) == 0 {
		return
	}

	processedKeys := make(map[string]bool)

	for _, match := range matches {
		extracts := make(map[string]string)
		for k, v := range opt.Extracts {
			extracts[k] = v
		}

		for i, name := range p.reg.SubexpNames() {
			if i > 0 && i < len(match) {
				if name != "" {
					extracts[name] = trimSpace(match[i])
				} else {
					extracts[strconv.Itoa(i)] = trimSpace(match[i])
				}
			}
		}

		key := ""
		if val, ok := extracts["key"]; ok {
			key = val
		}

		if key != "" && processedKeys[key] {
			continue
		}

		if key != "" {
			processedKeys[key] = true
		}

		opt.Extracts = extracts
		(&normalParser{}).Parse(m, c, opt)
	}
}

func RegisterBigKeys() {
	Register(collectBigKeysMetrics)
}

var collectBigKeysMetrics = map[string]MetricConfig{
	"bigkeys_string": {
		Parser: &multiMatchRegexParser{
			name:   "bigkeys_string",
			source: "bigkeys_output",
			reg: regexp.MustCompile(
				`Type: string, key: (?P<key>[^,]+), key_length: (?P<key_length>\d+), value_length: (?P<value_length>\d+)`,
			),
		},
		MetricMeta: MetaDatas{
			{
				Name:      "bigkeys_string_key_length",
				Help:      "Big key length for string type",
				Type:      metricTypeGauge,
				Labels:    []string{LabelNameAddr, LabelNameAlias, "type", "key"},
				ValueName: "key_length",
			},
			{
				Name:      "bigkeys_string_value_length",
				Help:      "Big key value length for string type",
				Type:      metricTypeGauge,
				Labels:    []string{LabelNameAddr, LabelNameAlias, "type", "key"},
				ValueName: "value_length",
			},
		},
	},
	"bigkeys_complex": {
		Parser: &multiMatchRegexParser{
			name:   "bigkeys_complex",
			source: "bigkeys_output",
			reg: regexp.MustCompile(
				`Type: (?P<type>hash|list|set|zset), key: (?P<key>[^,]+), key_length: (?P<key_length>\d+), member_size: (?P<member_size>\d+)`,
			),
		},
		MetricMeta: MetaDatas{
			{
				Name:      "bigkeys_member_size",
				Help:      "Big key member size by type and key",
				Type:      metricTypeGauge,
				Labels:    []string{LabelNameAddr, LabelNameAlias, "type", "key"},
				ValueName: "member_size",
			},
			{
				Name:      "bigkeys_complex_key_length",
				Help:      "Big key length for complex type",
				Type:      metricTypeGauge,
				Labels:    []string{LabelNameAddr, LabelNameAlias, "type", "key"},
				ValueName: "key_length",
			},
		},
	},
}