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

#include "tools/ToolMain.h"
#include "BlockGenerator.h"
#include "BlockSaver.h"
#include "NemesisConfigurationLoader.h"
#include "NemesisExecutionHasher.h"
#include "tools/ToolConfigurationUtils.h"
#include "catapult/io/RawFile.h"
#include "catapult/crypto/KeyPair.h"
#include "catapult/plugins/PluginModule.h"
#include "catapult/extensions/LocalNodeStateFileStorage.h"
#include "catapult/io/BufferedFileStream.h"
#include "catapult/extensions/LocalNodeStateFileStorage.h"
#include "catapult/cache/SupplementalDataStorage.h"
#include "catapult/extensions/PluginUtils.h"
#include "catapult/plugins/PluginLoader.h"

namespace catapult { namespace tools { namespace nemgen {

	namespace {

		constexpr size_t Default_Loader_Batch_Size = 100'000;
		constexpr auto Supplemental_Data_Filename = "supplemental.dat";

		std::unique_ptr<utils::LogFilter> CreateLogFilter(const config::BasicLoggerConfiguration& config) {
			auto pFilter = std::make_unique<utils::LogFilter>(config.Level);
			for (const auto& pair : config.ComponentLevels)
				pFilter->setLevel(pair.first.c_str(), pair.second);

			return pFilter;
		}

		std::shared_ptr<void> SetupLogging(const config::LoggingConfiguration& config) {
			auto pBootstrapper = std::make_shared<utils::LoggingBootstrapper>();
			pBootstrapper->addConsoleLogger(config::GetConsoleLoggerOptions(config.Console), *CreateLogFilter(config.Console));
			pBootstrapper->addFileLogger(config::GetFileLoggerOptions(config.File), *CreateLogFilter(config.File));
			return std::move(pBootstrapper);
		}

		std::string GetStorageFilename(const cache::CacheStorage& storage) {
			return storage.name() + ".dat";
		}

		void WriteToFile(const std::string& filePath, const std::string& content) {
			io::RawFile file(filePath, io::OpenMode::Read_Write);
			file.write({ reinterpret_cast<const uint8_t*>(content.data()), content.size() });
		}

		io::BufferedInputFileStream OpenInputStream(const config::CatapultDirectory& directory, const std::string& filename) {
			// TODO: remove this temporary workaround after mainnet upgrade to 0.8.0
			auto filepath = directory.file(filename);
			if (!boost::filesystem::exists(filepath))
				return io::BufferedInputFileStream(io::RawFile(filepath, io::OpenMode::Read_Write));

			return io::BufferedInputFileStream(io::RawFile(filepath, io::OpenMode::Read_Only));
		}

		bool LoadStateFromDirectory(
				const config::CatapultDirectory& directory,
				cache::CatapultCache& cache,
				cache::SupplementalData& supplementalData) {
			if (!extensions::HasSerializedState(directory))
				return false;

			// 1. load cache data
			for (const auto& pStorage : cache.storages()) {
				auto inputStream = OpenInputStream(directory, GetStorageFilename(*pStorage));
				pStorage->loadAll(inputStream, Default_Loader_Batch_Size);
			}

			// 2. load supplemental data
			Height chainHeight;
			{
				auto inputStream = OpenInputStream(directory, Supplemental_Data_Filename);
				cache::LoadSupplementalData(inputStream, supplementalData, chainHeight);
			}

			// 3. commit changes
			auto cacheDelta = cache.createDelta();
			cache.commit(chainHeight);
			return true;
		}

		class NemGenTool : public Tool {
		public:
			std::string name() const override {
				return "Nemesis Block Generator Tool";
			}

			void prepareOptions(OptionsBuilder& optionsBuilder, OptionsPositional&) override {
				optionsBuilder("resources,r",
						OptionsValue<std::string>(m_resourcesPath)->default_value(".."),
						"the path to the resources directory");

				optionsBuilder("nemesisProperties,p",
						OptionsValue<std::string>(m_nemesisPropertiesFilePath),
						"the path to the nemesis properties file");

				optionsBuilder("summary,s",
						OptionsValue<std::string>(m_summaryFilePath),
						"the path to summary output file (default: <bindir>/summary.txt)");

				optionsBuilder("no-summary,n",
						OptionsSwitch(),
						"don't generate summary file");

				optionsBuilder("useTemporaryCacheDatabase,t",
						OptionsSwitch(),
						"true if a temporary cache database should be created and destroyed");

				optionsBuilder("reconstructor,rn",
							   OptionsSwitch(),
							   "true if nemesis block should be based on the existing chain state");
			}

