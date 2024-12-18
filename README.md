# 功能简述: 实现流图中的结点性能瓶颈分析
![Alt text](./docs/image/image-3.png)
# 1. 自上而下的性能分析方法和计算器体系结构
VTune Amplifier 中的指标采用分层方式组织，用于识别微架构瓶颈。此层次结构和方法称为自上而下的分析方法 - 一种简化、准确且快速的方法，用于识别架构和微架构级别的关键瓶颈。常见的性能瓶颈以分层结构组织，并使用独立于微架构的指标加权其成本。因此，层次结构在处理器代际之间保持一致且向前兼容，从而降低了传统上理解新微架构及其模型特定事件所需的高学习曲线。

[1.相关论文查看](https://www.intel.com/content/www/us/en/developer/articles/technical/understanding-how-general-exploration-works-in-intel-vtune-amplifier-xe.html)    
[2.相关原理查看](https://www.intel.com/content/www/us/en/developer/articles/technical/understanding-how-general-exploration-works-in-intel-vtune-amplifier-xe.html)                     
[3.分析方法查看](https://www.intel.com/content/www/us/en/docs/vtune-profiler/cookbook/2023-0/top-down-microarchitecture-analysis-method.html#GUID-FEA77CD8-F9F1-446A-8102-07D3234CDB68)

主要特点： 在传统 PMU（性能检测单元）中添加8个简单的新性能事件可以快速且正确地识别目标程序主要的性能瓶颈

关于对应的计算公式和事件列表请[点击此处跳转](./docs/intel_paper.md)

# 2. 实现方式
使用perf_event_open + ebpf 的方式完成计数统计，以动态库的方式提供对外接口，详情参见[实现思路](./docs/demo.md)


# 3. 使用方式
未完待续。。。。。。

