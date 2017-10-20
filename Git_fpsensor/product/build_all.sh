#!/bin/bash


cd `dirname $0`

export MODE=Debug

./build_ta.sh "$@"

if (( $? != 0 ))
then
    echo =====================Build Debug TA Fail!=====================
    exit 1
fi

./build_ca.sh "$@"
if (( $? != 0 ))
then
    echo =====================Build Debug CA Fail!=====================
    exit 1
fi

export MODE=Release

./build_ta.sh "$@"

if (( $? != 0 ))
then
    echo =====================Build Release TA Fail!=====================
    exit 1
fi

./build_ca.sh "$@"
if (( $? != 0 ))
then
    echo =====================Build Release CA Fail!=====================
    exit 1
fi
