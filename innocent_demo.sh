#!/bin/sh

help()
{
	cat <<EOF

	USAGE: $0 [字]

	EXAMPLES:
	$0 "剑"
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
