# innocent

## 简介

查询成语的`Linux`内核模块。可用于开发**成语接龙**、**接二连三** 等程序。
`python` 使用示例见目录内 `innocent_demo.py`。

## 安装

	make
	sudo make install

## 使用

使用示例:

	$ sudo insmod ./innocent.ko # 或 sudo modprobe innocent
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

列出所有第二个字为 "中" 的成语：

	./innocent_demo.sh "2中"

