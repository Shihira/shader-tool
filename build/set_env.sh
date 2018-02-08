#!/bin/bash
echo 'Info: Please make sure you have run this script as:'
echo '    source set_env.sh'

export SHRTOOL_ASSETS_DIR=$PWD/../assets
#export SHRTOOL_LOG_LEVEL=30000
export GUILE_LOAD_PATH=$SHRTOOL_ASSETS_DIR/modules

if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
    zsh
fi
