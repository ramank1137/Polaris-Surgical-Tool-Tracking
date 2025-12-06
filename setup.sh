#!/bin/bash
file_path=ndiTrack.py

if [ ! -f "$file_path" ]; then
    echo "Error: File not found."
    exit 1
fi

sed -i 's|"ARDEMO_PATH"|"'$(pwd)/CombinedAPISample'"|' "$file_path"

# # Compiling the code
# cd CombinedAPISample
# make
# cd ..

python3 setup.py install

sed -i 's|"'$(pwd)/CombinedAPISample'"|"ARDEMO_PATH"|' "$file_path"
