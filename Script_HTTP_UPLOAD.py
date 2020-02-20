#!/usr/bin/python

# NO TE "Permission denied"
# chmod +x Script_HTTP_UPLOAD.py

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

def upload(options, command = FLASH):
  url = '%s://%s:%s/update' % (options.remote_http, options.remote_addr, options.remote_port)
  # logging.debug("url: %s", str(url))
  # logging.debug("command: %s", str(command))
  c = pycurl.Curl()
  c.setopt(c.URL, url)
  destFilename = options.dest_file
  if (command == SPIFFS):
    destFilename = options.dest_file.replace(".bin", "_spiffs.bin")
  destFilename = destFilename.replace(".bin", "-" + options.bin_version + ".bin")
  c.setopt(c.HTTPPOST, [('file', (c.FORM_FILE, options.bin_file, )), ('dest', destFilename), ])
  c.setopt(pycurl.CONNECTTIMEOUT, 3)
  c.setopt(pycurl.TIMEOUT, int(options.timeout))
  c.setopt(pycurl.NOSIGNAL, 1)
  # c.setopt(pycurl.VERBOSE, True)
  logging.debug('Uploading file %s...', options.bin_file)
  try:
    c.perform()
  except: # catch *all* exceptions
    e = sys.exc_info()[0]
    logging.critical('Exception : %s' % e )
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
    default = "192.168.1.42"
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
  group.add_option("--timeout",
    dest = "timeout",
    help = "TimeOut",
    default = 120
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

  # BUG Le programme plante et n'arrive jamais jusqu'ici!
  logging.debug('Trying to exit...')

  sys.exit(0)
# end main

if __name__ == '__main__':
  sys.exit(main(sys.argv))
# end if
