# 头文件: shmfifo.h
##  结构：
####  struct ShmFifo<br>
#####  说明：
&emsp;&emsp;shm fifo运行环境结构，保存shm fifo所需要的信息

##  函数：
#### struct ShmFifo\* ShmFifoOpen(const char \*path, size_t msg_size, size_t msg_count) 
###### 功能：
&emsp;&emsp;打开或创建一个管道
###### 参数：
|参数名|说明|
|------|------|
|path|管道文件路径|
|msg_size|管道中每个消息的最大大小|
|msg_count|管道的最大长度即最多的消息个数|
###### 返回值：
返回管道的句柄
|值|说明|
|---|---|
|非NULL|成功|
|NULL|失败|

----
#### void ShmFileClose(struct ShmFile \*fifo)
###### 功能：
&emsp;&emsp;关闭管道
###### 参数：
|参数名|说明|
|------|------|
|fifo|管道句柄|
###### 返回值：
无

----
#### ssize_t ShmFifoPush(struct ShmFile \*fifo, const char *\buf, const size_t buf_size) 
###### 功能：

&emsp;&emsp;将数据压入管道
###### 参数：

|参数名|说明|
|------|------|
|fifo|管道句柄|
|buf|写入管道的数据缓冲区|
|buf_size|写入管道缓冲区的大小，不能超过msg_size,超过时被截断|
###### 返回值：

|值|说明|
|---|---|
|<0|错误号|
|>=0|实际写入的字节数|


----
#### int ShmFifoPop(struct ShmFifo \*fifo)
###### 功能：
&emsp;&emsp;从管道的头部弹出数据
###### 参数：

|参数名|说明|
|------|------|
|fifo|管道句柄|

###### 返回值：

|值|说明|
|---|---|
|<0|错误号|
|0|成功|


----
#### ssize_t ShmFifoPopData(struct ShmFifo \*fifo, char \* const buf, const size_t buf_size)
###### 功能：
&emsp;&emsp;从管道的头部弹出数据,并赋值到buf中
###### 参数：

|参数名|说明|
|------|------|
|fifo|管道句柄|
|buf|接收数据的缓冲区|
|buf_size|接收数据缓冲区大小|

###### 返回值：

|值|说明|
|---|---|
|<0|错误号|
|>=0|读到的字节数|

----
#### void\* ShmFifoTop(struct ShmFifo \*fifo, size_t \*size)
###### 功能：
&emsp;&emsp;获取头部元素，但不弹出管道
###### 参数：

|参数名|说明|
|------|------|
|fifo|管道句柄|
|size|用于保存头部元素消息大小|


###### 返回值：

|值|说明|
|---|---|
|NULL|错误|
|非NULL|管道头部数据地址|
