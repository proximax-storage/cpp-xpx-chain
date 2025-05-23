/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "StorageConfiguration.h"
#include "catapult/config/ConfigurationFileLoader.h"
#include "catapult/utils/ConfigurationUtils.h"

namespace catapult { namespace storage {

	StorageConfiguration StorageConfiguration::Uninitialized() {
		return StorageConfiguration();
	}

	StorageConfiguration StorageConfiguration::LoadFromBag(const utils::ConfigurationBag& bag) {
		StorageConfiguration config;

#define LOAD_DB_PROPERTY(NAME) utils::LoadIniProperty(bag, "replicator", #NAME, config.NAME)

		LOAD_DB_PROPERTY(Key);
		LOAD_DB_PROPERTY(Host);
		LOAD_DB_PROPERTY(Port);
		LOAD_DB_PROPERTY(TransactionTimeout);
		LOAD_DB_PROPERTY(StorageDirectory);
		LOAD_DB_PROPERTY(UseTcpSocket);
		LOAD_DB_PROPERTY(UseRpcReplicator);
		LOAD_DB_PROPERTY(RpcHost);
		LOAD_DB_PROPERTY(RpcPort);
		LOAD_DB_PROPERTY(RpcHandleLostConnection);
		LOAD_DB_PROPERTY(RpcDbgChildCrash);

#undef LOAD_DB_PROPERTY

#define TRY_LOAD_DB_PROPERTY(NAME) utils::TryLoadIniProperty(bag, "replicator", #NAME, config.NAME)

		config.LogOptions = "";
		TRY_LOAD_DB_PROPERTY(LogOptions);

#undef TRY_LOAD_DB_PROPERTY

		return config;
	}

	StorageConfiguration StorageConfiguration::LoadFromPath(const boost::filesystem::path& resourcesPath) {
		return config::LoadIniConfiguration<StorageConfiguration>(resourcesPath / "config-storage.properties");
	}
}}
