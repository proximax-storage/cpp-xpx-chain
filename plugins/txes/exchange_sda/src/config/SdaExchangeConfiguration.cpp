/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "SdaExchangeConfiguration.h"
#include "catapult/utils/ConfigurationUtils.h"
#include "catapult/utils/ConfigurationBag.h"

namespace catapult { namespace config {

	SdaExchangeConfiguration SdaExchangeConfiguration::Uninitialized() {
		return SdaExchangeConfiguration();
	}

	SdaExchangeConfiguration SdaExchangeConfiguration::LoadFromBag(const utils::ConfigurationBag& bag) {
		SdaExchangeConfiguration config;

#define LOAD_PROPERTY(NAME) utils::LoadIniProperty(bag, "", #NAME, config.NAME)
		LOAD_PROPERTY(Enabled);
		LOAD_PROPERTY(MaxOfferDuration);
		LOAD_PROPERTY(LongOfferKey);
		LOAD_PROPERTY(OfferSortPolicy);
#undef LOAD_PROPERTY

		utils::VerifyBagSizeLte(bag, PluginConfiguration::CommonPropertyNumber() + 4);
		return config;
	}
}}
