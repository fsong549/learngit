del /s /f /q out
call ndk-build -B NDK_PROJECT_PATH=./ ^
    NDK_APPLICATION_MK=Application.mk ^
    APP_BUILD_SCRIPT=Android.mk ^
    NDK_OUT=./out/obj  ^
    NDK_LIBS_OUT=./out/libs
