sudo printf "Opening virtual serial port... Use ttyS10 and ttyS11 to establish connection\n Done.\nPress CTRL+C at any time to close it.\n";
sudo socat -d  -d  PTY,link=/dev/ttyS10,mode=777   PTY,link=/dev/ttyS11,mode=777
printf "\nClosing virtual serial port... Done.\n"
