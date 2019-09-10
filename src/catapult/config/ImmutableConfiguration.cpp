/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ImmutableConfiguration.h"
#include "catapult/utils/ConfigurationBag.h"
#include "catapult/utils/ConfigurationUtils.h"

namespace catapult { namespace config {

	// region ImmutableConfiguration

	ImmutableConfiguration ImmutableConfiguration::Uninitialized() {
		return ImmutableConfiguration();
	}

	ImmutableConfiguration ImmutableConfiguration::LoadFromBag(const utils::ConfigurationBag& bag) {
		ImmutableConfiguration config;

#define LOAD_IMMUTABLE_PROPERTY(NAME) utils::LoadIniProperty(bag, "immutable", #NAME, config.NAME)

		LOAD_IMMUTABLE_PROPERTY(NetworkIdentifier);
		LOAD_IMMUTABLE_PROPERTY(GenerationHash);

		LOAD_IMMUTABLE_PROPERTY(ShouldEnableVerifiableState);
		LOAD_IMMUTABLE_PROPERTY(ShouldEnableVerifiableReceipts);

		LOAD_IMMUTABLE_PROPERTY(CurrencyMosaicId);
		LOAD_IMMUTABLE_PROPERTY(HarvestingMosaicId);

		LOAD_IMMUTABLE_PROPERTY(InitialCurrencyAtomicUnits);

#undef LOAD_IMMUTABLE_PROPERTY

		utils::VerifyBagSizeLte(bag, 7);
		return config;
	}

	// endregion

	// region utils

	UnresolvedMosaicId GetUnresolvedCurrencyMosaicId(const ImmutableConfiguration& config) {
		return UnresolvedMosaicId(config.CurrencyMosaicId.unwrap());
	}

	// endregion
}}
