/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/config/BlockchainConfiguration.h"
#include "catapult/model/InflationCalculator.h"
#include <mutex>  // For std::unique_lock
#include <shared_mutex>
#include <thread>

namespace catapult { namespace cache { class CatapultCache; } }

namespace catapult { namespace config {

	constexpr Height HEIGHT_OF_LATEST_CONFIG = Height(-1);
using namespace std::chrono_literals;
	class BlockchainConfigurationHolder {
	public:
		using PluginInitializer = std::function<void(model::NetworkConfiguration&)>;

		explicit BlockchainConfigurationHolder(cache::CatapultCache* pCache =  nullptr);
		explicit BlockchainConfigurationHolder(const BlockchainConfiguration& config);
		explicit BlockchainConfigurationHolder(const BlockchainConfiguration& config, cache::CatapultCache* pCache, const Height& height);
#ifdef CLEANUP_LOGGING_ENABLED
		virtual CATAPULT_DESTRUCTOR_CLEANUP_LOG(info, BlockchainConfigurationHolder, "Destroying blockchain configuration holder.")
#else
		virtual ~BlockchainConfigurationHolder() = default;
#endif
	public:
		/// Extracts the resources path from the command line arguments.
		/// \a argc commmand line arguments are accessible via \a argv.
		static boost::filesystem::path GetResourcesPath(int argc, const char** argv);

		/// Get \a config at \a height
		virtual const BlockchainConfiguration& Config(const Height& height) const;

		/// Get latest available config
		virtual const BlockchainConfiguration& Config() const;

		/// Initialize init configuration

		/// Removes all plugin configs in all network configs.
		void ClearPluginConfigurations() const;

		/// Get config at \a height or latest available config
		virtual const BlockchainConfiguration& ConfigAtHeightOrLatest(const Height& height) const;

		void SetCache(cache::CatapultCache* pCache) {
			m_pCache = pCache;
		}

		void InitializeNetworkConfiguration(const model::NetworkConfiguration& networkConfiguration, const config::SupportedEntityVersions& supportedEntities);
		void SetPluginInitializer(const PluginInitializer& initializer) {
			m_pluginInitializer = initializer;
		}

		void InsertConfig(const Height& height, const std::string& strConfig, const std::string& supportedVersion);


		void RemoveConfig(const Height& height);

		const model::InflationCalculator& InflationCalculator();
	private:
		/// Must be used with a locked m_mutex
		const BlockchainConfiguration* LastConfigOrNull(const Height& height) const;

	protected:
		void InsertConfig(const Height& height, const model::NetworkConfiguration& networkConfig, const config::SupportedEntityVersions& supportedEntities);
	protected:
		std::map<Height, BlockchainConfiguration> m_configs;
		cache::CatapultCache* m_pCache;
		PluginInitializer m_pluginInitializer;
		std::unique_ptr<model::InflationCalculator> m_pInflationCalculator;
		mutable std::shared_mutex m_mutex;
	};
}}
