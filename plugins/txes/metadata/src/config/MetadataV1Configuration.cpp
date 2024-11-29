/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MetadataV1Configuration.h"
#include "catapult/utils/ConfigurationUtils.h"

namespace catapult { namespace config {

	MetadataV1Configuration MetadataV1Configuration::Uninitialized() {
		return MetadataV1Configuration();
	}

	MetadataV1Configuration MetadataV1Configuration::LoadFromBag(const utils::ConfigurationBag& bag) {
		MetadataV1Configuration config;

#define LOAD_METADATA(NAME) utils::LoadIniProperty(bag, "", #NAME, config.NAME)

		LOAD_METADATA(MaxFields);
		LOAD_METADATA(MaxFieldKeySize);
		LOAD_METADATA(MaxFieldValueSize);

#undef LOAD_METADATA

		utils::VerifyBagSizeLte(bag, PluginConfiguration::CommonPropertyNumber() + 3);
		return config;
	}
}}
