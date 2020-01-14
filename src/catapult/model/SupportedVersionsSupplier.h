/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/functions.h"
#include "catapult/types.h"

namespace catapult { namespace model {

	using SupportedVersionsSupplier = supplier<const VersionSet &>;

#define DEFINE_SUPPORTED_TRANSACTION_VERSION_SUPPLIER(NAME, CONFIG, PLUGIN) \
    struct NAME##TransactionSupportedVersionSupplier { \
        NAME##TransactionSupportedVersionSupplier(const model::BlockChainConfiguration& config) : m_config(config) \
        {} \
        const VersionSet& operator()() const { \
            const auto& pluginConfig = m_config.Network.template GetPluginConfiguration<config::CONFIG##Configuration>(); \
            return pluginConfig.NAME##TransactionSupportedVersions; \
        } \
    private: \
        const model::BlockChainConfiguration& m_config; \
    };
}}