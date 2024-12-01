#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/perf_event.h>
#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <signal.h>
#include <errno.h>

static long perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
                            int cpu, int group_fd, unsigned long flags)
{
    return syscall(__NR_perf_event_open, hw_event, pid, cpu,
                   group_fd, flags);
}

int main()
{
    struct perf_event_attr pe;
    memset(&pe, 0, sizeof(struct perf_event_attr));

    pe.type = PERF_TYPE_RAW;
    pe.size = sizeof(struct perf_event_attr);
    pe.config = (1 << 24) | (0x01 << 8) | 0xB1; // 正确的事件配置
    pe.disabled = 1;       // 开始时禁用计数器
    pe.exclude_kernel = 1;
    pe.exclude_hv = 1;
    pe.inherit = 1;        // 允许子进程继承计数器

    pid_t pid = fork();
    if (pid == 0) {
        // 子进程

        // 使子进程暂停，等待父进程设置性能计数器
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        kill(getpid(), SIGSTOP);

        // 运行目标程序
        execl("./alloc-test", "alloc-test", NULL);

        // 如果 execl 失败
        perror("execl");
        return -1;
    } else if (pid > 0) {
        // 父进程

        // 等待子进程暂停
        int status;
        waitpid(pid, &status, WUNTRACED);
        if (!WIFSTOPPED(status)) {
            fprintf(stderr, "Child did not stop as expected.\n");
            return -1;
        }

        // 打开针对子进程的性能事件
        int fd = perf_event_open(&pe, pid, -1, -1, 0);
        if (fd == -1) {
            perror("perf_event_open");
            return -1;
        }

        // 重置和启动计数器
        ioctl(fd, PERF_EVENT_IOC_RESET, 0);
        ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);

        // 让子进程继续执行
        ptrace(PTRACE_DETACH, pid, NULL, NULL);

        // 等待子进程结束
        waitpid(pid, NULL, 0);

        // 停止计数器
        ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);

        long long count;
        if (read(fd, &count, sizeof(long long)) == -1) {
            perror("read");
            return -1;
        }
 //42591586823
 //42681728693    
        printf("Event Count: %lld\n", count);

        close(fd);
    } else {
        // fork 失败
        perror("fork");
        return -1;
    }

    return 0;
}