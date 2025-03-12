package metrics

import (
    "regexp"
)

func RegisterCommandP99() {
    Register(collectCommandP99Metrics)
}

var collectCommandP99Metrics = map[string]MetricConfig{
    "command_p99_info": {
        Parser: &regexParser{
            name: "command_p99_info",
            reg: regexp.MustCompile(`Command:\s*(?P<cmd>\S+)\s*\r?\n(?:.*\r?\n)*?TP99 ms:\s*(?P<tp99>[\d.]+)`),
            Parser: &normalParser{},
        },
        MetricMeta: MetaDatas{
            {
                Name:      "command_p99_latency",
                Help:      "99th percentile latency (ms) for each Pika command",
                Type:      metricTypeGauge,
                Labels:    []string{LabelNameAddr, LabelNameAlias, "cmd"},
                ValueName: "tp99",
            },
        },
    },
}
