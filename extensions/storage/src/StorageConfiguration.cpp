/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "StorageConfiguration.h"
#include "catapult/config/ConfigurationFileLoader.h"
#include "catapult/utils/ConfigurationUtils.h"

namespace catapult { namespace storage {

#define LOAD_PROPERTY(SECTION, NAME) utils::LoadIniProperty(bag, SECTION, #NAME, config.NAME)

	StorageConfiguration StorageConfiguration::Uninitialized() {
		return StorageConfiguration();
	}

	StorageConfiguration StorageConfiguration::LoadFromBag(const utils::ConfigurationBag& bag) {
		StorageConfiguration config;

#define LOAD_DB_PROPERTY(NAME) LOAD_PROPERTY("replicator", NAME)

		LOAD_DB_PROPERTY(Host);
		LOAD_DB_PROPERTY(Port);
		LOAD_DB_PROPERTY(TransactionTimeout);
		LOAD_DB_PROPERTY(StorageDirectory);
		LOAD_DB_PROPERTY(SandboxDirectory);
		LOAD_DB_PROPERTY(UseTcpSocket);
		LOAD_DB_PROPERTY(UseRpcReplicator);
		LOAD_DB_PROPERTY(RpcHost);
		LOAD_DB_PROPERTY(RpcPort);
		LOAD_DB_PROPERTY(RpcHandleLostConnection);
		LOAD_DB_PROPERTY(RpcDbgChildCrash);

#undef LOAD_DB_PROPERTY

		utils::VerifyBagSizeLte(bag, 11);
		return config;
	}

#undef LOAD_PROPERTY

	StorageConfiguration StorageConfiguration::LoadFromPath(const boost::filesystem::path& resourcesPath) {
		return config::LoadIniConfiguration<StorageConfiguration>(resourcesPath / "config-storage.properties");
	}
}}
