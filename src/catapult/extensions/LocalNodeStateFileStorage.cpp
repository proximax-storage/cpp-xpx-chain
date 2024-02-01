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

#include "LocalNodeStateFileStorage.h"
#include "LocalNodeChainScore.h"
#include "LocalNodeStateRef.h"
#include "NemesisBlockLoader.h"
#include "catapult/cache/CacheStorage.h"
#include "catapult/cache/CatapultCache.h"
#include "catapult/cache/SupplementalDataStorage.h"
#include "catapult/config/CatapultDataDirectory.h"
#include "catapult/config/NodeConfiguration.h"
#include "catapult/consumers/BlockChainSyncHandlers.h"
#include "catapult/io/BlockStorageCache.h"
#include "catapult/io/BufferedFileStream.h"
#include "catapult/io/FilesystemUtils.h"
#include "catapult/io/IndexFile.h"
#include "catapult/plugins/PluginManager.h"
#include "catapult/utils/StackLogger.h"
#include "plugins/txes/config/src/cache/NetworkConfigCache.h"

namespace catapult { namespace extensions {

	// region constants + utils

	namespace {
		constexpr size_t Default_Loader_Batch_Size = 100'000;
		constexpr auto Supplemental_Data_Filename = "supplemental.dat";
		constexpr auto Network_Config_Filename = "activeconfig.dat";

		std::string GetStorageFilename(const cache::CacheStorage& storage) {
			return storage.name() + ".dat";
		}
	}

	// endregion

	// region HasSerializedState

	bool HasSerializedState(const config::CatapultDirectory& directory) {
		return boost::filesystem::exists(directory.file(Supplemental_Data_Filename));
	}

	bool HasActiveNetworkConfig(const config::CatapultDirectory& directory) {
		return boost::filesystem::exists(directory.file(Network_Config_Filename));
	}

	// endregion

	// region LoadStateFromDirectory

	namespace {
		io::BufferedInputFileStream OpenInputStream(const config::CatapultDirectory& directory, const std::string& filename) {
			return io::BufferedInputFileStream(io::RawFile(directory.file(filename), io::OpenMode::Read_Only));
		}

		bool LoadStateFromDirectory(
				const config::CatapultDirectory& directory,
				cache::CatapultCache& cache,
				cache::SupplementalData& supplementalData) {
			if (!HasSerializedState(directory))
				return false;

			// 1. load cache data
			utils::StackLogger stopwatch("load state", utils::LogLevel::Warning);
			for (const auto& pStorage : cache.storages()) {
				auto storageName = GetStorageFilename(*pStorage);
				auto inputStream = OpenInputStream(directory, storageName);
				pStorage->loadAll(inputStream, Default_Loader_Batch_Size);
				CATAPULT_LOG(debug) << "loading cache storage: " + storageName;
			}

			// 2. load supplemental data
			Height chainHeight;
			{
				CATAPULT_LOG(debug) << "loading supplemental data state";
				auto inputStream = OpenInputStream(directory, Supplemental_Data_Filename);
				cache::LoadSupplementalData(inputStream, supplementalData, chainHeight);
				CATAPULT_LOG(debug) << "loaded supplemental data state";
			}

			// 3. commit changes
			auto cacheDelta = cache.createDelta();
			cache.commit(chainHeight);
			return true;
		}
	}

	const model::NetworkConfiguration LoadActiveNetworkConfig(const config::CatapultDirectory& directory, const config::ImmutableConfiguration& immutableConfig) {
		auto networkConfig = LoadActiveNetworkConfigString(directory);
		auto stream = std::istringstream(networkConfig);
		return model::NetworkConfiguration::LoadFromBag(utils::ConfigurationBag::FromStream(stream), immutableConfig);
	}
	const std::string LoadActiveNetworkConfigString(const config::CatapultDirectory& directory) {
		auto inputStream = OpenInputStream(directory, Network_Config_Filename);
		auto size = io::Read16(inputStream);
		std::string networkConfig;
		networkConfig.resize(size);
		io::Read(inputStream, MutableRawBuffer((uint8_t*)networkConfig.data(), networkConfig.size()));
		return networkConfig;
	}

