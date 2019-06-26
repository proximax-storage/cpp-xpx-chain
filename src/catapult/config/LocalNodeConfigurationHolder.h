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

	class LocalNodeConfigurationHolder {
	public:
		LocalNodeConfigurationHolder();

	public:
		class LocalNodeConfigurationHolderDelta {
		public:
			explicit LocalNodeConfigurationHolderDelta(LocalNodeConfigurationHolder& configHolder);
			~LocalNodeConfigurationHolderDelta();

		public:
			void Commit();

		private:
			void setBlockChainConfig(const Height& height, const std::string& m_serializedBlockChainConfig);
			void removeBlockChainConfig(const Height& height);

		private:
			LocalNodeConfigurationHolder& m_configHolder;
			std::map<Height, std::string> m_blockChainConfigs;

			friend class LocalNodeConfigurationHolder;
		};

	public:
		/// Extracts the resources path from the command line arguments.
		/// \a argc commmand line arguments are accessible via \a argv.
		static boost::filesystem::path GetResourcesPath(int argc, const char** argv);

		const LocalNodeConfiguration& Config() const;
		void SetConfig(const LocalNodeConfiguration& config);
		const LocalNodeConfiguration& LoadConfig(int argc, const char** argv);

		void SetBlockChainConfig(const model::BlockChainConfiguration& config);
		void SetBlockChainConfig(const Height& height, const std::string& m_serializedBlockChainConfig);
		void RemoveBlockChainConfig(const Height& height);

		void SaveBlockChainConfigs(io::OutputStream& output);
		void LoadBlockChainConfigs(io::InputStream& input);

		std::unique_ptr<LocalNodeConfigurationHolderDelta> CreateDelta();

	private:
		void setInitialBlockChainConfig(const std::string& serializedBlockChainConfig);
		void update();

		friend class LocalNodeConfigurationHolderDelta;

	private:
		LocalNodeConfiguration m_currentLocalNodeConfig;
		std::map<Height, std::string> m_blockChainConfigs;
		LocalNodeConfigurationHolderDelta* m_pDelta;
	};
}}
