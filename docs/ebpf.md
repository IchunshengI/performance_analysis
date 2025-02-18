# 附录. eBPF部分
## 1.  eBPF 程序类型和附加点类型
eBPF程序类型是通过**SEC()宏**在代码中声明的，该宏定义了程序的挂载点
![Alt text](../image/image-1.png)
对应的挂载函数可以使用下面命令查看
```c++
su
bpftool feature
```
![Alt text](../image/image-2.png)

## 2. 安装教程
```c++
git clone git@github.com:libbpf/libbpf.git
cd libbpf/src
make
sudo make install
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
sudo apt install libelf-dev /* 命令行安装libelf，用于加载elf文件(ebpf程序的输出格式) */
sudo apt install clang llvm /* 便于编译epbf文件 */
sudo apt install linux-headers-$(uname -r) /* 安装内核头文件 */
```
出现如下字样即可说明安装成功
![Alt text](../image/image.png)

## 编译目标
```c++
// 内核态
clang -O2 -g -target bpf -c perfcount_kern.bpf.c -o perfcount_kern.o
bpftool gen skeleton perfcount_kern.o > perfcount_kern.skel.h
// 用户态
clang -O2 -g -Wall -I/usr/include/bpf -I. perfcount_user.c -o perfcount_user -lbpf -lelf -lz
```
