/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "StreamingConfiguration.h"
#include "catapult/utils/ConfigurationUtils.h"

namespace catapult { namespace config {

	StreamingConfiguration StreamingConfiguration::Uninitialized() {
		return StreamingConfiguration();
	}

	StreamingConfiguration StreamingConfiguration::LoadFromBag(const utils::ConfigurationBag& bag) {
		StreamingConfiguration config;

#define LOAD_PROPERTY(NAME) utils::LoadIniProperty(bag, "", #NAME, config.NAME)
		LOAD_PROPERTY(Enabled);
#undef LOAD_PROPERTY

#define TRY_LOAD_CHAIN_PROPERTY(NAME) utils::TryLoadIniProperty(bag, "", #NAME, config.NAME)
		config.MaxFolderNameSize = 512u;
		TRY_LOAD_CHAIN_PROPERTY(MaxFolderNameSize);
#undef TRY_LOAD_CHAIN_PROPERTY

		return config;
	}
}}
