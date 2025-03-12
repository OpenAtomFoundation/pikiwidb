package metrics

import (
    "regexp"
)

func init() {
    Register(collectCommandP99Metrics)
}

var collectCommandP99Metrics = map[string]MetricConfig{
    "command_p99_info": {
        Parser: &regexParser{
            name: "command_p99_info",
            reg: regexp.MustCompile(`Command:\s*(?P<cmd>[\w]+)\s*\r?\nTotal calls:\s*(?P<total_calls>[\d.]+)\s*\r?\n(?:(?:Bucket\[(?P<bucket_time>[\d.]+)\s*ms\]:\s*(?P<bucket_count>[\d.]+)\s*\r?\n)*)TP99 ms:\s*(?P<tp99>[\d.]+)`),
            Parser: &normalParser{},
        },
        MetricMeta: MetaDatas{
            {
                Name:      "command_total_calls",
                Help:      "Total number of calls for each Pika command",
                Type:      metricTypeGauge,
                Labels:    []string{LabelNameAddr, LabelNameAlias, "cmd"},
                ValueName: "total_calls",
            },
            {
                Name:      "command_p99_latency",
                Help:      "99th percentile latency (ms) for each Pika command",
                Type:      metricTypeGauge,
                Labels:    []string{LabelNameAddr, LabelNameAlias, "cmd"},
                ValueName: "tp99",
            },
            {
                Name:      "command_latency_buckets",
                Help:      "Latency distribution buckets for Pika commands",
                Type:      metricTypeGauge,
                Labels:    []string{LabelNameAddr, LabelNameAlias, "cmd", "bucket"},
                ValueName: "bucket_count",
            },
        },
    },
}
