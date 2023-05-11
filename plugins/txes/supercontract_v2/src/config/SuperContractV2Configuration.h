/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/PluginConfiguration.h"
#include "catapult/types.h"

namespace catapult { namespace utils { class ConfigurationBag; } }

namespace catapult { namespace config {

	/// Super contract plugin configuration settings.
	struct SuperContractV2Configuration : public model::PluginConfiguration {
	public:
		DEFINE_CONFIG_CONSTANTS(supercontract_v2)

		/// Whether the plugin is enabled.
		bool Enabled;

		/// Maximum transaction mosaics size.
		uint16_t MaxServicePaymentsSize;

		/// Maximum size of rows in transactions
		uint16_t MaxRowSize;

		/// Maximum execution payment per call
		uint64_t MaxExecutionPayment;

		/// Maximum auto executions to be prepaid
		uint64_t MaxAutoExecutions;

		/// Automatic Executions deadline
		Height AutomaticExecutionsDeadline;

	private:
		SuperContractV2Configuration() = default;

	public:
		/// Creates an uninitialized Super contract configuration.
		static SuperContractV2Configuration Uninitialized();

		/// Loads an Super contract configuration from \a bag.
		static SuperContractV2Configuration LoadFromBag(const utils::ConfigurationBag& bag);
	};
}}