	StateHeights LoadStateFromDirectory(
			const config::CatapultDirectory& directory,
			const LocalNodeStateRef& stateRef,
			const plugins::PluginManager& pluginManager) {
		cache::SupplementalData supplementalData;
		if (LoadStateFromDirectory(directory, stateRef.Cache, supplementalData)) {
			stateRef.State = supplementalData.State;
			stateRef.Score += supplementalData.ChainScore;
			CATAPULT_LOG(debug) << "sucessfully loaded state from directory";
		} else {
			auto cacheDelta = stateRef.Cache.createDelta();
			NemesisBlockLoader loader(cacheDelta, pluginManager, pluginManager.createObserver());
			loader.executeAndCommit(stateRef, StateHashVerification::Enabled);
			stateRef.Score += model::ChainScore(1); // set chain score to 1 after processing nemesis
			CATAPULT_LOG(debug) << "no state. loaded nemesis.";
		}

		StateHeights heights;
		heights.Cache = stateRef.Cache.height();
		heights.Storage = stateRef.Storage.view().chainHeight();
		return heights;
	}

	const Height LoadLatestHeightFromDirectory(const config::CatapultDirectory& directory) {
		auto inputStream = OpenInputStream(directory, Supplemental_Data_Filename);
		cache::SupplementalData data;
		Height rtHeight;
		cache::LoadSupplementalData(inputStream, data, rtHeight);
		return rtHeight;
	}

	// endregion


	// region LocalNodeStateSerializer

	namespace {
		io::BufferedOutputFileStream OpenOutputStream(const config::CatapultDirectory& directory, const std::string& filename) {
			return io::BufferedOutputFileStream(io::RawFile(directory.file(filename), io::OpenMode::Read_Write));
		}

		void SaveActiveNetworkConfig(const std::string& networkConfig, const config::CatapultDirectory& directory, io::OutputStream& output) {
			io::Write16(output, networkConfig.size());
			io::Write(output, RawBuffer((uint8_t*)networkConfig.data(), networkConfig.size()));
			output.flush();
		}

		void SaveStateToDirectory(
				const config::CatapultDirectory& directory,
				const std::vector<std::unique_ptr<const cache::CacheStorage>>& cacheStorages,
				const state::CatapultState& state,

				const model::ChainScore& score,
				Height height,
				const std::optional<state::NetworkConfigEntry>& config,
				const consumer<const cache::CacheStorage&, io::OutputStream&>& save) {
			// 1. create directory if required
			CATAPULT_CLEANUP_LOG(info, "Saving state to directory. Verify if directory exists.");
			if (!boost::filesystem::exists(directory.path()))
				CATAPULT_CLEANUP_LOG(info, "Creating Directory..");
				boost::filesystem::create_directory(directory.path());

			// 2. save cache data
			CATAPULT_CLEANUP_LOG(info, "Saving cache data for storages.");
			for (const auto& pStorage : cacheStorages) {
				CATAPULT_CLEANUP_LOG(info, "Saving cache data for storage: "+pStorage.get()->name());
				auto outputStream = OpenOutputStream(directory, GetStorageFilename(*pStorage));
				save(*pStorage, outputStream);
				CATAPULT_CLEANUP_LOG(info, "Saved cache data for storage: "+pStorage.get()->name());
			}

			// 3. save supplemental data
			cache::SupplementalData supplementalData{ state, score };
			CATAPULT_CLEANUP_LOG(info, "Saving supplemental data.");
			auto outputStream = OpenOutputStream(directory, Supplemental_Data_Filename);
			cache::SaveSupplementalData(supplementalData, height, outputStream);
			CATAPULT_CLEANUP_LOG(info, "Saved supplemental data.");

			// 4. save active or to be active network configuration

			if(config.has_value()) {
				auto outputStream = OpenOutputStream(directory, Network_Config_Filename);
				SaveActiveNetworkConfig(config->networkConfig(), directory, outputStream);
				CATAPULT_CLEANUP_LOG(info, "Saved active network config data.");
			}

		}
	}

