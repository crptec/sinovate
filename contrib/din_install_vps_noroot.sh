#!/bin/bash

clear

R='\033[0;31m'; G='\033[0;32m'; P='\033[0;35m'; Y='\033[0;33m'; B='\033[0;34m';  NC='\033[0m'

# read user
echo -e "${R}Root${Y} login will be disabled for security reasons.${NC}"
echo -e "Please enter username for the new D.I.N. user."
echo -ne "Username (default: ${G}sinovate${NC}): "
read NODEUSER
if [ -z "$NODEUSER" ]; then
  NODEUSER="sinovate"
fi
echo

## Change where files are located
CONFIGFOLDER="/home/$NODEUSER/.sin"
COIN_DAEMON="/home/$NODEUSER/sind"
COIN_CLI="/home/$NODEUSER/sin-cli"
##
CONFIG_FILE='sin.conf'
# need to change for new codebase
COIN_REPO='https://github.com/SINOVATEblockchain/sinovate/releases/latest/download/daemon.tar.gz'
#

# + bootstrap
BS_REMOTE="https://service.sinovate.io/mainnet/latest/bootstrap.zip"
BS_LOCAL="/home/$NODEUSER/bootstrap.zip"
BSLEN_REMOTE=$(wget --spider --no-check-certificate $BS_REMOTE 2>&1 | grep Length | cut -d " " -f2)
BSLEN_LOCAL=$(du -b $BS_LOCAL 2>/dev/null | cut -f1)
# - bootstrap

COIN_NAME='sinovate'
COIN_PORT=20970

function create_user() {
  if [ $(grep -c "^$NODEUSER:" /etc/passwd) == 0 ]; then
    echo -e "User ${G}$NODEUSER${NC} doesn't exist, creating new user"
    useradd -s /bin/bash $NODEUSER
    while true; do
      passwd $NODEUSER && break
    done
    mkhomedir_helper $NODEUSER
    adduser $NODEUSER sudo
    #    sed -i '/^PermitRootLogin[ \t]\+\w\+$/{ s//PermitRootLogin no/g; }' /etc/ssh/sshd_config
    sed -i '/^PermitRootLogin[ \t]\+\w\+$/{ s//PermitRootLogin prohibit-password/g; }' /etc/ssh/sshd_config
    systemctl reload ssh || systemctl reload sshd
  fi
  echo
}

function create_swap() {
  #if there are less than 2GB of memory and no swap add 4gb of swap
  if (( $(free -m | awk '/^Mem:/{print $2}')<2000 )); then
    if [[ ! $(swapon --show) ]]; then
      echo -e "${G}Memory less than 4GB - creating a swap file${NC}"
      fallocate -l 4G /swapfile
      dd if=/dev/zero of=/swapfile bs=1M count=4096
      chmod 600 /swapfile
      mkswap /swapfile
      swapon /swapfile
      #create a backup of fstab just in case
      cp /etc/fstab /etc/fstab.sinbackup
      echo '/swapfile none swap sw 0 0' | tee -a /etc/fstab
    fi
  fi
  echo
}

function download_node() {
  echo -e "${G}Preparing to download $COIN_NAME binary files${NC}"
  COIN_ZIP="daemon.tgz"
  wget -q -t 10 --show-progress --no-check-certificate $COIN_REPO -O $COIN_ZIP
  compile_error
  tar -xzf $COIN_ZIP -C /home/$NODEUSER >/dev/null 2>&1
  compile_error
  strip $COIN_DAEMON $COIN_CLI
  chown -R $NODEUSER:$NODEUSER /home/$NODEUSER
  echo -e "Done\n"
}

function configure_systemd() {
  cat << EOF > /etc/systemd/system/$COIN_NAME.service
[Unit]
Description=$COIN_NAME service
After=network.target
[Service]
User=$NODEUSER
Group=$NODEUSER

Type=forking
#PIDFile=$CONFIGFOLDER/$COIN_NAME.pid

ExecStart=$COIN_DAEMON -daemon -conf=$CONFIGFOLDER/$CONFIG_FILE -datadir=$CONFIGFOLDER
ExecStop=-$COIN_CLI -conf=$CONFIGFOLDER/$CONFIG_FILE -datadir=$CONFIGFOLDER stop

Restart=always
PrivateTmp=true
TimeoutStopSec=60s
TimeoutStartSec=10s
StartLimitInterval=120s
StartLimitBurst=5

[Install]
WantedBy=multi-user.target
EOF

  systemctl daemon-reload
  systemctl enable $COIN_NAME.service >/dev/null 2>&1
  systemctl start $COIN_NAME.service

  if [[ -z "$(ps axo cmd:100 | egrep $COIN_DAEMON)" ]]; then
    echo -e "${R}$COIN_NAME is not running${NC}, please investigate. You should start by running the following commands as root:"
    echo -e "${G}systemctl start $COIN_NAME.service"
    echo -e "systemctl status $COIN_NAME.service"
    echo -e "less /var/log/syslog${NC}"
    exit 1
  fi
}

