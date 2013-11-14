innocent
========

成语驱动。
使用示例:

	$ insmod ./innocent.ko
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

对于较高版本的 Linux 内核， `hlist_for_each_entry()`接口已改动，需修改才能编译成功。
