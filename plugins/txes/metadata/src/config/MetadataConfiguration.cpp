/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MetadataConfiguration.h"
#include "catapult/utils/ConfigurationBag.h"
#include "catapult/utils/ConfigurationUtils.h"

namespace catapult { namespace config {

	MetadataConfiguration MetadataConfiguration::Uninitialized() {
		return MetadataConfiguration();
	}

	MetadataConfiguration MetadataConfiguration::LoadFromBag(const utils::ConfigurationBag& bag) {
		MetadataConfiguration config;

#define LOAD_METADATA(NAME) utils::LoadIniProperty(bag, "", #NAME, config.NAME)

		LOAD_METADATA(AddressMetadataTransactionSupportedVersions);
		LOAD_METADATA(MosaicMetadataTransactionSupportedVersions);
		LOAD_METADATA(NamespaceMetadataTransactionSupportedVersions);
		LOAD_METADATA(MaxFields);
		LOAD_METADATA(MaxFieldKeySize);
		LOAD_METADATA(MaxFieldValueSize);

#undef LOAD_METADATA

		utils::VerifyBagSizeLte(bag, PluginConfiguration::CommonPropertyNumber() + 6);
		return config;
	}
}}
