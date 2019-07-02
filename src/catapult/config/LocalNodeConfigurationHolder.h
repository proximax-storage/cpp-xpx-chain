/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "LocalNodeConfiguration.h"

namespace catapult {
	namespace io {
		struct OutputStream;
		struct InputStream;
	}
}

namespace catapult { namespace config {

	class BlockChainConfigTraits {
	public:
		using ConfigType = model::BlockChainConfiguration;

		static void SetConfig(LocalNodeConfiguration& catapultConfig, const ConfigType& config) {
			const_cast<ConfigType&>(catapultConfig.BlockChain) = config;
		}

		static ConfigType LoadConfig(std::istream& input) {
			auto bag = utils::ConfigurationBag::FromStream(input);
			return model::BlockChainConfiguration::LoadFromBag(bag);
		}
	};

	class SupportedEntityVersionsTraits {
	public:
		using ConfigType = SupportedEntityVersions;

		static void SetConfig(LocalNodeConfiguration& catapultConfig, const ConfigType& config) {
			const_cast<ConfigType&>(catapultConfig.SupportedEntityVersions) = config;
		}

		static ConfigType LoadConfig(std::istream& input) {
			return LoadSupportedEntityVersions(input);
		}
	};

	template <typename TTraits>
	class SubConfigurationHolder {
	public:
		SubConfigurationHolder(LocalNodeConfiguration& currentLocalNodeConfig) : m_catapultConfig(currentLocalNodeConfig)
		{}

	public:
		class SubConfigurationHolderDelta {
		public:
			explicit SubConfigurationHolderDelta(SubConfigurationHolder& configHolder);
			~SubConfigurationHolderDelta();

		public:
			void Commit();
			void SetSubConfig(const Height& height, const std::string& m_serializedSubConfig);
			void RemoveSubConfig(const Height& height);

		private:
			SubConfigurationHolder& m_configHolder;
			std::map<Height, std::string> m_serializedSubConfigs;
		};

	public:
		void LoadInitialSubConfig(boost::filesystem::path path);
		void SetSubConfig(const typename TTraits::ConfigType& subConfig);
		void SetSubConfig(const Height& height, const std::string& serializedSubConfig);
		void RemoveSubConfig(const Height& height);

		void SaveSubConfigs(io::OutputStream& output);
		void LoadSubConfigs(io::InputStream& input);

		std::unique_ptr<SubConfigurationHolderDelta> CreateDelta();

	private:
		void setInitialSubConfig(const std::string& serializedSubConfig);
		void update();

	private:
		LocalNodeConfiguration& m_catapultConfig;
		std::map<Height, std::string> m_serializedSubConfigs;
		SubConfigurationHolderDelta* m_pDelta;
	};

	class LocalNodeConfigurationHolder {
	public:
		LocalNodeConfigurationHolder();

	public:
		class LocalNodeConfigurationHolderDelta {
		public:
			explicit LocalNodeConfigurationHolderDelta(LocalNodeConfigurationHolder& configHolder);

		public:
			void Commit();

		private:
			void setBlockChainConfig(const Height& height, const std::string& m_serializedBlockChainConfig);
			void removeBlockChainConfig(const Height& height);

			void setSupportedEntityVersions(const Height& height, const std::string& m_serializedSupportedEntityVersions);
			void removeSupportedEntityVersions(const Height& height);

		private:
			std::unique_ptr<SubConfigurationHolder<BlockChainConfigTraits>::SubConfigurationHolderDelta> m_pBlockChainConfigDelta;
			std::unique_ptr<SubConfigurationHolder<SupportedEntityVersionsTraits>::SubConfigurationHolderDelta> m_pSupportedEntityVersionsDelta;
		};

	public:
		/// Extracts the resources path from the command line arguments.
		/// \a argc commmand line arguments are accessible via \a argv.
		static boost::filesystem::path GetResourcesPath(int argc, const char** argv);

		const LocalNodeConfiguration& Config() const;
		LocalNodeConfiguration& Config();
		void SetConfig(const LocalNodeConfiguration& config);
		const LocalNodeConfiguration& LoadConfig(int argc, const char** argv);

		void SetBlockChainConfig(const model::BlockChainConfiguration& config);
		void SetBlockChainConfig(const Height& height, const std::string& m_serializedBlockChainConfig);
		void RemoveBlockChainConfig(const Height& height);

		void SaveBlockChainConfigs(io::OutputStream& output);
		void LoadBlockChainConfigs(io::InputStream& input);

		void SetSupportedEntityVersions(const SupportedEntityVersions& config);
		void SetSupportedEntityVersions(const Height& height, const std::string& m_serializedSupportedEntityVersions);
		void RemoveSupportedEntityVersions(const Height& height);

		void SaveSupportedEntityVersions(io::OutputStream& output);
		void LoadSupportedEntityVersions(io::InputStream& input);

		std::unique_ptr<LocalNodeConfigurationHolderDelta> CreateDelta();

	private:
		LocalNodeConfiguration m_catapultConfig;
		SubConfigurationHolder<BlockChainConfigTraits> m_blockChainConfigHolder;
		SubConfigurationHolder<SupportedEntityVersionsTraits> m_supportedEntityVersionsHolder;
		LocalNodeConfigurationHolderDelta* m_pDelta;
	};
}}
