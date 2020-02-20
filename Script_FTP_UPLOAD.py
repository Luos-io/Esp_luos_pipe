#!/usr/bin/python

# NO TE "Permission denied"
# chmod +x Script_FTP_UPLOAD.py

#~/.platformio/penv/bin/pip install paho-mqtt
import paho.mqtt.client as mqttClient
import time

import logging
import sys
import os
import optparse
import logging
import pycurl

FLASH = 0
SPIFFS = 100

root = logging.getLogger()
root.setLevel(logging.DEBUG)

ch = logging.StreamHandler(sys.stdout)
ch.setLevel(logging.DEBUG)
# formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
formatter = logging.Formatter('%(levelname)s - %(message)s')
ch.setFormatter(formatter)
root.addHandler(ch)

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        logging.debug("Connected to broker")
        global Connected                #Use global variable
        Connected = True                #Signal connection
    else:
        logging.error("Connection failed")

Connected = False   #global variable for the state of the connection

def mqtt_publish(broker_address, broker_port, broker_user, broker_password, channel, message):
    client = mqttClient.Client("Platformio")               #create new instance
    client.username_pw_set(broker_user, password=broker_password)    #set username and password
    client.on_connect= on_connect                      #attach function to callback
    client.connect(broker_address, port=broker_port)          #connect to broker

    client.loop_start()        #start the loop

    while Connected != True:    #Wait for connection
        time.sleep(0.1)

        client.publish(channel, message)
        client.disconnect()
        client.loop_stop()

def update_mqtt(options, command = FLASH):
  #LEDDisplays/B4-E6-2D-17-F9-00/Update
  logging.debug("Command 'UPLOAD' via MQTT ? %s", str(options.mqtt_cmd_update))
  if (options.mqtt_cmd_update == '1') :
    logging.debug("Command 'UPLOAD' via MQTT!")

    destChannel = options.dest_file.replace(".bin", "/%s/Update" % (options.proto_mac_address) )
    if (command == SPIFFS):
      destChannel = destChannel.replace("Update", "UpdateSPIFFS")
    logging.debug("Command 'UPLOAD' channel : '%s'" % (destChannel))

    destFilename = options.dest_file
    if (command == SPIFFS):
      destFilename = options.dest_file.replace(".bin", "_spiffs.bin")
    destFilename = destFilename.replace(".bin", "-" + options.bin_version + ".bin")
    message = '%s://%s:%s/OTA/files/%s' % (options.remote_http, options.remote_addr, options.remote_port, destFilename)
    #http://mqtt.alternadom.com/OTA/files/LEDDisplays-0.51.bin
    logging.debug("Command 'UPLOAD' message : '%s'" % (message))
    mqtt_publish(options.mqtt_broker_address, options.mqtt_broker_port, options.mqtt_broker_user, options.mqtt_broker_password, destChannel, message)
  else:
      logging.info("No command 'UPLOAD' via MQTT!")
  # end if

def upload(options, command = FLASH):
  url = '%s://%s:%s/OTA/upload.php' % (options.remote_http, options.remote_addr, options.remote_port)
  # logging.debug("url: %s", str(url))
  # logging.debug("command: %s", str(command))
  c = pycurl.Curl()
  c.setopt(c.URL, url)
  destFilename = options.dest_file
  if (command == SPIFFS):
    destFilename = options.dest_file.replace(".bin", "_spiffs.bin")
  destFilename = destFilename.replace(".bin", "-" + options.bin_version + ".bin")
  c.setopt(c.HTTPPOST, [('file', (c.FORM_FILE, options.bin_file, )), ('dest', destFilename), ])
  c.perform()
  c.close()

def parser():
  parser = optparse.OptionParser(
    usage = "%prog [options]",
    description = "Upload image to over the air Host server for the IOT with OTA support."
  )

  group = {}

  # destination ip and port
  group = optparse.OptionGroup(parser, "HostServer")
  group.add_option("--remote_http",
    dest = "remote_http",
    help = "Type de connexion au serveur distant pour la gestion des updates",
    default = "http"
  )
  group.add_option("--remote_addr",
    dest = "remote_addr",
    help = "Adresse du serveur distant pour la gestion des updates",
    default = "mqtt.alternadom.com"
  )
  group.add_option("--remote_port",
    dest = "remote_port",
    help = "Port du serveur distant pour la gestion des updates",
    default = 80
  )
  group.add_option("--bin_file",
    dest = "bin_file",
    help = "BIN file",
    metavar="FILE",
    default = None
  )
  group.add_option("--dest_file",
    dest = "dest_file",
    help = "Destination file",
    default = None
  )
  group.add_option("--bin_version",
    dest = "bin_version",
    help = "Version file",
    default = "0.1"
  )
  parser.add_option_group(group)

  #MQTT

  group = optparse.OptionGroup(parser, "Broker")
  group.add_option("--mqtt_cmd_update",
    dest = "mqtt_cmd_update",
    help = "Command UPDATE via MQTT",
    default = 0
  )
  group.add_option("--proto_mac_address",
    dest = "proto_mac_address",
    help = "MAC Address of the destination device",
    default = "Global"
  )
  group.add_option("--mqtt_broker_address",
    dest = "mqtt_broker_address",
    help = "MQTT : Address for the broker",
    default = "mqtt.alternadom.com"
  )
  group.add_option("--mqtt_broker_port",
    dest = "mqtt_broker_port",
    help = "MQTT : Port for the broker",
    default = "1883"
  )
  group.add_option("--mqtt_broker_user",
    dest = "mqtt_broker_user",
    help = "MQTT : User for the broker",
    default = None
  )
  group.add_option("--mqtt_broker_password",
    dest = "mqtt_broker_password",
    help = "MQTT : Password for the broker",
    default = None
  )

  parser.add_option_group(group)

  # output group
  group = optparse.OptionGroup(parser, "Output")
  group.add_option("--debug",
    dest = "debug",
    help = "Show debug output. And override loglevel with debug.",
    action = "store_true",
    default = False
  )
  parser.add_option_group(group)

  (options, args) = parser.parse_args()

  return options
# end parser

def main(args):
  # get options
  options = parser()

  # adapt log level
  loglevel = logging.WARNING
  if (options.debug):
    loglevel = logging.DEBUG
  # end if

  # logging
  logging.basicConfig(level = loglevel, format = '%(asctime)-8s [%(levelname)s]: %(message)s', datefmt = '%H:%M:%S')

  # logging.debug("Options: %s", str(options))

  command = FLASH
  if (options.bin_file.endswith("spiffs.bin")):
    command = SPIFFS

  if (not options.remote_addr or not options.bin_file or not options.dest_file):
    logging.critical("Not enough arguments.")

    return 1
  # end if

  if not os.path.exists(options.bin_file):
    logging.critical('Sorry: the file %s does not exist', options.bin_file)

    return 1
  # end if

  upload(
    options,
    command
  )

  update_mqtt(
    options,
    command
  )
# end main

if __name__ == '__main__':
  sys.exit(main(sys.argv))
# end if
