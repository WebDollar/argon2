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

if [[ $(pwd) =~ Node-WebDollar[[:alnum:]]+ || Node-WebDollar ]]; then

	if [[ $(find . -type d -name argon2 | grep -x './argon2') == ./argon2 ]]; then

		echo "$showinfo argon2 is already inside Node-WebDollar."
		read -e -p "$showinput Do you want to compile argon2 again? (y or n): " yn_compile

		if [[ $yn_compile == [nN] ]]; then
			echo -e "$showinfo OK..."

		elif [[ $yn_compile == [yY] ]]; then
			echo "$showexecute Changing dir to ${YELLOW}argon2$STAND" && cd argon2
			echo "$showexecute Compiling argon2..." && make
			echo "$showexecute Going back to Node-WebDollar folder..." && cd ..

			if [[ -d dist_bundle/CPU  ]]; then

				echo "$showok CPU folder inside dist_bundle exists!"
				echo "$showexecute Copying argon2/* files to dist_bundle/CPU" && cp -a argon2/* dist_bundle/CPU/
			else
				if [[ ! -d dist_bundle/CPU ]]; then
					echo "$showerror CPU folder inside dist_bundle not found!"
					echo "$showexecute Creating one now..." && mkdir dist_bundle/CPU
					echo "$showexecute Copying argon2/* files to dist_bundle/CPU" && cp -a argon2/* dist_bundle/CPU/
				fi
			fi

		elif [[ $yn_compile == * ]]; then
			echo -e "$showerror Possible options are: yY or nN."
		fi
	else
		if [[ ! $(find . -type d -name argon2 | grep -x './argon2') == ./argon2 ]]; then

			echo "$showerror argon2 not found inside Node-WebDollar!"
			echo "$showinfo Cloning argon2 from WebDollar repository..."
			git clone https://github.com/WebDollar/argon2.git
			cd argon2

			if [[ $(pwd) =~ argon[[:alnum:]]+ ]]; then
			        echo "$showinfo Current dir is $(pwd)"
			        echo "$showexecute ${GREEN}autoreconf -i$STAND" && autoreconf -i
				echo "$showexecute ${GREEN}./configure$STAND" && ./configure
				echo "$showexecute ${GREEN}make$STAND" && make
				echo "$showexecute ${GREEN}make check$STAND" && make check # check if reponse PASSES

				if [[ -d ../dist_bundle/CPU  ]]; then

					echo "$showok CPU folder inside dist_bundle exists!"
					echo "$showexecute Copying argon2/* files to dist_bundle/CPU" && cp -a * ../dist_bundle/CPU/
				else
					if [[ ! -d ../dist_bundle/CPU ]]; then
						echo "$showerror CPU folder inside dist_bundle not found!"
						echo "$showexecute Creating one now..." && mkdir ../dist_bundle/CPU
						echo "$showexecute Copying argon2/* files to dist_bundle/CPU" && cp -a * ../dist_bundle/CPU/
					fi
				fi
			else
			        if [[ ! $(pwd) =~ argon[[:alnum:]]+ ]]; then
			                echo "$showerror You are not inside the ${YELLOW}argon2$STAND folder."
			                echo "$showinfo Run this script inside argon2 folder."
			        fi
			fi
		fi
	fi
else
	if [[ $(pwd) =~ argon[[:alnum:]]+ ]]; then

		read -e -p "$showinput Do you want to compile argon2 again? (y or n): " yn_compile

		if [[ $yn_compile == [nN] ]]; then
			echo -e "$showinfo OK..."

		elif [[ $yn_compile == [yY] ]]; then
			echo "$showexecute Compiling argon2..." && make

		elif [[ $yn_compile == * ]]; then
			echo -e "$showerror Possible options are: yY or nN."
		fi
	else
	        if [[ ! $(pwd) =~ argon[[:alnum:]]+ ]]; then
	                echo "$showerror You are not inside the ${YELLOW}argon2$STAND folder."
	                echo "$showinfo Run this script inside argon2 folder."
	        fi

	fi
fi
