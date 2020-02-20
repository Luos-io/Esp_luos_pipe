# NO TE "Permission denied"
# chmod +x Script_UPLOAD.py

from time import time
import datetime

Import("env")

now = datetime.datetime.now()

oldBuildFlags = env.get("BUILD_FLAGS")
# print oldBuildFlags
test = '"' + now.strftime("%Y-%m-%d %H:%M") + '"'
env.Append(
    BUILD_FLAGS= ['\'-D BUILD_TIMESTAMP=' + test + '\''])
# print env.Dump("BUILD_FLAGS");

#esptool.py --port /dev/cu.wchusbserial1420 read_mac | grep MAC: | awk '{print $2}'
