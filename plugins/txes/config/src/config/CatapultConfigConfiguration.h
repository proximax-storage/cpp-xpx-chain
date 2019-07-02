/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/utils/FileSize.h"
#include "catapult/model/PluginConfiguration.h"
#include <stdint.h>

namespace catapult { namespace utils { class ConfigurationBag; } }

namespace catapult { namespace config {

	/// Catapult config plugin configuration settings.
	struct CatapultConfigConfiguration : public model::PluginConfiguration {
	public:
		/// Maximum blockchain config data size.
		utils::FileSize MaxBlockChainConfigSize;

		/// Maximum supported entity versions config data size.
		utils::FileSize MaxSupportedEntityVersionsSize;

	private:
		CatapultConfigConfiguration() = default;

	public:
		/// Creates an uninitialized catapult config plugin configuration.
		static CatapultConfigConfiguration Uninitialized();

		/// Loads an catapult config plugin configuration from \a bag.
		static CatapultConfigConfiguration LoadFromBag(const utils::ConfigurationBag& bag);
	};
}}
