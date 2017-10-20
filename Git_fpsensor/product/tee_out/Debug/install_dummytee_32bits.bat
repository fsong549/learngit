adb wait-for-device
adb root
adb wait-for-device
adb remount

adb push ./libfpsensor_module.default.so /system/lib/hw/fpsensor_module.default.so
adb push ./libfpsensor_fingerprint.default.so /system/lib/hw/fpsensor_fingerprint.default.so

adb reboot
pause
