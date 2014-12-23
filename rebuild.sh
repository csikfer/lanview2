#!/bin/bash

APP=$1

cd $APP
qmake ${APP}.pro
make clean
make
cd ..