			int run(const Options& options) override {

				bool is_reconstructor = options.find("reconstructor") != options.end() ;
										// 1. load config
				auto nemesisConfig = LoadNemesisConfiguration(m_nemesisPropertiesFilePath);
				if (!LogAndValidateNemesisConfiguration(nemesisConfig))
					return -1;

				auto signer = crypto::KeyPair::FromString(nemesisConfig.NemesisSignerPrivateKey);
				auto config = LoadConfiguration(nemesisConfig.ResourcesPath, false);


				if(is_reconstructor) {

					auto databaseCleanupMode = options["useTemporaryCacheDatabase"].as<bool>()
													   ? CacheDatabaseCleanupMode::Purge
													   : CacheDatabaseCleanupMode::None;
					auto rcConfig = LoadConfiguration(nemesisConfig.ReconstructionResourcesPath, false);
					auto pRCConfigHolder = std::make_shared<config::BlockchainConfigurationHolder>(rcConfig);

					auto dataDirectory = config::CatapultDataDirectoryPreparer::Prepare(rcConfig.User.DataDirectory);
					auto pLoggingGuard = SetupLogging(config.Logging);
					auto pluginManager = std::make_unique<plugins::PluginManager>(pRCConfigHolder, extensions::CreateStorageConfiguration(pRCConfigHolder->Config()));
					auto keyPair = crypto::KeyPair::FromString(pRCConfigHolder->Config().User.BootKey);
					std::vector<plugins::PluginModule>  pluginModules;
					std::string prefix = "catapult.";
					for (const auto& name : { "catapult.coresystem", PLUGIN_NAME(signature) })
						plugins::LoadPluginByName(*pluginManager, pluginModules, pluginManager->configHolder()->Config().User.PluginsDirectory, name);

					for (const auto& pair : pluginManager->config().Plugins)
						plugins::LoadPluginByName(*pluginManager, pluginModules, pluginManager->configHolder()->Config().User.PluginsDirectory, pair.first);

					auto cache =  pluginManager->createCache();
					pluginManager->configHolder()->SetCache(&cache);

					if (pluginManager->isStorageStateSet())
						pluginManager->storageState().setCache(&cache);

					auto initializers = pluginManager->createPluginInitializer();
					initializers(const_cast<model::NetworkConfiguration&>(pluginManager->configHolder()->Config().Network));
					pluginManager->configHolder()->SetPluginInitializer(std::move(initializers));

					cache::SupplementalData supplementalData;

					if (!LoadStateFromDirectory(dataDirectory.dir("state"), cache, supplementalData))
						CATAPULT_THROW_RUNTIME_ERROR("State cannot be loaded.");

					auto cacheDelta = cache.createDelta();
					auto broker = StateBroker(cacheDelta, pluginManager->configHolder(), nemesisConfig.NemesisSignerPrivateKey);
					NemesisTransactions transactions(signer, nemesisConfig, pluginManager->transactionRegistry());
					auto pBlock = ReconstructNemesisBlock(nemesisConfig, transactions, cache, *pluginManager, pluginManager->configHolder(), broker);
					auto blockElement = ReconstructNemesisBlockElement(nemesisConfig, *pBlock, transactions);// block element without transactions
					// Load secondary base config holder
					pluginManager.reset();
					pRCConfigHolder.reset();
					auto config = LoadConfiguration(nemesisConfig.ResourcesPath, false, broker.GetActiveConfiguration());
					auto pConfigHolder = std::make_shared<config::BlockchainConfigurationHolder>(config);
					auto executionHashesDescriptor = CalculateAndLogNemesisExecutionHashes(nemesisConfig, blockElement, pConfigHolder, databaseCleanupMode, &transactions);

					if (!options["no-summary"].as<bool>()) {
						if (m_summaryFilePath.empty())
							m_summaryFilePath = nemesisConfig.BinDirectory + "/summary.txt";

						WriteToFile(m_summaryFilePath, executionHashesDescriptor.Summary);
					}

					blockElement.EntityHash = UpdateNemesisBlock(nemesisConfig, *pBlock, executionHashesDescriptor);
					SaveNemesisBlockElementWithSpooling(blockElement, nemesisConfig, transactions);
					broker.Dump(nemesisConfig.AccountEquivalenceFile);
					return 0;
				}

				auto pConfigHolder = std::make_shared<config::BlockchainConfigurationHolder>(config);

				auto databaseCleanupMode = options["useTemporaryCacheDatabase"].as<bool>()
												   ? CacheDatabaseCleanupMode::Purge
												   : CacheDatabaseCleanupMode::None;
				// 2. create the nemesis block element

				auto pBlock = CreateNemesisBlock(nemesisConfig, m_resourcesPath);
				auto blockElement = CreateNemesisBlockElement(nemesisConfig, *pBlock);
				auto executionHashesDescriptor = CalculateAndLogNemesisExecutionHashes(nemesisConfig, blockElement, pConfigHolder, databaseCleanupMode, nullptr);
				if (!options["no-summary"].as<bool>()) {
					if (m_summaryFilePath.empty())
						m_summaryFilePath = nemesisConfig.BinDirectory + "/summary.txt";

					WriteToFile(m_summaryFilePath, executionHashesDescriptor.Summary);
				}

				// 3. update block with result of execution
				CATAPULT_LOG(info) << "*** Nemesis Summary ***" << std::endl << executionHashesDescriptor.Summary;
				blockElement.EntityHash = UpdateNemesisBlock(nemesisConfig, *pBlock, executionHashesDescriptor);

				// 4. save the nemesis block element
				SaveNemesisBlockElement(blockElement, nemesisConfig);
				return 0;
			}

		private:
			std::string m_resourcesPath;
			std::string m_nemesisPropertiesFilePath;
			std::string m_summaryFilePath;
		};
	}
}}}

int main(int argc, const char** argv) {
	catapult::tools::nemgen::NemGenTool nemGenTool;
	return catapult::tools::ToolMain(argc, argv, nemGenTool);
}
