#! /usr/bin/env python
import sys

msg_id=int(str(sys.argv[1]),0) & 0x1FFFFFFF
msg_prio=msg_id>>26
msg_type=(msg_id>>24) & 0x03
msg_dst=(msg_id>>16) & 0xFF
msg_src=(msg_id>>8) & 0xFF
msg_cmd=msg_id & 0xFF

#print('id: {0:b}'.format(msg_id))
print('id: {0:#010x} {0:#031b}'.format(msg_id ,msg_id))
#print("id:", hex(msg_id), format(40,str(bin(msg_id))))

print('prio: {0:#03x}       {0:#04b}'.format(msg_prio))
print('type: {0:#03x}       {0:#04b}'.format(msg_type))
print('dst: {0:#04x} {0:#010b}'.format(msg_dst))
print('src: {0:#04x} {0:#010b}'.format(msg_src))
print('cmd: {0:#04x} {0:#010b}'.format(msg_cmd))


nmsg_prio=msg_id>>26
nmsg_type=(msg_id>>24) & 0x03
nmsg_src=(msg_id>>18) & 0x3F
nmsg_dst=(msg_id>>12) & 0x3F
nmsg_targ=(msg_id>>6) & 0x3F
nmsg_cmd=msg_id & 0x1F


print('New schema')

#print('id: {0:b}'.format(msg_id))
print('id: {0:#010x} {0:#031b}'.format(msg_id))
#print("id:", hex(msg_id), format(40,str(bin(msg_id))))

print('prio:   {0:#04x} {0:#04b}'.format(nmsg_prio))
print('type:   {0:#04x} {0:#04b}'.format(nmsg_type))
print('src:    {0:#04x} {0:#08b}'.format(nmsg_src))
print('dst:    {0:#04x} {0:#08b}'.format(nmsg_dst))
print('target: {0:#04x} {0:#08b}'.format(nmsg_targ))
print('cmd:    {0:#04x} {0:#07b}'.format(nmsg_cmd))
