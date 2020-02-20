# Custom settings, as referred to as "extra_script" in platformio.ini
#
# See http://docs.platformio.org/en/latest/projectconf.html#extra-script
# See https://github.com/platformio/platformio/issues/426

from SCons.Script import DefaultEnvironment

from base64 import b64decode

env = DefaultEnvironment()

# Must be full path, or use
# https://docs.python.org/2/library/os.path.html#os.path.expanduser

env.Replace(
    LOCAL_UPLOADER="esptool.py",
    LOCAL_UPLOADERFLAGS=[
        "--port", "$UPLOAD_PORT",
        "--baud", "$UPLOAD_SPEED",
        "write_flash", "0x00000",
    ],
    UPLOADCMD='$LOCAL_UPLOADER $LOCAL_UPLOADERFLAGS $SOURCE'
    # UPLOADCMD='$LOCAL_UPLOADER -f $SOURCE -o ' + b64decode(ARGUMENTS.get("OTA_FILE"))
)
