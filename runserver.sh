#!bin/sh
echo "qt start begin-----------------------------------"
#cd smart_home_qt_dev
#./smart_home -qws&
cd smart_home_qt_server
./smart_home &
#cd ../smart_home_qt_phone
#./smart_home -qws&

echo "qt start end----------------------------------------"
echo "console start begin----------------------------------------"
cd ..
./s
#./c &
#./lamp1 &
#./lamp2 &
#./fan &
#./switch &
echo "console start end----------------------------------------"
