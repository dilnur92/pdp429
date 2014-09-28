#!/bin/bash

# Make the directory to hold all the testing items
mkdir -p testfiles
cd testfiles

# Download all the testing files
files=( "full" "jan" "prime" "spover" )

for i in "${files[@]}"
do
    wget http://www.cs.utexas.edu/users/peterson/prog7/$i.log
    wget http://www.cs.utexas.edu/users/peterson/prog7/$i.obj
    wget http://www.cs.utexas.edu/users/peterson/prog7/$i.out
    wget http://www.cs.utexas.edu/users/peterson/prog7/$i.in
done

cd ..

mkdir -p testoutput
gcc  -o pdp429 pdp429.c

# Run the tests
for i in "${files[@]}"
do
    ./pdp429 -v ./testfiles/$i.obj < ./testfiles/$i.in > ./testoutput/$i.out 2> ./testoutput/$i.log

    diff ./testfiles/$i.out ./testoutput/$i.out > ./testoutput/$i.out.diff
    diff ./testfiles/$i.log ./testoutput/$i.log > ./testoutput/$i.log.diff

    if [[ -s ./testoutput/$i.out.diff ]]; then
        echo -e "\e[00;31m$i OUTPUT FAILED\e[00m"
    fi
    if [[ -s ./testoutput/$i.log.diff ]]; then
        echo -e "\e[00;31m$i LOG OUTPUT FAILED\e[00m"
    fi
done
