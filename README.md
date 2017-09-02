# Saber -- 一个高性能、高可用的分布式服务框架
## 简介
Saber是一个分布式服务框架，主要参考了Google的Chubby和Apache的Zookeeper，在Voyager和Skywalker的基础上实现高效且可靠的分布式协调服务。
<br/>
<br/>[![Build Status](https://travis-ci.org/QiumingLu/saber.svg?branch=master)](https://travis-ci.org/QiumingLu/saber)
<br/>
<br/>Author: Mirants Lu (mirantslu@gmail.com) 
<br/>
<br/>**Saber主要包含以下几个部分**：
<br/>
<br/>1. **util**: 基础库模块，包括互斥量、条件变量、线程、时间处理等基础工具。
<br/>2. **proto**: Saber的消息协议，在Google的Protobuf的基础上搭建。 
<br/>3. **service**: 事件通知机制，对外开放事件通知接口。
<br/>4. **client**: Saber的客户端实现，对外提供节点的创建、删除、读写等接口。
<br/>5. **server**:  Saber的服务端实现，对外提供服务的启动等接口。
<br/>6. **main**: Saber的可执行程序实现。
<br/>7. **test**: Saber的使用和测试示例。

## 特性
* 同时提供Client端和Server端。
* 采用分桶策略来管理会话。
* 提供事件通知机制。
* 严格地顺序访问控制。
* 只有Master节点才能处理读写请求。
* 基于Voyager来完成网络传输功能。
* 基于Skywalker来完成分布式一致性功能。
* 基于Protobuf来完成消息的序列化和反序列化。

## 局限
* 目前只能创建永久节点，不能创建临时节点。

## 使用场景
* 数据分布/订阅
* 负载均衡
* 命名服务

## 性能

## 兼容性
Saber只支持Linux，macOS 等类Unix平台，不支持Windows平台。以下是一些曾测试的平台/编译器组合：
* Linux 4.4.0，GCC 5.4.0 
* macOS 10.12，Clang 3.6.0

## 编译安装
(1) LevelDB编译安装(https://github.com/google/leveldb/blob/master/README.md) 
* 进入third_party/leveldb目录 
* 执行 make 
* 执行 sudo cp -rf out-shared/libleveldb.* /usr/local/lib/ 
* 执行 sudo cp -rf include/leveldb /usr/local/include

(2) Protobuf编译安装(https://github.com/google/protobuf/blob/master/src/README.md) 
* 进入third_party/protobuf目录 
* 执行 ./autogen.sh
* 执行 ./configure 
* 执行 make && sudo make install

(3) Voyager编译安装(https://github.com/QiumingLu/voyager/blob/master/README.md) 
* 进入third_party/voyager目录
* 执行./build.sh
* 进入./build/release目录
* 执行sudo make install

(4) Skywalker编译安装
* 进入third_party/skwalker目录
* 执行./build.sh
* 进入./build/release目录
* 执行sudo make install

(5) Saber编译安装
* 执行./build.sh
* 进入./build/release目录
* 执行sudo make install
