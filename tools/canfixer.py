#!/usr/bin/env python3
# coding: utf-8


from __future__ import absolute_import, print_function

import sys
import argparse
import socket
from datetime import datetime

import can
from can import Bus, BusState, Logger


def decon(msg_id):
   msg_prio=msg_id>>26
   msg_type=(msg_id>>24) & 0x03
   msg_dst=(msg_id>>16) & 0xFF
   msg_src=(msg_id>>8) & 0xFF
   msg_cmd=msg_id & 0xFF
   print("prio: ", hex(msg_prio),"type: ", hex(msg_type)," dst: ", hex(msg_dst)," src: ", hex(msg_src)," cmd: ",hex(msg_cmd))
   return [msg_prio, msg_type, msg_dst, msg_src, msg_cmd]


def main():
    verbosity = 2

    logging_level_name = ['critical', 'error', 'warning', 'info', 'debug', 'subdebug'][min(5, verbosity)]
    can.set_logging_level(logging_level_name)

    can_filters = []
    config = {"can_filters": can_filters, "single_handle": True}
    config["interface"] = "socketcan"
    config["bitrate"] = 125000
    bus = Bus("can1", **config)

    print('Connected to {}: {}'.format(bus.__class__.__name__, bus.channel_info))
    print('Can Logger (Started on {})\n'.format(datetime.now()))

    try:
        while True:
            msg = bus.recv(1)
            if msg is not None:
               de=decon(msg.arbitration_id)
               m= { "prio": hex(de[0]), "type": hex(de[1]), "dst": hex(de[2]), "src": hex(de[3]), "cmd": hex(de[4]), "target": hex(msg.data[0]) }
               if de[2] is 3 and msg.data[0] is 3 and de[1] is 0 and de[3] is 3 and de[4] is 3 :
                 print(m) 
                 m = can.Message(arbitration_id=0x0C030900,
                     data=[3],
                     extended_id=True)
                 try:
                   bus.send(m)
                 except BaseException as e:
                   logging.error("Error sending can message {%s}: %s" % (m, e))

    except KeyboardInterrupt:
        pass
    finally:
        bus.shutdown()

if __name__ == "__main__":
    main()
