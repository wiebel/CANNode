#!/usr/bin/env python3 

from __future__ import absolute_import, print_function

import sys
import argparse
import socket
from datetime import datetime

import can
from can import Bus, BusState, Logger

import paho.mqtt.client as mqtt

sub_topic = "coil"
coil_map = { 
  'EZ': [ 0x03, 0x01 ],
  'K': [ 0x03, 0x02 ],
  'WZ1': [ 0x02, 0x01 ],
  'WZ2': [ 0x02, 0x02 ],
  'TER': [ 0x03, 0x03 ],
  'FE': [ 0x03, 0x07 ],
  'TRE': [ 0x02, 0x03 ],
  'FO': [ 0x01, 0x03 ],
  'TRO': [ 0x01, 0x07 ],
  'KI1': [ 0x01, 0x06 ],
  'KI2': [ 0x01, 0x04 ],
  'SCH': [ 0x01, 0x08 ],
  'B1': [ 0x01, 0x01 ],
  'B2': [ 0x01, 0x02 ],
  'ROL_EZ_G_DOWN': [0x03, 0x05, -1],
  'ROL_EZ_G_UP': [0x03,0x04, -1],
  'ROL_EZ_N_DOWN': [0x03, 0x06, -2],
  'ROL_EZ_N_UP': [0x03,0x08, -2],
  'EG1': [ 0x02, 0x09 ],
  'EG2': [ 0x03, 0x09 ],
  'OG': [ 0x01, 0x09 ],
  'ALL': [ 0xFF, 0x09 ],
}

cmd_map = {
  'off': 0,
  '0': 0,
  'on': 1,
  '1': 1,
  'value': 2,
  'toggle': 3
}
  

def decon(msg_id):
   msg_prio=msg_id>>26
   msg_type=(msg_id>>24) & 0x03
   msg_dst=(msg_id>>16) & 0xFF
   msg_src=(msg_id>>8) & 0xFF
   msg_cmd=msg_id & 0xFF
#   print("msg_id: ", hex(msg_id), "prio: ", hex(msg_prio),"type: ", hex(msg_type)," dst: ", hex(msg_dst)," src: ", hex(msg_src)," cmd: ",hex(msg_cmd))
   return [msg_prio, msg_type, msg_dst, msg_src, msg_cmd]

def con(msg_dst,msg_cmd,msg_prio=3,msg_type=0,msg_src=11, ):
   msg_id=(msg_prio<<26) + \
   ((msg_type&0x03)<<24) +\
   ((msg_dst&0xFF)<<16) +\
   ((msg_src&0xFF)<<8) +\
   (msg_cmd&0xFF)
   print("Constructed ID: "+hex(msg_id))
   return msg_id

def on_connect(mcp_mqtt, userdata, flags, rc):
    print("Connected with result code "+str(rc))
    mcp_mqtt.subscribe(sub_topic+"/+")


def main():
    can_filters = []
    config = {"can_filters": can_filters, "single_handle": True}
    config["interface"] = "socketcan_native"
    config["bitrate"] = 125000
    canbus = can.Bus("can1", **config)
   # canBuffer= canbus.BufferedReader()
    print('Connected to {}: {}'.format(canbus.__class__.__name__, canbus.channel_info))
    print('Can Logger (Started on {})\n'.format(datetime.now()))

    mcp_mqtt = mqtt.Client()
    mcp_mqtt.on_connect = on_connect
    mcp_mqtt.user_data_set(canbus)
    mcp_mqtt.connect("mcp", 1883, 60)

    try:
      while True:
        msg = canbus.recv(1)
#        msg = canBuffer.get_message()
        if msg is not None:
          de=decon(msg.arbitration_id)
          m= { "prio": hex(de[0]), "type": hex(de[1]), "dst": hex(de[2]), "src": hex(de[3]), "cmd": hex(de[4]), "action": hex(msg.data[0]) }
          if de[2]==0 and de[4] == 6:
            print("received state: ", hex(de[3]), hex(msg.data[0]), hex(msg.data[1]))   
            for key, address in coil_map.items():
              if address[0] == de[3] and address[1] == msg.data[0]+1:
                 print("coil/"+key+" changed to "+str(msg.data[1]))
                 mcp_mqtt.publish("coil/"+key+"/state", msg.data[1] , retain=1)	
    except KeyboardInterrupt:
        pass
    finally:
        canbus.shutdown()

if __name__ == "__main__":
    main()
