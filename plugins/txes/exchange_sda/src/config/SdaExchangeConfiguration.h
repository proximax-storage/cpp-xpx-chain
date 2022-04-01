/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/PluginConfiguration.h"
#include "catapult/types.h"
#include "catapult/utils/Hashers.h"

namespace catapult { namespace utils { class ConfigurationBag; } }

namespace catapult { namespace config {

	/// SDA Exchange plugin configuration settings.
	struct SdaExchangeConfiguration : public model::PluginConfiguration {
	public:
		DEFINE_CONFIG_CONSTANTS(exchangesda)

	public:
		/// Whether the plugin is enabled.
		bool Enabled;

		/// Maximum offer duration.
		BlockDuration MaxOfferDuration;

		/// A public key of the account allowed to set
		/// offer duration exceeding MaxOfferDuration.
		Key LongOfferKey;

		/// Policy to sort SDA-SDA offers.
		enum class SortPolicies : uint8_t {
			Default,
			SmallToBig,
			SmallToBigSortedByEarliestExpiry,
			BigToSmall, 
			BigToSmallSortedByEarliestExpiry,
			ExactOrClosest
		};

		SortPolicies SortPolicy;

	private:
		SdaExchangeConfiguration() = default;

	public:
		/// Creates an uninitialized service configuration.
		static SdaExchangeConfiguration Uninitialized();

		/// Loads an service configuration from \a bag.
		static SdaExchangeConfiguration LoadFromBag(const utils::ConfigurationBag& bag);
	};
}}
