## Wifi scanner/sniffer based on ESP8266 NodeMCU.
#### Button "flash" switches the microcontroller from scan mode to mode web-view.



###  Compilation and flashing of the firmware:

#### Create WSL distro:
    wsl --set-default-version 1
    wsl --install -d Ubuntu-22.04
    wsl --set-version Ubuntu-22.04 1

#### Update subsystem:
    sudo apt update
    sudo apt-get install -y make gcc g++ gperf install-info gawk flex bison libexpat-dev sed git unzip bash wget bzip2 libtool-bin libncurses5 libncurses5-dev
    sudo apt-get install -y python2-dev python2 
    sudo update-alternatives --install /usr/bin/python python /usr/bin/python2 1

#### Configure SDK:
    curl https://bootstrap.pypa.io/pip/2.7/get-pip.py --output get-pip.py
    sudo python2 get-pip.py
    pip install pyserial
    
    sudo usermod -aG dialout <username>
    sudo chmod -R 777 /dev/<ttySx> 
        <ttySx> is ttyS1, ttyS2, ttyS3.... as COM1, COM2, COM3 serial port
    
    wget https://dl.espressif.com/dl/xtensa-lx106-elf-gcc8_4_0-esp-2020r3-linux-amd64.tar.gz
    mkdir -p ~/.espressif
    cd ~/.espressif
    tar -xzf ~/xtensa-lx106-elf-gcc8_4_0-esp-2020r3-linux-amd64.tar.gz
    git clone --recursive https://github.com/espressif/ESP8266_RTOS_SDK.git
    
    export PATH="$PATH:$HOME/.espressif/xtensa-lx106-elf/bin"
    export IDF_PATH=~/.espressif/ESP8266_RTOS_SDK 
    python2 -m pip install --user -r $IDF_PATH/requirements.txt

#### Build:
    git clone https://github.com/RomanSereda/wifi-ether-watcher.git
    cd ~/.espressif/wifi-ether-watcher
    make menuconfig
         WIFI SCAN Configuration  ---> set "WiFi SSID", set "WiFi Password" (your router)
    make flash
