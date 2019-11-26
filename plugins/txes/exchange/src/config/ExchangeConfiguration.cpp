/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ExchangeConfiguration.h"
#include "catapult/utils/ConfigurationUtils.h"
#include "catapult/utils/ConfigurationBag.h"

namespace catapult { namespace config {

	ExchangeConfiguration ExchangeConfiguration::Uninitialized() {
		return ExchangeConfiguration();
	}

	ExchangeConfiguration ExchangeConfiguration::LoadFromBag(const utils::ConfigurationBag& bag) {
		ExchangeConfiguration config;

#define LOAD_PROPERTY(NAME) utils::LoadIniProperty(bag, "", #NAME, config.NAME)
		LOAD_PROPERTY(Enabled);
		LOAD_PROPERTY(MaxOfferDuration);
		LOAD_PROPERTY(LongOfferKey);
#undef LOAD_PROPERTY

		utils::VerifyBagSizeLte(bag, PluginConfiguration::CommonPropertyNumber() + 3);
		return config;
	}
}}
