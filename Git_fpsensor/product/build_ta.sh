#!/bin/bash

rm -r ta/Out/
./ta/Locals/Build/build.sh "$@"
if (( $? != 0 ))
then
    echo =====================Build Ta Fail!=====================
    exit 1
fi

source ./setup.sh
mkdir -p ./tee_out/${MODE}
if [ $TEE == qsee ]; then
    cp -rvaf ./ta/Out/Bin/${MODE}/qsee* ./tee_out/${MODE}/
    exit 0
fi
if [ $TEE == spreadtrum ]; then
    cp -vf ./ta/Out/Bin/${MODE}/fp_ta.elf ./tee_out/${MODE}/
    cp -vf ./ta/Out/Bin/${MODE}/fp_ta.syms.elf ./tee_out/${MODE}/
    cp -vf ./ta/Out/Bin/${MODE}/tos.bin ./tee_out/${MODE}/
    exit 0
fi

if [ $TEE == dummytee ]; then
    cp ./ta/Out/Bin/${MODE}/${APP_ABI}/libfpsensor_module.default.so ./tee_out/${MODE}
else
    cp ./ta/Out/Bin/${MODE}/${TA_NAME} ./tee_out/${MODE}
fi
