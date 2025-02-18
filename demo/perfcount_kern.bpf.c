// 文件：perfcount_kern.bpf.c

#include <linux/types.h>
#include <linux/ptrace.h>
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>

// 这里我们用一个全局的 BPF map 存储计数
// 如果事件出现频率较高，可以考虑使用 percpu map 或其它高效计数方式

struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 1);
    __type(key, __u32);
    __type(value, long long);
} counter_map SEC(".maps");

// PERF_EVENT 类型的 eBPF 程序，在对应硬件事件发生时会触发
SEC("perf_event")
int handle_perf_event(struct bpf_perf_event_data *ctx)
{
    // key 先固定为 0，只有一个计数器
    __u32 key = 0;
    long long *value = bpf_map_lookup_elem(&counter_map, &key);
    if (value) {
        __sync_fetch_and_add(value, 1);
    }
    return 0;
}

char _license[] SEC("license") = "GPL";
