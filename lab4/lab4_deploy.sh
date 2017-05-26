#!/bin/bash
if [[ $(hostname) == "brooklyn" ]]; then
    rm -r lab4_temp.tgz
    TEMP_DIR="$(mktemp -d)"
    echo "Temp directory: $TEMP_DIR"
    cp -r * $TEMP_DIR 
    cd $TEMP_DIR
    rm -rf cmake-build-debug 
    tar -cvzf lab4_temp.tgz * 
    scp lab4_temp.tgz root@$1:~/lab4/lab4_temp.tgz  
    scp lab4_deploy.sh root@$1:~/lab4/lab4_deploy.sh
    ssh root@$1 "~/lab4/lab4_deploy.sh"
else #my edison
    if [[ $(hostname) != "harlem" ]];  then
        echo "This isn't \"harlem\", my Edison. Hello, TA! (why are you running this)"
        echo "This isn't going to be very useful to you"
    fi
    cd lab4 
    tar -xvzf lab4_temp.tgz 
    make 
fi 
