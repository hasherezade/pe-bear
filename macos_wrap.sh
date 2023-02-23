#!/bin/bash

MCDEPLOY=$1
echo $MCDEPLOY #the given path to macdeploy
if [[ -z $MCDEPLOY ]]; then
	#locate macdeployqt:
	MPATH=`whereis macdeployqt`
	echo $MPATH

	read P1 MCDEPLOY <<<$(IFS=":"; echo $MPATH)

	if [[ -z $MCDEPLOY ]]; then
		echo "macdeployqt not found"
		exit -1
	fi	
fi

APP_PATH="./build_qt5/pe-bear/"
# clean the previous build
rm -rf $APP_PATH/PE-bear.app

#build stuff
./build_qt5.sh

#strip the created build
strip $APP_PATH/PE-bear.app/Contents/MacOS/PE-bear

#wrap by macdeployqt:
$MCDEPLOY $APP_PATH/PE-bear.app

# if wrapping succeeded, zip and store:
if [[ $? == 0 ]]; then
	CURR_DIR=`pwd`
	ZIP_OUT=$CURR_DIR/PE-bear.app.zip
	
	cd $APP_PATH
	zip $ZIP_OUT ./PE-bear.app -r
	echo "Wrapped: "$ZIP_OUT
	cd $CURR_DIR
fi
