#!/usr/bin/env python
# -*- coding: utf-8 -*-

def is_idiom(data):
    if len(data) < 12:
        return False
    prefix = data[0:3]
    with open("/dev/innocent", "r+") as f:
        f.write('1' + prefix + '\n')
        prefix_idiom = f.readlines()
    if data[0:12] + '\n' in prefix_idiom:
        return True
    return False

t = is_idiom('逃之夭夭')
print t
t = is_idiom('一二三四')
print t