function create_config() {
  mkdir -p $CONFIGFOLDER >/dev/null
  if [ "$?" -ne 0 ]; then
    echo -e "${R}Cannot create $CONFIGFOLDER, installation aborted\n${NC}"
    exit 1
  fi
  echo "SIN script v8 used" >> $CONFIGFOLDER/debug.log
  RPCUSER=$(tr -cd '[:alnum:]' < /dev/urandom | fold -w10 | head -n1)
  RPCPASSWORD=$(tr -cd '[:alnum:]' < /dev/urandom | fold -w22 | head -n1)
  cat << EOF > $CONFIGFOLDER/$CONFIG_FILE
debug=0
staking=0
listen=1
server=1
daemon=1
rpcuser=$RPCUSER
rpcpassword=$RPCPASSWORD
rpcallowip=127.0.0.1
rpcbind=127.0.0.1
port=$COIN_PORT
EOF
  chown -R $NODEUSER:$NODEUSER $CONFIGFOLDER
}

function create_key() {
  echo -e "${G}The node's private key is being created, please wait...${NC}"
  NODE_TMPDIR="/tmp/.sin"
  mkdir -p $NODE_TMPDIR &&\
  echo "
rpcuser=user
rpcpassword=resu
rpcbind=127.0.0.1
rpcport=20961
  " > $NODE_TMPDIR/$CONFIG_FILE

  [ ! "$(pgrep -f $NODE_TMPDIR)" ] &&\
  $COIN_DAEMON -noconnect -daemon -datadir=$NODE_TMPDIR -conf=$NODE_TMPDIR/$CONFIG_FILE >/dev/null 2>&1
  CLITMP="$COIN_CLI -datadir=$NODE_TMPDIR -conf=$NODE_TMPDIR/$CONFIG_FILE"
  while ! $CLITMP getblockcount >/dev/null 2>&1; do pgrep -f $NODE_TMPDIR >/dev/null || break; sleep 0.5; done

  KEYPAIR=$($CLITMP infinitynode keypair | awk '/Address|PrivateKey|"PublicKey/{print $1, $2}' | tr -d "\",:")
  COINKEY=$(echo "$KEYPAIR" | awk '/PrivateKey/{print $2}')
  PUBLICKEY=$(echo "$KEYPAIR" | awk '/PublicKey/{print $2}')
  ADDRESS=$(echo "$KEYPAIR" | awk '/Address/{print $2}')

  # need to uncomment for new codebase, because now wallet do not autocreate
  #  $CLITMP createwallet wallet >/dev/null 2>&1 &&\
  $CLITMP importprivkey $COINKEY >/dev/null 2>&1

  $CLITMP stop >/dev/null 2>&1
  while [ "$(pgrep -f $NODE_TMPDIR)" ]; do sleep 0.2; done

  mv -f $NODE_TMPDIR/wallet.dat $CONFIGFOLDER >/dev/null 2>&1

  # need to uncomment for new codebase, variant 1
  #  mv -f $NODE_TMPDIR/wallet/wallet.dat $CONFIGFOLDER >/dev/null 2>&1

  # need to uncomment for new codebase, variant 2
  #  mkdir -p $CONFIGFOLDER/wallets/main >/dev/null 2>&1
  #  mv -f $NODE_TMPDIR/wallet/wallet.dat $CONFIGFOLDER/wallets/main >/dev/null 2>&1
  #  echo "{ \"wallet\": [ \"main\" ] }" > $CONFIGFOLDER/settings.json

  echo "$KEYPAIR" > $CONFIGFOLDER/node.txt
  chown -R $NODEUSER:$NODEUSER $CONFIGFOLDER
  rm -r $NODE_TMPDIR >/dev/null 2>&1
  echo -e "Done\n"
}

function update_config() {
  cat << EOF >> $CONFIGFOLDER/$CONFIG_FILE
infinitynode=1
infinitynodeprivkey=$COINKEY
externalip=$NODEIP
EOF
}

