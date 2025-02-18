#include <bpf/bpf.h>
#include <bpf/libbpf.h>
#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/syscall.h>
#include "perfcount_kern.skel.h" // eBPF Skeleton

static long perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
                            int cpu, int group_fd, unsigned long flags)
{
    return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}

int main(int argc, char **argv) {
    struct perfcount_kern_bpf *skel = NULL;
    struct perf_event_attr pe;
    struct bpf_link *link = NULL;
    int fd = -1;
    pid_t pid;

    // 加载 eBPF Skeleton
    skel = perfcount_kern_bpf__open_and_load();
    if (!skel) {
        fprintf(stderr, "Failed to open/load BPF skeleton\n");
        return 1;
    }

    // 初始化 eBPF Map 的初始值
    __u32 key = 0;
    long long initial_value = 0;
    if (bpf_map_update_elem(bpf_map__fd(skel->maps.counter_map), &key, &initial_value, BPF_ANY) < 0) {
        fprintf(stderr, "Failed to initialize BPF map\n");
        goto cleanup;
    }

    // 设置 perf_event 属性
    memset(&pe, 0, sizeof(pe));
    pe.type = PERF_TYPE_RAW;
    pe.size = sizeof(pe);
    pe.config = (1 << 24) | (0x01 << 8) | 0xB1;  // 示例事件编码
    pe.disabled = 1;
    pe.exclude_kernel = 1;
    pe.exclude_hv = 1;
    pe.inherit = 1;
    pe.sample_period = 100000;  // 每 100,000 次事件触发一次采样中断

    // 创建子进程
    pid = fork();
    if (pid < 0) {
        perror("fork");
        goto cleanup;
    } else if (pid == 0) {
        // 子进程：暂停等待父进程设置 perf_event
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        kill(getpid(), SIGSTOP);

        // 运行目标程序
        execl("./alloc-test", "alloc-test", NULL);

        // 如果 execl 失败
        perror("execl");
        return -1;
    }

    // 父进程：等待子进程暂停
    int status;
    waitpid(pid, &status, WUNTRACED);
    if (!WIFSTOPPED(status)) {
        fprintf(stderr, "Child did not stop as expected.\n");
        goto cleanup;
    }

    // 打开针对子进程的性能事件
    fd = perf_event_open(&pe, pid, -1, -1, 0);
    if (fd == -1) {
        perror("perf_event_open");
        goto cleanup;
    }

    // 将 eBPF 程序附加到 perf_event
    link = bpf_program__attach_perf_event(skel->progs.handle_perf_event, fd);
    if (!link || libbpf_get_error(link)) {
        fprintf(stderr, "Failed to attach perf event: %ld\n",
                link ? libbpf_get_error(link) : -1);
        goto cleanup;
    }

    // 启动计数器
    ioctl(fd, PERF_EVENT_IOC_RESET, 0);
    ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);

    // 放行子进程
    ptrace(PTRACE_DETACH, pid, NULL, NULL);

    // 等待子进程完成
    waitpid(pid, NULL, 0);

    // 停止计数器
    ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);

    // 读取 perf_event 的原始计数值
    long long raw_count = 0;
    if (read(fd, &raw_count, sizeof(raw_count)) != sizeof(raw_count)) {
        perror("Failed to read raw perf event count");
    } else {
        printf("Raw Event Count (direct read): %lld\n", raw_count);
    }

    // 从 eBPF Map 中读取计数
    long long bpf_count = 0;
    if (bpf_map_lookup_elem(bpf_map__fd(skel->maps.counter_map), &key, &bpf_count) < 0) {
        fprintf(stderr, "Failed to lookup count in BPF map\n");
    } else {
        // 计算真实事件总数
        long long total_events = bpf_count * 100000; // 乘以 sample_period
        printf("Event Count (from BPF Map): %lld (with sample_period=%d)\n", total_events, 100000);
    }

cleanup:
    if (link) bpf_link__destroy(link);
    if (fd >= 0) close(fd);
    perfcount_kern_bpf__destroy(skel);
    return 0;
}
