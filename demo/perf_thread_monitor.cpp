#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/perf_event.h>
#include <string.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <errno.h>

static long perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
                            int cpu, int group_fd, unsigned long flags)
{
    return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}

// 定义线程参数结构体
typedef struct {
    int thread_id;
    long long event_count;
} thread_data_t;

// 模拟任务1：大规模整数加法
void task1() {
    long long sum = 0;
    for (long i = 0; i < 100000000L; i++) {
        sum += i;
    }
}

// 模拟任务2：矩阵乘法
void task2() {
    int size = 100;
    int matrix1[size][size], matrix2[size][size], result[size][size];
    memset(result, 0, sizeof(result));

    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            matrix1[i][j] = i + j;
            matrix2[i][j] = i - j;
        }
    }

    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            for (int k = 0; k < size; k++) {
                result[i][j] += matrix1[i][k] * matrix2[k][j];
            }
        }
    }
}

void *thread_function(void *arg)
{
    thread_data_t *data = (thread_data_t *)arg;

    // 配置 perf_event_attr
    struct perf_event_attr pe;
    memset(&pe, 0, sizeof(struct perf_event_attr));
    pe.type = PERF_TYPE_RAW;
    pe.size = sizeof(struct perf_event_attr);
    pe.config = (1 << 24) | (0x01 << 8) | 0xB1; // 正确的事件配置
    pe.disabled = 1;       // 开始时禁用计数器
    pe.exclude_kernel = 1;
    pe.exclude_hv = 1;

    // 打开性能事件计数器
    int fd = perf_event_open(&pe, 0, -1, -1, 0);
    if (fd == -1) {
        perror("perf_event_open");
        pthread_exit(NULL);
    }

    // 启动计数器
    ioctl(fd, PERF_EVENT_IOC_RESET, 0);
    ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);

    // 执行不同任务
    if (data->thread_id == 1) {
        task1();
    } else if (data->thread_id == 2) {
        task2();
    }

    // 停止计数器
    ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);

    // 读取计数器
    if (read(fd, &data->event_count, sizeof(long long)) == -1) {
        perror("read");
        close(fd);
        pthread_exit(NULL);
    }

    close(fd);
    pthread_exit(NULL);
}

int main()
{
    pthread_t threads[2];
    thread_data_t thread_data[2];

    // 创建两个线程
    for (int i = 0; i < 2; i++) {
        thread_data[i].thread_id = i + 1;
        if (pthread_create(&threads[i], NULL, thread_function, &thread_data[i]) != 0) {
            perror("pthread_create");
            return -1;
        }
    }

    // 等待两个线程结束
    for (int i = 0; i < 2; i++) {
        pthread_join(threads[i], NULL);
    }

    // 输出线程的性能事件统计结果
    for (int i = 0; i < 2; i++) {
        printf("Thread %d Event Count: %lld\n", thread_data[i].thread_id, thread_data[i].event_count);
    }

    return 0;
}
