#!/bin/sh

help()
{
	cat <<EOF

USAGE: $0 [1-4][字]

EXAMPLES:
	$0 "剑"
	$0 "2文" # 列出所有第二个字为 "文" 的成语
	$0 "4 学" # 列出所有第四个字为 "学" 的成语, 数字后可带空格
EOF
}

if [ "$1" = '-h' ]
then
	help
	exit 0
fi

if [ ! -c /dev/innocent ]; then
	insmod ./innocent.ko
fi

if [ ! -z "$1" ]; then
	echo "$1" > /dev/innocent
fi
cat /dev/innocent
