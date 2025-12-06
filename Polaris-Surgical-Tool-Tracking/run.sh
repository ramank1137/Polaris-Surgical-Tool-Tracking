#!/bin/bash
draw=0
curr_dir=$0

# install_path=$ARDEMO_PATH

for arg in "$@"
do
    if [ "compile" == "$arg" ]
    then 
        echo "Compiling..."
        make
    fi
    if [[ "$arg" =~ "debug=" ]]
    then 
        debug=${arg#debug=}
        export GST_DEBUG=$debug:5
        export GST_DEBUG_FILE=debug.txt
        echo $debug
    fi

    if [[ "$arg" == "draw" ]]
    then 
        draw=1
        export GST_DEBUG_DUMP_DOT_DIR=$(pwd)
    fi

    if [ "run" == "$arg" ]
    then 
        install_path=$(dirname "$curr_dir")
        cd $install_path/build/linux;
        export LD_LIBRARY_PATH=:$(pwd);
        ./ardemo "$2" "$3" "$4" "$5" "$6" "$7"
        exit 0
    fi
done

if [ $draw -eq 1 ]
then
    dot -Tpng *.dot > pipepline_curr.png
    rm *.dot
fi
