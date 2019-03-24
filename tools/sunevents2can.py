#!/usr/bin/env python3
# coding: utf-8

from __future__ import absolute_import, print_function

import sys
import argparse
import socket
from datetime import datetime

import can
from can import Bus, BusState, Logger

import paho.mqtt.client as mqtt

topic = "sunevents"
event_map = {
  'sunrise': "up",  
  'night': "down",  
}
coil_map = { 
  'ROL_EZ_DOWN': [0x03,0x05],
  'ROL_EZ_UP': [0x03,0x04]
}
group_map = {
  'up': ['ROL_EZ_UP'], 
  'down': ['ROL_EZ_DOWN'] 
}
cmd_map = {
  'off': 0,
  'on': 1,
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

    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    mcp_mqtt.subscribe(topic)
    print("Subscribed to:",topic)

def on_message(mcp_mqtt, userdata, msg):
    local_bus=userdata
    print("data Received topic: ", msg.topic)
    m_decode=str(msg.payload.decode("utf-8","ignore"))
    print("data Received",m_decode)
    data=m_decode
    event=event_map[data]
    if event is not None:
   #   event=sub
      for key, value in coil_map.items():
        msg_id=con(msg_dst=value[0],msg_cmd=0)
        msg_data=[value[1]] 
        print("turning off: ",key,value[0],value[1])
        m = can.Message(arbitration_id=msg_id,
            data=msg_data,
            extended_id=True)
        print("going to sent to CAN",m)
        try:
          local_bus.send(m)
        except BaseException as e:
          logging.error("Error sending can message {%s}: %s" % (m, e))
        print("data sent to CAN",m)

      print("Group: ",event) 
      for coil in group_map[event]:  
        print("turning on", coil)
        addr = coil_map[coil]
        msg_id=con(msg_dst=addr[0],msg_cmd=1)
        msg_data=[addr[1]] 
        print(msg_data)
        m = can.Message(arbitration_id=msg_id,
            data=msg_data,
            extended_id=True)
        print("going to sent to CAN",m)
    
        try:
            local_bus.send(m)
        except BaseException as e:
            logging.error("Error sending can message {%s}: %s" % (m, e))
        print("data sent to CAN",m)


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

    mcp_mqtt = mqtt.Client()
    mcp_mqtt.on_connect = on_connect
    mcp_mqtt.on_message = on_message
    mcp_mqtt.user_data_set(bus)
    mcp_mqtt.connect("mcp", 1883, 60)

    try:
        while True:
            mcp_mqtt.loop_start()
            msg = bus.recv(1)
            if msg is not None:
               de=decon(msg.arbitration_id)
               m= { "prio": hex(de[0]), "type": hex(de[1]), "dst": hex(de[2]), "src": hex(de[3]), "cmd": hex(de[4]), "action": hex(msg.data[0]) }
#               print(m)        
 #              mcp_mqtt.publish("can/rx", str(m))	
    except KeyboardInterrupt:
        pass
    finally:
        bus.shutdown()

if __name__ == "__main__":
    main()