	LocalNodeStateSerializer::LocalNodeStateSerializer(const config::CatapultDirectory& directory) : m_directory(directory)
	{}

	void LocalNodeStateSerializer::save(
			const cache::CatapultCache& cache,
			const state::CatapultState& state,
			const model::ChainScore& score) const {
		auto cacheStorages = cache.storages();
		auto cacheView = cache.createView();
		auto height = cacheView.height();
		std::optional<state::NetworkConfigEntry> config;
		if(!cacheStorages.empty()) {
			auto& networkConfigCache = cache.sub<cache::NetworkConfigCache>();
			auto networkConfigCacheView = networkConfigCache.createView(height+Height(1));
			auto nextBlockConfig = networkConfigCacheView->FindConfigHeightAt(height+Height(1));
			if(nextBlockConfig != Height(0)) {
				auto configIter = networkConfigCacheView->find(nextBlockConfig);
				config = std::make_optional(configIter.get());
			}
		}
		SaveStateToDirectory(m_directory, cacheStorages, state, score, height, config, [&cacheView](const auto& storage, auto& outputStream) {
			storage.saveAll(cacheView, outputStream);
		});
	}

	void LocalNodeStateSerializer::save(
			const cache::CatapultCache& cache,
			const cache::CatapultCacheDelta& cacheDelta,
			const std::vector<std::unique_ptr<const cache::CacheStorage>>& cacheStorages,
			const state::CatapultState& state,
			const model::ChainScore& score,
			Height height) const {
		std::optional<state::NetworkConfigEntry> config;
		// To allow for simpler tests.
		if(!cacheStorages.empty()) {
			auto& networkConfigCache = cache.sub<cache::NetworkConfigCache>();
			auto networkConfigCacheView = networkConfigCache.createView(height+Height(1));
			auto nextBlockConfig = networkConfigCacheView->FindConfigHeightAt(height+Height(1));
			if(nextBlockConfig != Height(0)) {
				auto configIter = networkConfigCacheView->find(nextBlockConfig);
				config = std::make_optional(configIter.get());
			}
		}
		SaveStateToDirectory(m_directory, cacheStorages, state, score, height, config, [&cacheDelta](const auto& storage, auto& outputStream) {
			storage.saveSummary(cacheDelta, outputStream);
		});
	}

	void LocalNodeStateSerializer::moveTo(const config::CatapultDirectory& destinationDirectory) {
		io::PurgeDirectory(destinationDirectory.str());
		boost::filesystem::remove(destinationDirectory.path());
		boost::filesystem::rename(m_directory.path(), destinationDirectory.path());
	}

	// endregion

	// region SaveStateToDirectoryWithCheckpointing

	namespace {
		void SetCommitStep(const config::CatapultDataDirectory& dataDirectory, consumers::CommitOperationStep step) {
			io::IndexFile(dataDirectory.rootDir().file("commit_step.dat")).set(utils::to_underlying_type(step));
		}
	}

	void SaveStateToDirectoryWithCheckpointing(
			const config::CatapultDataDirectory& dataDirectory,
			const config::NodeConfiguration& nodeConfig,
			const cache::CatapultCache& cache,
			const state::CatapultState& state,
			const model::ChainScore& score) {
		SetCommitStep(dataDirectory, consumers::CommitOperationStep::Blocks_Written);

		LocalNodeStateSerializer serializer(dataDirectory.dir("state.tmp"));

		if (nodeConfig.ShouldUseCacheDatabaseStorage) {
			auto storages = cache.storages();
			auto height = cache.height();

			auto cacheDetachableDelta = cache.createDetachableDelta();
			auto cacheDetachedDelta = cacheDetachableDelta.detach();
			auto pCacheDelta = cacheDetachedDelta.tryLock();

			serializer.save(cache, *pCacheDelta, storages, state, score, height);
		} else {
			serializer.save(cache, state, score);
		}

		SetCommitStep(dataDirectory, consumers::CommitOperationStep::State_Written);

		serializer.moveTo(dataDirectory.dir("state"));

		SetCommitStep(dataDirectory, consumers::CommitOperationStep::All_Updated);
	}

	// endregion
}}
