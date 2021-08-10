make clean
make
./ospf -i 0 -f input.txt -o output1.txt -h 1 -a 5 -s 20 &
./ospf -i 1 -f input.txt -o output2.txt -h 1 -a 5 -s 20 &
./ospf -i 2 -f input.txt -o output3.txt -h 1 -a 5 -s 20 &
./ospf -i 3 -f input.txt -o output4.txt -h 1 -a 5 -s 20 &
./ospf -i 4 -f input.txt -o output5.txt -h 1 -a 5 -s 20 &
./ospf -i 5 -f input.txt -o output6.txt -h 1 -a 5 -s 20 &
./ospf -i 6 -f input.txt -o output7.txt -h 1 -a 5 -s 20 &
./ospf -i 7 -f input.txt -o output8.txt -h 1 -a 5 -s 20  
