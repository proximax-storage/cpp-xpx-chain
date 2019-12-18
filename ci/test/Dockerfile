FROM 249767383774.dkr.ecr.ap-southeast-1.amazonaws.com/proximax-catapult-server-dependencies-build-image:v1.0-buster

COPY . /cpp-xpx-chain/
WORKDIR /cpp-xpx-chain/
RUN ls -al
RUN echo 'building sirius-chain server'
RUN rm -rf _build
RUN mkdir _build
RUN cd _build && cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-pthread" .. && \
    make publish && make \
    tests.catapult.addressextraction \
    tests.catapult.api \
    ## tests.catapult.cache \
    tests.catapult.cache_core \
    tests.catapult.cache_db \
    tests.catapult.cache_tx \
    tests.catapult.chain \
    tests.catapult.config \
    tests.catapult.config_holder \
    tests.catapult.consumers \
    tests.catapult.coresystem \
    tests.catapult.crypto \
    tests.catapult.deltaset \
    tests.catapult.diagnostics \
    tests.catapult.disruptor \
    tests.catapult.eventsource \
    tests.catapult.extensions \
    tests.catapult.filespooling \
    tests.catapult.handlers \
    tests.catapult.harvesting \
    tests.catapult.hashcache \
    tests.catapult.int.mongo \
    tests.catapult.int.mongo.plugins.config \
    tests.catapult.int.mongo.plugins.contract \
    tests.catapult.int.mongo.plugins.lockhash \
    tests.catapult.int.mongo.plugins.locksecret \
    tests.catapult.int.mongo.plugins.metadata \
    tests.catapult.int.mongo.plugins.mosaic \
    tests.catapult.int.mongo.plugins.multisig \
    tests.catapult.int.mongo.plugins.namespace \
    # tests.catapult.int.mongo.plugins.property \
    tests.catapult.int.mongo.plugins.upgrade \
    tests.catapult.int.node \
    tests.catapult.int.node.stress \
    tests.catapult.int.stress \
    tests.catapult.int.stress.patriciaTreeGenerator \
    tests.catapult.io \
    tests.catapult.ionet \
    tests.catapult.local.broker \
    tests.catapult.local.recovery \
    tests.catapult.local.server \
    tests.catapult.model \
    tests.catapult.mongo \
    tests.catapult.mongo.plugins.accountlink \
    tests.catapult.mongo.plugins.aggregate \
    tests.catapult.mongo.plugins.config \
    tests.catapult.mongo.plugins.contract \
    tests.catapult.mongo.plugins.exchange \
    tests.catapult.mongo.plugins.lockhash \
    tests.catapult.mongo.plugins.locksecret \
    tests.catapult.mongo.plugins.metadata \
    tests.catapult.mongo.plugins.mosaic \
    tests.catapult.mongo.plugins.multisig \
    tests.catapult.mongo.plugins.namespace \
    tests.catapult.mongo.plugins.property \
    tests.catapult.mongo.plugins.transfer \
    tests.catapult.mongo.plugins.upgrade \
    tests.catapult.net \
    tests.catapult.networkheight \
    tests.catapult.nodediscovery \
    tests.catapult.observers \
    tests.catapult.packetserver \
    tests.catapult.partialtransaction \
    tests.catapult.pluginhandlers \
    tests.catapult.plugins \
    tests.catapult.plugins.accountlink \
    tests.catapult.plugins.aggregate \
    tests.catapult.plugins.config \
    tests.catapult.plugins.contract \
    tests.catapult.plugins.hashcache \
    tests.catapult.plugins.lockhash \
    tests.catapult.plugins.locksecret \
    tests.catapult.plugins.metadata \
    tests.catapult.plugins.mosaic \
    tests.catapult.plugins.multisig \
    tests.catapult.plugins.namespace \
    tests.catapult.plugins.property \
    tests.catapult.plugins.signature \
    tests.catapult.plugins.transfer \
    tests.catapult.plugins.upgrade \
    tests.catapult.sdk \
    tests.catapult.state \
    tests.catapult.subscribers \
    tests.catapult.sync \
    tests.catapult.syncsource \
    tests.catapult.thread \
    tests.catapult.timesync \
    tests.catapult.transactionsink \
    tests.catapult.tree \
    tests.catapult.unbondedpruning \
    tests.catapult.utils \
    tests.catapult.validators \
    tests.catapult.zeromq

RUN echo 'Running unit tests'
RUN set +e
RUN cd _build && ctest -T Test --no-compress-output || true


