##########################################
Run debug catpult
##########################################
###

rm -f ./cmake-build-debug/data/*/data/server.lock

#### Clion
in "Edit Vonfiguration..." Add(+) 1+5 "console applications"
with "Program agruments:" data/api-node-0 or data/peer-node-0 (0...4)
with "Working directory:" .../cmake-build-debug
in "Edit Configuration..." Add(+) "compound"

#### Rest
brew install yarn node nvm
!!!
source $(brew --prefix nvm)/nvm.sh
nvm install 12.14.1
!!!

cd .../js-xpx-chain-rest/rest
yarn run build
yarn run start


#### Clear mongodb
(https://www.mongodb.com/docs/manual/tutorial/install-mongodb-on-os-x/)
(brew install mongodb-community)
(M1)?????rm -rf /opt/homebrew/var/mongodb/*

"Intel"
brew services stop mongodb/brew/mongodb-community
rm -rf /usr/local/var/mongodb/*
brew services start mongodb/brew/mongodb-community

"Silicon"
brew services stop mongodb/brew/mongodb-community
rm -rf /opt/homebrew/var/mongodb/*
brew services start mongodb/brew/mongodb-community

#### Generate nemesis block
/scripts/bootstrap/runCatapultServers.sh

#### LISTEN ports
lsof -i -n -P | grep LISTEN
netstat -anvp tcp | awk 'NR<3 || /LISTEN/' | grep 7915

#### Remove server.lock
rm -f ./cmake-build-debug/data/*/data/server.lock

#### Prepare
cd git@github.com:proximax-storage/go-xpx-chain-sdk.git
cd go-xpx-chain-sdk
git checkout 26873aac8592de1a2d3e489ce3c24610115a4bbd  (?)
cd tools/liquidity_provider/
go build
cd ../transfer
go build

#### User Key
3C7C91E82BF69B206A523E64DB21B07598834970065ABCFA2BB4212138637E0B

#### useRpcReplicator = false
cd ~/Proj/cpp-xpx-chain/cmake-build-debug/data
find . -type f -name "config-storage.properties" -exec sed -i '' 's/useRpcReplicator = true/useRpcReplicator = false/g' {} +

docker compose down
docker rmi -f <efee6fff0d12>
rm -rf replicator_service_logs/*
aws ecr get-login-password --region ap-southeast-1 |docker login --username AWS --password-stdin 249767383774.dkr.ecr.ap-southeast-1.amazonaws.com/proximax-catapult-server && rm -rf /opt/catapult-config/replicator_service_logs/* && rm -f /opt/catapult-config/data/server.lock && docker compose up -d

