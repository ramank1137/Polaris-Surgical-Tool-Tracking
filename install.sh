#!/bin/bash
echo "Installing GStreamer.."
apt-get install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libgstreamer-plugins-bad1.0-dev gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav gstreamer1.0-tools gstreamer1.0-x gstreamer1.0-alsa gstreamer1.0-gl gstreamer1.0-gtk3 gstreamer1.0-qt5 gstreamer1.0-pulseaudio

# Check if the installation was successful
if gst-inspect-1.0 --version > /dev/null 2>&1; then
    # If the command was successful, display a green tick
    echo -e "\n\e[32m\u2714\e[0m GStreamer installed successfully"
else
    # If the command failed, display a red cross
    echo -e "\e[31m\u2718\e[0m GStreamer is not installed or an error occurred."
fi

echo "Installing GTk.."

sudo apt-get install libgtk-3-dev

#check if installation is successful
if pkg-config --modversion gtk+-3.0 > /dev/null 2>&1; then
    # If the command was successful, display a green tick
    echo -e "\n\e[32m\u2714\e[0m gtk-3.0 installed successfully"
    echo ""
    echo "Run the setup.sh script with root privileges to setup the library"
    echo "sudo bash setup.sh"
    echo ""
else
    # If the command failed, display a red cross
    echo -e "\e[31m\u2718\e[0m gtk-3.0 is not installed or an error occurred."
fi
