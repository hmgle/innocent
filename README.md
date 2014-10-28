# innocent

## 简介

查询成语的`Linux`内核模块。可用于开发**成语接龙**、**接二连三** 等程序。
`python` 使用示例见目录内 `innocent_demo.py`。

## 安装

```bash
make
sudo make install
```

若提示 `/lib/modules/xxx/build: 没有那个文件或目录. 停止.` 错误, 请先安装内核头文件目录树:

```bash
# for Debian/Ubuntu:
sudo apt-get install linux-headers-$(uname -r)
```

## 使用

使用示例:

```console
$ sudo insmod ./innocent.ko # make install 后可以 sudo modprobe innocent
$ echo "君" > /dev/innocent
$ cat /dev/innocent
君子之过
君子不器
君射臣决
君仁臣直
君唱臣和
君侧之恶
君暗臣蔽
君子之交
君子协定
君子三戒
君子好逑
君子固穷
君圣臣贤
君辱臣死
君命无二
君臣佐使
```

列出所有第二个字为 "钟" 的成语：

```console
$ ./innocent_demo.sh "2钟"
撞钟舞女
撞钟击鼓
撞钟伐鼓
撞钟吹螺
万钟之藏
情钟我辈
窃钟掩耳
龙钟潦倒
龙钟老态
黄钟瓦缶
洪钟大吕
晨钟暮鼓
朝钟暮鼓
盗钟掩耳
黄钟瓦釜
黄钟毁弃
黄钟大吕
黄钟长弃
毁钟为铎
击钟鼎食
击钟陈鼎
鸣钟食鼎
鸣钟列鼎
现钟弗打
现钟不打
```

