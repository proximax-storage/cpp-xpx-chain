rm -R _build
mkdir _build
cd _build

cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-pthread" ..

make publish
# First we build extensions which catapult.server required during runtime
make \
    # Required extensions
    catapult.mongo.plugins.accountlink \
    catapult.mongo.plugins.aggregate \
    catapult.mongo.plugins.contract \
    catapult.mongo.plugins.lockhash \
    catapult.mongo.plugins.locksecret \
    catapult.mongo.plugins.metadata \
    catapult.mongo.plugins.mosaic \
    catapult.mongo.plugins.multisig \
    catapult.mongo.plugins.namespace \
    catapult.mongo.plugins.property \
    catapult.mongo.plugins.transfer \
    catapult.plugins.accountlink \
    catapult.plugins.aggregate \
    catapult.plugins.config \
    catapult.plugins.contract \
    catapult.plugins.hashcache \
    catapult.plugins.hashcache.cache \
    catapult.plugins.lockhash \
    catapult.plugins.locksecret \
    catapult.plugins.metadata \
    catapult.plugins.mosaic \
    catapult.plugins.multisig \
    catapult.plugins.namespace \
    catapult.plugins.property \
    catapult.plugins.signature \
    catapult.plugins.transfer \
    catapult.plugins.upgrade \
    extension.addressextraction \
    extension.diagnostics \
    extension.eventsource \
    extension.filechain \
    extension.harvesting \
    extension.hashcache \
    extension.mongo \
    extension.networkheight \
    extension.nodediscovery \
    extension.packetserver \
    extension.partialtransaction \
    extension.pluginhandlers \
    extension.sync \
    extension.syncsource \
    extension.timesync \
    extension.transactionsink \
    extension.unbondedpruning \
    extension.zeromq \
    # Tools
    catapult.tools.address \
    catapult.tools.health \
    catapult.tools.nemgen \
    catapult.tools.network \
    # Catapult
    catapult.server \
    -j4

cd ..

# Now we want to create a docker image,
# so we need to create it with shared libs which is required by catapult.server and extensions
mkdir ./temp
# We copy all libs to temp folder
./scripts/release-script/copyDeps.sh ./_build/bin/ ./temp
