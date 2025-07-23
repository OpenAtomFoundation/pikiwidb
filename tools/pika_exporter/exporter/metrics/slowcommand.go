package metrics

import (
    "regexp"
)

func RegisterSlowCommand() {
    Register(collectSlowCommandMetrics)
}

var collectSlowCommandMetrics = map[string]MetricConfig{
    "slow_command_info": {
        Parser: &regexParser{
            name: "slow_command_info",
            reg: regexp.MustCompile(`(?P<cmd>\S+):slow_count=(?P<slow_count>\d+)`),
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
