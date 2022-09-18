#!/bin/bash

#locate macdeployqt:
MPATH=`whereis macdeployqt`
echo $MPATH

read P1 P2 <<<$(IFS=":"; echo $MPATH)

if [[ -z $P2 ]]; then
	echo "macdeployqt not found"
	exit -1
fi
echo $P2
APP_PATH="./build_qt5/pe-bear/"
# clean the previous build
rm -rf $APP_PATH/PE-bear.app

#build stuff
./build_qt5.sh

#strip the created build
strip $APP_PATH/PE-bear.app/Contents/MacOS/PE-bear

#wrap by macdeployqt:
$P2 ./build_qt5/pe-bear/PE-bear.app

# if wrapping succeeded, zip and store:
if [[ $? == 0 ]]; then
	CURR_DIR=`pwd`
	ZIP_OUT=$CURR_DIR/PE-bear.app.zip
	
	cd $APP_PATH
	zip $ZIP_OUT ./PE-bear.app -r
	echo "Wrapped: "$ZIP_OUT
	cd $CURR_DIR
fi