function install_bootstrap() {
  echo -e "${G}Installing bootstrap${NC}"
  read -n 1 -p "Want to download bootstrap (y/n)? " CHOICE
  case "$CHOICE" in
    y|Y ) echo;;
    * )   echo -e "\n${Y}Skipping bootstrap install\n${NC}"
    return;;
  esac

  if [ ! "$BSLEN_REMOTE" ]; then
    echo -e "${R}File $BS_REMOTE not found.\nSkipping bootstrap download\n${NC}"
    return
  fi

  # download bootstarp
  if [ "$BSLEN_REMOTE" != "$BSLEN_LOCAL" ]; then
    echo "Start downloading $(basename $BS_LOCAL)..."
    wget -q -t 10 --show-progress --no-check-certificate -O $BS_LOCAL $BS_REMOTE
    if [ "$?" -ne 0 ]; then
      test -f $BS_LOCAL && rm $BS_LOCAL
      echo -e "${R}File download error! Skipping bootstrap install\n${NC}"
      return
    else
      echo "Done"
    fi
  fi
  chown $NODEUSER:$NODEUSER $BS_LOCAL

  # extract bootstrap
  echo -n "Extracting $(basename $BS_LOCAL) "
  mkdir -p $CONFIGFOLDER
  unzip -o -d $CONFIGFOLDER $BS_LOCAL | awk 'BEGIN {ORS="."} {if(NR%25==0) print "."}'; echo
  if [ "$?" -ne 0 ]; then
    echo -e "${R}Bootstrap extracting error!\n${NC}"
    return
  fi
  echo -e "Bootstrap extracting successful\n"
  chown -R $NODEUSER:$NODEUSER $CONFIGFOLDER
  # bypassing the mv error "Directory not empty"
  rm -R $CONFIGFOLDER/blocks $CONFIGFOLDER/chainstate $CONFIGFOLDER/indexes 2>/dev/null
  mv -f $CONFIGFOLDER/bootstrap/* $CONFIGFOLDER && rm -R $CONFIGFOLDER/bootstrap/
}

function enable_firewall() {
  echo -e "${G}Installing and setting up firewall to allow ingress on port $COIN_PORT${NC}"
  ufw allow $COIN_PORT/tcp comment "$COIN_NAME MN port" >/dev/null
  ufw allow ssh comment "SSH" >/dev/null 2>&1
  ufw limit ssh/tcp >/dev/null 2>&1
  ufw default allow outgoing >/dev/null 2>&1
  echo "y" | ufw enable >/dev/null 2>&1
  if [ "$?" -ne 0 ]; then
    echo -e "${R}Firewall installation failed!\n${NC}"
    return
  else
    echo -e "Done\n"
  fi
}

function get_ip() {
  NODEIP=$(curl -s ifconfig.me)
}

function compile_error() {
  if [ "$?" -gt "0" ]; then
    echo -e "${R}Failed to compile $COIN_NAME. Please investigate.${NC}"
    exit 1
  fi
}

function checks() {
  if ! test -r /etc/os-release; then
    echo -e "${R}Unsupported GNU/Linux distribution${NC}"
    exit 1
  fi
  source /etc/os-release
  case "$ID" in
    debian ) if [ "$VERSION_ID" -lt "10" ]; then
        echo -e "${R}Debian version must be 10 or higher${NC}"
        exit 1
    fi;;
    ubuntu ) if [ "${VERSION_ID%%.*}" -lt "18" ]; then
        echo -e "${R}Ubuntu version must be 18.04 or higher${NC}"
        exit 1
    fi;;
    * ) echo -e "${R}Unsupported GNU/Linux distribution${NC}"
    exit 1;;
  esac

  if [[ $EUID -ne 0 ]]; then
    echo -e "${R}$(basename $0) must be run as root.${NC}"
    exit 1
  fi

  if [ -n "$(pidof $COIN_DAEMON)" ] || [ -e "$COIN_DAEMON" ] ; then
    echo -e "${R}$COIN_NAME is already installed.${NC}"
    exit 1
  fi
}

function prepare_system() {
  echo -e "${G}Prepare the system to install $COIN_NAME D.I.N. node${NC}"
  echo -e "Updating the list of repository packages"
  DEBIAN_FRONTEND=noninteractive apt-get update > /dev/null 2>&1
  echo -e "Installing new versions of packages, it may take some time to finish"
  DEBIAN_FRONTEND=noninteractive apt-get -o Dpkg::Options::="--force-confdef" -o Dpkg::Options::="--force-confold" -y -qq upgrade >/dev/null 2>&1

  #apt-get install -y software-properties-common >/dev/null 2>&1
  #echo -e "Adding bitcoin PPA repository"
  #apt-add-repository -y ppa:bitcoin/bitcoin >/dev/null 2>&1
  #echo -e "Installing required packages, it may take some time to finish.${NC}"
  #apt-get update >/dev/null 2>&1
  #apt-get install -y -o Dpkg::Options::="--force-confdef" -o Dpkg::Options::="--force-confold" make software-properties-common \
  #build-essential libtool autoconf libssl-dev libboost-dev libboost-chrono-dev libboost-filesystem-dev libboost-program-options-dev \
  #libboost-system-dev libboost-test-dev libboost-thread-dev sudo automake git wget curl libdb4.8-dev bsdmainutils libdb4.8++-dev \
  #libminiupnpc-dev libgmp3-dev ufw pkg-config libevent-dev libzmq5 libdb5.3++ mc unzip bash-completion >/dev/null 2>&1

  echo -e "Installing required packages, it may take some time to finish"
  apt-get install -y -o Dpkg::Options::="--force-confdef" -o Dpkg::Options::="--force-confold" \
    binutils-gold wget curl bsdmainutils ufw pkg-config mc unzip \
    gawk bash-completion software-properties-common >/dev/null 2>&1

  if [ "$?" -gt "0" ];
  then
    echo -e "${R}Not all required packages were installed properly. Try to install them manually by running the following commands:${NC}\n"
    echo "apt-get update"
    echo "apt -y install software-properties-common"
    echo "apt-add-repository -y ppa:bitcoin/bitcoin"
    echo "apt-get update"
    echo "apt install -y openssh-server build-essential git automake autoconf pkg-config \
libssl-dev libboost-all-dev libprotobuf-dev libdb5.3-dev libdb5.3++-dev protobuf-compiler \
cmake curl g++-multilib libtool binutils-gold bsdmainutils pkg-config python3 libevent-dev \
screen libqt5gui5 libqt5core5a libqt5dbus5 qttools5-dev qttools5-dev-tools libqrencode-dev \
libprotobuf-dev protobuf-compiler mc unzip bash-completion"
    exit 1
  fi
  echo -e "Done\n"
}

function important_information() {
  echo
  echo -e "${G}!!! PLEASE SAVE THIS INFORMATION !!!${NC}"
  echo -e "================================================================================================================================"
  echo -e "${G}$COIN_NAME${NC} D.I.N. is up and running listening on port ${R}$COIN_PORT${NC}."
  echo -e "Configuration file is: ${R}$CONFIGFOLDER/$CONFIG_FILE${NC}"
  echo -e "Start: ${R}systemctl start $COIN_NAME.service${NC}"
  echo -e "Stop: ${R}systemctl stop $COIN_NAME.service${NC}"
  echo -e "VPS_IP:PORT ${R}$NODEIP:$COIN_PORT${NC}"
  echo -e "D.I.N. NODE PRIVATEKEY is: ${R}$COINKEY${NC}"
  echo -e "D.I.N. NODE PUBLICKEY is: ${R}$PUBLICKEY${NC}"
  echo -e "D.I.N. NODE ADDRESS is: ${R}$ADDRESS${NC}"
  echo -e "Please check ${G}$COIN_NAME${NC} is running with the following command: ${R}systemctl status $COIN_NAME.service${NC}"
  echo -e "================================================================================================================================"
  echo -e "${G}!!! PLEASE SAVE THIS INFORMATION !!!${NC}"
  echo
  echo "Quick help:"
  echo "1. send 5 coins to $ADDRESS"
  echo "2. send 1000025/500025/100025 coins to OwnerAddress, тake it from your desktop wallet"
  echo "3. wait for 6 confirmations"
  echo -e "4. ${G}infinitynodeburnfund OwnerAddress 1000000/500000/100000 BackupAddress${NC}"
  echo "5. wait for 6 confirmations"
  echo -e "6. ${G}infinitynodeupdatemeta OwnerAddress $PUBLICKEY $NODEIP first_16_char_BurnFundTx${NC}"
  echo "7. wait 55 confirmations to activate infinity node"
  echo
  echo -e "${Y}For security, the ${R}root${Y} user has been disabled for login via ssh using a password${NC}"
  echo -e "To work with the node, please log in as a user ${G}$NODEUSER${NC}"
  echo
}

function setup_node() {
  get_ip
  create_config
  create_key
  update_config
  install_bootstrap
  enable_firewall
  important_information
  configure_systemd
}

##### Main #####

checks
create_user
create_swap
prepare_system
download_node
setup_node
