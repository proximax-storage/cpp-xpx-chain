rm -R _build
mkdir _build
cd _build

cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-pthread" ..

make publish
# First we build extensions which sirius.bc required during runtime
make \
    # Required extensions
    catapult.mongo.plugins.accountlink \
    catapult.mongo.plugins.aggregate \
    catapult.mongo.plugins.committee \
    catapult.mongo.plugins.config \
    catapult.mongo.plugins.contract \
    catapult.mongo.plugins.exchange \
    catapult.mongo.plugins.exchangesda \
    catapult.mongo.plugins.lockhash \
    catapult.mongo.plugins.locksecret \
    catapult.mongo.plugins.metadata \
    catapult.mongo.plugins.mosaic \
    catapult.mongo.plugins.multisig \
    catapult.mongo.plugins.namespace \
    catapult.mongo.plugins.property \
    catapult.mongo.plugins.transfer \
    catapult.mongo.plugins.upgrade \
    catapult.mongo.plugins.service \
    catapult.plugins.accountlink \
    catapult.plugins.aggregate \
    catapult.plugins.committee \
    catapult.plugins.config \
    catapult.plugins.contract \
    catapult.plugins.exchange \
    catapult.plugins.exchangesda \
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
    catapult.plugins.service \
    extension.addressextraction \
    extension.diagnostics \
    extension.eventsource \
    extension.filespooling \
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
    catapult.tools.benchmark \
    catapult.tools.health \
    catapult.tools.nemgen \
    catapult.tools.nemgen.blockhashes \
    catapult.tools.network \
    catapult.tools.statusgen \
    # Catapult
    sirius.bc \
    -j 4

cd ..

# Now we want to create a docker image,
# so we need to create it with shared libs which is required by sirius.bc and extensions
mkdir ./temp
# We copy all libs to temp folder
./scripts/release-script/copyDeps.sh ./_build/bin/ ./temp
