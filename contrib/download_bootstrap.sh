#!/bin/bash

NODEUSER=$(whoami)
CONFIGFOLDER="/home/$NODEUSER/.sin"

if ! test -d $CONFIGFOLDER; then
    echo "Dir $CONFIGFOLDER not found"
    exit 1
fi

# + bootstrap
BS_REMOTE="https://service.sinovate.io/mainnet/latest/bootstrap.zip"
BS_LOCAL="/home/$NODEUSER/bootstrap.zip"
BSLEN_REMOTE=$(wget --spider $BS_REMOTE 2>&1 | grep Length | cut -d " " -f2)
BSLEN_LOCAL=$(du -b $BS_LOCAL 2>/dev/null | cut -f1)
# - bootstrap

function install_bootstrap() {
    read -n 1 -p "Download bootstrap (y/n)? " CHOICE
    case "$CHOICE" in
        y|Y ) echo;;
        * )   echo -e "\nSkipping bootstrap download."
              exit;;
    esac

    if [ ! "$BSLEN_REMOTE" ]; then
        echo -e "File $BS_REMOTE not found.\nSkipping bootstrap download."
        exit 1
    fi

    # download bootstarp
    if [ "$BSLEN_REMOTE" != "$BSLEN_LOCAL" ]; then
        echo "Start downloading $(basename $BS_LOCAL)..."
        wget -q -t 10 --show-progress --no-check-certificate -O $BS_LOCAL $BS_REMOTE
        if [ "$?" -ne 0 ]; then
            test -f $BS_LOCAL && rm $BS_LOCAL
            echo "File download error! Skipping bootstrap download."
            exit 1
        else
            echo "File download successful."
        fi
    fi
    chown $NODEUSER:$NODEUSER $BS_LOCAL

    # extract bootstrap
    echo -n "Extracting $(basename $BS_LOCAL) "
    mkdir -p $CONFIGFOLDER
    unzip -o -d $CONFIGFOLDER $BS_LOCAL | awk 'BEGIN {ORS="."} {if(NR%25==0) print "."}'; echo
    if [ "$?" -ne 0 ]; then
        echo "Bootstrap extracting error!"
        exit 1
    fi
    echo "Bootstrap extracting successful."
    chown -R $NODEUSER:$NODEUSER $CONFIGFOLDER
    # bypassing the mv error "Directory not empty"
    rm -R $CONFIGFOLDER/blocks $CONFIGFOLDER/chainstate $CONFIGFOLDER/indexes 2>/dev/null
    mv -f $CONFIGFOLDER/bootstrap/* $CONFIGFOLDER && rm -R $CONFIGFOLDER/bootstrap/
}

install_bootstrap
