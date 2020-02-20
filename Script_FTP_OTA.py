# Custom settings, as referred to as "extra_script" in platformio.ini
#
# See http://docs.platformio.org/en/latest/projectconf.html#extra-script
# See https://github.com/platformio/platformio/issues/426

import os

from SCons.Script import DefaultEnvironment

from base64 import b64decode

env = DefaultEnvironment()

# Must be full path, or use
# https://docs.python.org/2/library/os.path.html#os.path.expanduser

env.Replace(
    LOCAL_UPLOADER=os.getcwd() + "/Script_FTP_UPLOAD.py",
    UPLOADCMD = '$LOCAL_UPLOADER '
         + ' --bin_file $SOURCE '
         + ' --bin_version ' + b64decode(ARGUMENTS.get("BIN_VERSION"))
         + ' --remote_http ' + b64decode(ARGUMENTS.get("REMOTE_HTTP"))
         + ' --remote_addr ' + b64decode(ARGUMENTS.get("REMOTE_ADDR"))
         + ' --remote_port ' + b64decode(ARGUMENTS.get("REMOTE_PORT"))
         + ' --dest_file ' + b64decode(ARGUMENTS.get("DEST_FILE"))
         + ' --proto_mac_address ' + b64decode(ARGUMENTS.get("PROTO_MAC_ADDRESS"))
         # + ' --mqtt_cmd_update ' + b64decode(ARGUMENTS.get("MQTT_CMD_UPDATE"))
         # + ' --mqtt_broker_address ' + b64decode(ARGUMENTS.get("MQTT_BROKER_ADDRESS"))
         # + ' --mqtt_broker_port ' + b64decode(ARGUMENTS.get("MQTT_BROKER_PORT"))
         # + ' --mqtt_broker_user ' + b64decode(ARGUMENTS.get("MQTT_BROKER_USER"))
         # + ' --mqtt_broker_password ' + b64decode(ARGUMENTS.get("MQTT_BROKER_PASSWORD"))
)
