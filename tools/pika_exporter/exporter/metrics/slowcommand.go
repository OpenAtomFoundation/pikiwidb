package metrics

import (
    "regexp"
)

func init() {
    Register(collectSlowCommandMetrics)
}

var collectSlowCommandMetrics = map[string]MetricConfig{
    "slow_command_info": {
        Parser: &regexParser{
            name: "slow_command_info",
            reg: regexp.MustCompile(`Command:\s*(?P<cmd>[\w]+),\s*Slow count:\s*(?P<slow_count>[\d]+)`),
            Parser: &normalParser{},
        },
        MetricMeta: MetaDatas{
            {
                Name:      "command_slow_count",
                Help:      "Number of times each Pika command was slow",
                Type:      metricTypeGauge,
                Labels:    []string{LabelNameAddr, LabelNameAlias, "cmd"},
                ValueName: "slow_count",
            },
        },
    },
}
