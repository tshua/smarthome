#!bin/sh
echo "qt start begin-----------------------------------"
cd ./smart_home_qt_phone
./smart_home &

echo "qt start end----------------------------------------"
echo "console start begin----------------------------------------"
sleep 10s

cd ..
./c 
echo "console start end----------------------------------------"
