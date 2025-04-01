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
            reg: regexp.MustCompile(`Command:\s*(?P<cmd>\S+)\s*\r?\n(?:.*\r?\n)*?TP99 ms:\s*(?P<tp99>[\d.]+)\s*\r?\n.*?TP999 ms:\s*(?P<tp999>[\d.]+)\s*\r?\n.*?TP9999 ms:\s*(?P<tp9999>[\d.]+)`),
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
            {
                Name:      "command_p999_latency",
                Help:      "99.9th percentile latency (ms) for each Pika command",
                Type:      metricTypeGauge,
                Labels:    []string{LabelNameAddr, LabelNameAlias, "cmd"},
                ValueName: "tp999",
            },
            {
                Name:      "command_p9999_latency",
                Help:      "99.99th percentile latency (ms) for each Pika command",
                Type:      metricTypeGauge,
                Labels:    []string{LabelNameAddr, LabelNameAlias, "cmd"},
                ValueName: "tp9999",
            },
        },
    },
}