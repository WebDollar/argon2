#!/bin/bash

#### COLOR SETTINGS ####
BLACK=$(tput setaf 0 && tput bold)
RED=$(tput setaf 1 && tput bold)
GREEN=$(tput setaf 2 && tput bold)
YELLOW=$(tput setaf 3 && tput bold)
BLUE=$(tput setaf 4 && tput bold)
MAGENTA=$(tput setaf 5 && tput bold)
CYAN=$(tput setaf 6 && tput bold)
WHITE=$(tput setaf 7 && tput bold)
BLACKbg=$(tput setab 0 && tput bold)
REDbg=$(tput setab 1 && tput bold)
GREENbg=$(tput setab 2 && tput bold)
YELLOWbg=$(tput setab 3 && tput bold)
BLUEbg=$(tput setab 4 && tput dim)
MAGENTAbg=$(tput setab 5 && tput bold)
CYANbg=$(tput setab 6 && tput bold)
WHITEbg=$(tput setab 7 && tput bold)
STAND=$(tput sgr0)

### System dialog VARS
showinfo="$GREEN[info]$STAND"
showerror="$RED[error]$STAND"
showexecute="$YELLOW[running]$STAND"
showok="$MAGENTA[OK]$STAND"
showdone="$BLUE[DONE]$STAND"
showinput="$CYAN[input]$STAND"
showwarning="$RED[warning]$STAND"
showremove="$GREEN[removing]$STAND"
shownone="$MAGENTA[none]$STAND"
redhashtag="$REDbg$WHITE#$STAND"
abortte="$CYAN[abort to Exit]$STAND"
showport="$YELLOW[PORT]$STAND"
##

### GENERAL_VARS
get_libtool=$(if [[ $(cat /etc/*release | grep -o -m 1 Ubuntu) ]]; then echo "$(apt-cache policy libtool | grep Installed | grep none | awk '{print$2}')"; else if [[ $(cat /etc/*release | grep -o -m 1 Debian) ]]; then echo "$(apt-cache policy libtool | grep Installed | grep none | awk '{print$2}')"; else if [[ $(cat /etc/*release | grep -o -m 1 centos) ]]; then echo "$(yum list libtool | grep -o Installed)"; fi fi fi)

###

#### Dependencies START
function deps(){
if [[ "$get_libtool" == "(none)" ]]; then
	echo "$showinfo We need to install libtool"
	if [[ $(cat /etc/*release | grep -o -m 1 Ubuntu) ]]; then sudo apt install -y libtool; else if [[ $(cat /etc/*release | grep -o -m 1 Debian) ]]; then sudo apt-get install -y libtool; else if [[ $(cat /etc/*release | grep -o -m 1 centos) ]]; then sudo yum install -y libtool;  fi fi fi
else
	if [[ "$get_libtool" == * ]]; then
		echo "$showok libtool is already installed!"
	fi
fi
}
#### Dependencies check END

deps # call deps function

if [[ $(pwd) =~ argon[[:alnum:]]+ ]]; then
        echo "$showinfo Current dir is $(pwd)"
        echo "$showexecute ${GREEN}autoreconf -i$STAND" && autoreconf -i
	echo "$showexecute ${GREEN}./configure$STAND" && ./configure
	echo "$showexecute ${GREEN}make$STAND" && make
	echo "$showexecute ${GREEN}make check$STAND" && make check # check if reponse PASSES
else
        if [[ ! $(pwd) =~ argon[[:alnum:]]+ ]]; then
                echo "$showerror You are not inside the ${YELLOW}argon2$STAND folder."
                echo "$showinfo Run this script inside argon2 folder."
        fi
fi
