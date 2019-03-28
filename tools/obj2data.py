#! /usr/bin/env python
import sys
import json
#stuff = json.loads(sys.argv[1])


#data = json.load(sys.stdin)


#print(json.dumps(sys.stdin, sort_keys=True, indent=4))
###print str(sys.argv[1])
#print(stuff['foo'])

#print(json.dumps(data, indent=4))

#msg_id=int(str(sys.argv[1]),0)
#msg_prio=msg_id>>26
#msg_type=(msg_id>>24) & 0x03
#msg_dst=(msg_id>>16) & 0xFF
#msg_src=(msg_id>>8) & 0xFF
#msg_cmd=msg_id & 0xFF

#print "id:", hex(msg_id), bin(msg_id)
#print "prio:", hex(msg_prio), bin(msg_prio)
#print "type:", hex(msg_type), bin(msg_type)
#print "dst:", hex(msg_dst), bin(msg_dst)
#print "src:", hex(msg_src), bin(msg_src)
#print "cmd:", hex(msg_cmd), bin(msg_cmd)
import sys
import simplejson as json


def main(args):
    try:
        inputFile = open(args[1])
        input = json.load(inputFile)
        inputFile.close()
    except IndexError:
        usage()
        return False
    if len(args) < 3:
        print json.dumps(input, sort_keys = False, indent = 10)
    else:
        outputFile = open(args[2], "w")
        json.dump(input, outputFile, sort_keys = False, indent = 4)
        outputFile.close()
    return True


def usage():
    print __doc__


if __name__ == "__main__":
    sys.exit(not main(sys.argv))
