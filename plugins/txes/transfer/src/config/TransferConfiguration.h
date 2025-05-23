/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#pragma once
#include "catapult/model/PluginConfiguration.h"
#include <stdint.h>

namespace catapult { namespace utils { class ConfigurationBag; } }

namespace catapult { namespace config {

	/// Transfer plugin configuration settings.
	struct TransferConfiguration : public model::PluginConfiguration {
	public:
		DEFINE_CONFIG_CONSTANTS(transfer)

	public:
		/// Maximum transaction message size.
		uint16_t MaxMessageSize;
		/// Maximum transaction mosaics size.
		uint16_t MaxMosaicsSize;

	private:
		TransferConfiguration() = default;

	public:
		/// Creates an uninitialized transfer configuration.
		static TransferConfiguration Uninitialized();

		/// Loads a transfer configuration from \a bag.
		static TransferConfiguration LoadFromBag(const utils::ConfigurationBag& bag);
	};
}}
