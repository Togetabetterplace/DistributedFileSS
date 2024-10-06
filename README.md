### DFSS

Distributed systems course design in the second semester of the third year, distributed file sharing and synchronization systems

本项目为解决跨服务器文件同步的问题而开发，主要功能为：

- 使用P2P架构实现文件共享与同步
- 使用Raft算法实现文件同步的一致性
- 使用imgui和glfw实现图像化界面
- 使用定时器实现定时同步
- 使用HTTP协议，WinSocket进行文件传输

#### 项目设计

系统本身使用p2p架构，每个节点都维护一个文件存储空间，同时存储了本节点已知的其他节点和数据源。用户可通过可视化界面对系统进行操作，主要功能有：

- 添加数据源
- 添加节点
- 设置本节点文件存储路径
- 展示当前节点信息
- 设定同步实践
- 手动同步
- 退出

#### 运行效果

系统界面
![image](https://github.com/user-attachments/assets/f25a9da1-0b0e-4949-b567-f80b1138b853)

添加节点
- 输入IP和端口号
- 点击add
![image](https://github.com/user-attachments/assets/cab516a1-3882-4c0c-a66f-29e107acb57e)

添加数据源
- 输入数据源文件名
- 输入数据源路径
- 输入哈希值
- 点击添加

![image](https://github.com/user-attachments/assets/e5316868-fe38-48a1-9347-269dc38903fd)

设定同步时间
- 可设定按随时同步，分钟同步，按天同步等

![image](https://github.com/user-attachments/assets/8368211b-a1f8-4381-bdd5-f59b74402ef2)

点击查看当前节点信息
![image](https://github.com/user-attachments/assets/1a96cc10-7a10-4169-8852-3dfeff99ff1e)


设置本节点存储路径
![image](https://github.com/user-attachments/assets/e85611b5-6644-4f17-bd26-ea1eff8d2e6f)

#### 测试

测试文件夹结构

![image](https://github.com/user-attachments/assets/fd6537be-7a79-4b17-afe6-cd05036cb740)


待同步文件

![image](https://github.com/user-attachments/assets/f51f2e5f-34ef-457e-8359-2b423cd769f6)


同步文件结果

![image](https://github.com/user-attachments/assets/4010d4e6-8029-4e70-8fe2-d6f5c45e38e5)


ps：视频音频图片等文件也可以进行传输和同步，在进行文件传输时，对于文本文件会添加HTTP相应头，如果是非结构化的文件则不添加







