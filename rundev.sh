#!bin/sh
echo "qt start begin-----------------------------------"
#cd smart_home_qt_dev
#./smart_home -qws&
cd smart_home_qt_dev
./smart_home &
#cd ../smart_home_qt_phone
#./smart_home -qws&

echo "qt start end----------------------------------------"
echo "console start begin----------------------------------------"
sleep 10s
cd ..
./lamp1 &
sleep 5s
./lamp2 &
sleep 5s
./fan &
sleep 5s
./switch 
echo "console start end----------------------------------------"
