# X-Robot
一个为BDS定制的轻量QQ机器人

>功能列表

1. MC聊天->QQ的转发
2. list查在线玩家
3. QQ中#发送消息到mc
4. QQ中管理员以上级别"/命令"控制台执行"命令"
5. 群员退群取消白名单,"绑定 ****"来绑定,"查询绑定"来查询,管理员使用"查询绑定 atQQ/XboxID"来查询群成员绑定"
6. 发送"未绑定名单"获取摸鱼人员名单,发送“删除绑定 QQ号”来删除绑定+白名单
7. 发送"查服"来获取服务器信息
8. 发送"菜单"获取指令列表
9. 开服，关服，崩服重启支持
10. 使用"查云黑 玩家名/at“来查讯是否在云黑
11. 自定义指令

>选择我们有哪些好处?

1. 性能占用少
   使用C++开发，最大程度优化插件性能。插件内存占用不过50MB，CPU占用更是少之又少

2. 稳定，完善
   插件开发经历了4个月的测试(到目前为止)表现稳定，bug少。功能完善且配置方便。只需寥寥几笔即可完成配置，可谓是懒人必备

3. 懒人
   插件配置方便，即装即用，无需额外配置

4. 无UI
   没有UI的机器人，所有配置均可通过修改配置文件达到，Linux，面板服也能快乐使用.

>安装指南

Lip安装（首选）
1. BDS根目录打开cmd/powershell
2. 输入```lip install github.com/xingshuyu/x-robot-tth@1.1.0```

传统安装
1. 在 [Release](https://github.com/XingShuyu/X-Robot/releases)中下载X-Robot.zip，并解压在BDS根目录中，exe和bedrock_server_mod.exe在同一目录
2. 启动Manager.exe，第一次启动会要求配置，按顺序输入机器人QQ号，QQ密码，QQ群号，服务器名称即可配置完成。机器人已启动
3. 在服务器配置好后，启动BDS，或者在QQ群里面发“开服”即可启动服务器。看到“服务器已启动”即开服成功。

配置文件详解:


![image](https://user-images.githubusercontent.com/82715884/207810448-0243a1da-2e92-4763-a70d-827558429d71.png)

>常见问题
Q：面板服总是报“CQ疑似异常，正在尝试重启"，QQ发消息总是不回应
A：把/plugins/X-Robot/go-cqhttp/config.yml中的
```
heartbeat:
  interval: 5
```
替换成
```
heartbeat:
  interval: -1
```

Q：非面板服开服卡进度，双击启动bedrock_server_mod可以启动
A：把/plugins/X-Robot/RobotInfo.json中的start_mode改成false

Q：想用中文的服务器名/初始化后服务器无法启动或启动有问题
A：手动配置/plugins/X-Robot/RobotInfo.json中的
```
    "QQ_group_id": 这里换成你的QQ群号
    "manager": {
        "cqhttp_config": true,
    }
    "serverName": "这里换成你的服务器名字",
```

# 赞助
不要求强制赞助，但是你的赞助可以帮助我更好的发展
* [爱发电](https://afdian.net/@X-Robot)

>功能使用

>>自定义指令功能
1. 打开插件中的Message文件
2. 根据范例，依次往后排序号0，1，2...
3. 其中的QQ表示QQ收到的信息，mc表示QQ收到信息后在mc聊天板发送的东西，cmd表示QQ收到消息后控制台执行的命令
4. 这是一个范例，执行效果为清理掉落物
```
{
  "0": {
    "cmd": "help",
    "mc": "test report",
    "QQ": "自定义命令范例"
  },
  "1": {
    "QQ": "清理掉落物",
    "mc": "开始清理掉落物",
    "cmd": "kill @e[type=item]"
}
```
5. 注意，自定义指令为实时加载，编辑完后保存，无需重启服务器直接就能使用
6. 注意，若不需要执行指令或发消息，写为
```
  "mc": ""
  "cmd": ""
```

>>OP鉴权
op鉴定权默认为支持所有管理员执行op命令，但是可以通过更改op.json来更改
op权限就是谁能执行上文的sudo指令
这是默认值(所有管理员都允许执行op)
```
{
  "OP": 0
}
```

想要令特定成员成员拥有权限，而其他人没有，可以这样写
```
{
  "OP": 1,
  "778599906": 1
}
```
这个配置文件给与了778599906这个群成员op权限
>>多服务器支持
多服务器支持配置步骤如下
1. 按照基础配置方式，配置两台服务器
2. CQ的配置文件中，增加如下
```
  - http: # HTTP 通信设置
      post:           # 反向HTTP POST地址列表
      - url: ''                # 地址
        secret: ''             # 密钥
        max-retries: 3         # 最大重试，0 时禁用
        retries-interval: 1500 # 重试时间，单位毫秒，0 时立即
      - url: http://127.0.0.1:5703/ # 地址
        secret: ''                  # 密钥
        max-retries: 0             # 最大重试，0 时禁用
        retries-interval: 0      # 重试时间，单位毫秒，0 时立即
```
其中，5703是你的端口号，可以自行更改，不能是5702，但要与下文的端口一致
3. 打开第二个服务器的/BDS/plugins/LL_Robot/RobotInfo.json文件
4. 将"5701"改为"5703"这里端口可以自己更改，与上文一致即可
5. 将serverName改为第二个服务器的名字
6. 保存，启动CQ和两个服务器

>使用第三方软件列表

* [[Mrs4s/go-cqhttp]](https://github.com/Mrs4s/go-cqhttp)
* [[jbeder/yaml-cpp]](https://github.com/jbeder/yaml-cpp)
* [[yhirose/cpp-httplib]](https://github.com/yhirose/cpp-httplib)
* [[nlohmann/json]](https://github.com/nlohmann/json)

>鸣谢

* 感谢go-cqhttp,cpp-httplib,json三个项目的支持
* 感谢LL中大佬的指教
* 感谢Tenderbear服务器全体成员的测试
