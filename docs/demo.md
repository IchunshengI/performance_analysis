

目前是perf_event_open+read的方式直接读取，测试代码可以参考[这里](../demo/READMD.md)

# 预期功能

1. 添加每个瓶颈的计算实现并增加对外接口

2. 任务执行后的 流图可视化 + 瓶颈可视化

预期效果如下，生成对应的svg图片并能展示结点连接信息，点击结点能够展示详细的瓶颈，同时结点提供名字
![Alt text](image/image-4.png)

# 第一部分实现
```c++
int fd = perf_event_open(&pe, 0, -1, -1, 0);  /* 设置好*/
ioctl(fd, PERF_EVENT_IOC_RESET, 0); /* 重置计数器 */
ioctl(fd, PERF_EVENT_IOC_ENABLE, 0); /* 是能计数器 */
ioctl(fd, PERF_EVENT_IOC_DISABLE, 0); /* 停止计数器 */

// map映射里面直接把fd映射过去就
```