/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include <stdint.h>
#include "catapult/utils/Casting.h"

namespace catapult { namespace config {

	/// Config ids for well-known caches.
	enum class ConfigId : uint32_t {
		aggregateConfigId,
		configConfigId,
		contractConfigId,
		locksecretConfigId,
		lockhashConfigId,
		metadataConfigId,
		mosaicConfigId,
		multisigConfigId,
		namespaceConfigId,
		propertyConfigId,
		serviceConfigId,
		supercontractConfigId,
		transferConfigId,
		upgradeConfigId,
        exchangeConfigId,
        operationConfigId,
		metadata_v2ConfigId,
		committeeConfigId,
		storageConfigId,
		streamingConfigId,
		liquidityproviderConfigId,
		contract_v2ConfigId,
        First = static_cast<uint32_t>(aggregateConfigId),
        Latest = static_cast<uint32_t>(contract_v2ConfigId),
	};

/// Defines config constants for a config with \a NAME.
#define DEFINE_CONFIG_CONSTANTS(NAME) \
	static constexpr size_t Id = utils::to_underlying_type(ConfigId::NAME##ConfigId); \
	static constexpr auto Name = "catapult.plugins."#NAME;
}}
