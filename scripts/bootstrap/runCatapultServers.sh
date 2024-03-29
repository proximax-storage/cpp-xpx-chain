PATH_TO_CATAPULT_SERVER=$(dirname $(dirname $(dirname $(realpath $0))))
PATH_TO_BOOTSTRAP=$PATH_TO_CATAPULT_SERVER/scripts/bootstrap

WORK_DIR=$PATH_TO_CATAPULT_SERVER/cmake-build-debug
num_addresses=50
raw_addresses_path=$WORK_DIR/addresses/raw-addresses.txt
formatted_address_path=$WORK_DIR/addresses/addresses.yaml

sudo rm -R $WORK_DIR/data/
sudo rm -R $WORK_DIR/nemesis/
sudo rm -R $WORK_DIR/config-build/
#sudo rm -R $WORK_DIR/addresses/

mkdir -p $WORK_DIR/addresses
if [ ! -f $formatted_address_path ]; then
  bash -c "$WORK_DIR/bin/catapult.tools.address --generate=$num_addresses -n mijin-test > $raw_addresses_path"
  bash -c "$PATH_TO_BOOTSTRAP/ruby/bin/store-addresses-if-needed.rb $WORK_DIR/addresses/raw-addresses.txt $WORK_DIR/addresses/addresses.yaml"
fi

mkdir -p $WORK_DIR/config-build
mkdir -p $WORK_DIR/nemesis
bash -c "$PATH_TO_BOOTSTRAP/ruby/bin/generate-and-write-configurations.rb $WORK_DIR/addresses/addresses.yaml $WORK_DIR/config-build $WORK_DIR/nemesis"

mkdir -p $WORK_DIR/data

generate_nem() {
  if [ ! -d $WORK_DIR/data ]; then
    echo "/data directory does not exist"
    exit 1
  fi

  if [[ "$OSTYPE" == "darwin"* ]]; then
    # Mac OSX
    sed -i '' "/dataDirectory/d" $WORK_DIR/config-build/$1/userconfig/resources/config-user.properties
    sed -i '' "/pluginsDirectory/d" $WORK_DIR/config-build/$1/userconfig/resources/config-user.properties
  else
    # Unknown.
    sed -i "/dataDirectory/d" $WORK_DIR/config-build/$1/userconfig/resources/config-user.properties
    sed -i "/pluginsDirectory/d" $WORK_DIR/config-build/$1/userconfig/resources/config-user.properties
  fi
  echo "dataDirectory = $WORK_DIR/data/$1/data" >>$WORK_DIR/config-build/$1/userconfig/resources/config-user.properties
  echo "pluginsDirectory = $WORK_DIR/bin" >>$WORK_DIR/config-build/$1/userconfig/resources/config-user.properties

  if [ ! -d $WORK_DIR/data/$1/data/00000 ]; then

    mkdir -p $WORK_DIR/data/$1/data/
    cd $WORK_DIR/data/$1/
    echo "running nemgen $1"

    if [ ! -d $WORK_DIR/nemesis/data ]; then
      mkdir settings
      mkdir -p seed/mijin-test/00000
      dd if=/dev/zero of=seed/mijin-test/00000/hashes.dat bs=1 count=64
      cd settings
      cp -R $WORK_DIR/config-build/$1/userconfig/resources ../resources
      $WORK_DIR/bin/catapult.tools.nemgen --nemesisProperties $WORK_DIR/nemesis/block-properties-file.properties
      cp -r $WORK_DIR/data/$1/seed/mijin-test/* $WORK_DIR/data/$1/data/
      cp -r $WORK_DIR/data/$1/data $WORK_DIR/nemesis/
      rm -R $WORK_DIR/data/$1/seed
      rm -R $WORK_DIR/data/$1/settings
      cd -
    else
      cp -r $WORK_DIR/nemesis/data $WORK_DIR/data/$1/
      cp -R $WORK_DIR/config-build/$1/userconfig/resources $WORK_DIR/data/$1/
    fi

  else
    echo "no need to run nemgen"
  fi

#  $WORK_DIR/bin/sirius.bc $WORK_DIR/config-build/$1/userconfig &>/dev/null 2>&1 &
  echo "You can find logs in '$WORK_DIR/data/$1/'"
}

generate_nem "api-node-0"
generate_nem "peer-node-0"
generate_nem "peer-node-1"
generate_nem "peer-node-2"
generate_nem "peer-node-3"
generate_nem "peer-node-4"

echo "You can kill all catapult servers 'killall $WORK_DIR/bin/sirius.bc'"
echo "multitail -i $WORK_DIR/data/api-node-0/catapult_server0000.log -i $WORK_DIR/data/peer-node-0/catapult_server0000.log -i $WORK_DIR/data/peer-node-1/catapult_server0000.log -i $WORK_DIR/data/peer-node-2/catapult_server0000.log -i $WORK_DIR/data/peer-node-3/catapult_server0000.log -i $WORK_DIR/data/peer-node-4/catapult_server0000.log"
echo "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
# echo "tail -f $WORK_DIR/data/api-node-0/catapult_server0000.log $WORK_DIR/data/peer-node-1/catapult_server0000.log $WORK_DIR/data/peer-node-1/catapult_server0000.log"
# tail -f $WORK_DIR/api-node-0.log $WORK_DIR/peer-node-0.log $WORK_DIR/peer-node-1.log
