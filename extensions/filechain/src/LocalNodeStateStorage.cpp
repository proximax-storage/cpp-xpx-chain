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

#include "catapult/cache/CacheStorageAdapter.h"
#include "catapult/cache/CatapultCache.h"
#include "catapult/cache/SupplementalData.h"
#include "catapult/cache/SupplementalDataStorage.h"
#include "catapult/config/UserConfiguration.h"
#include "catapult/config/LocalNodeConfiguration.h"
#include "catapult/io/BufferedFileStream.h"
#include "catapult/io/FileLock.h"
#include "catapult/utils/StackLogger.h"
#include "LocalNodeStateStorage.h"
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>

namespace catapult { namespace filechain {

	namespace {
		constexpr size_t Default_Loader_Batch_Size = 100'000;
		constexpr auto Supplemental_Data_Filename = "supplemental.dat";
		constexpr auto State_Lock_Filename = "state.lock";

		std::string GetStatePath(const std::string& baseDirectory, const std::string& filename) {
			boost::filesystem::path path = baseDirectory;
			path /= "state";
			if (!boost::filesystem::exists(path))
				boost::filesystem::create_directory(path);

			path /= filename;
			return path.generic_string();
		}

		void LoadCache(const std::string& baseDirectory, const std::string& filename, cache::CacheStorage& cacheStorage) {
			auto path = GetStatePath(baseDirectory, filename);
			io::BufferedInputFileStream file(io::RawFile(path.c_str(), io::OpenMode::Read_Only));
			cacheStorage.loadAll(file, Default_Loader_Batch_Size);
		}

		void SaveCache(const std::string& baseDirectory, const std::string& filename, const cache::CacheStorage& cacheStorage) {
			auto path = GetStatePath(baseDirectory, filename);
			io::BufferedOutputFileStream file(io::RawFile(path.c_str(), io::OpenMode::Read_Write));
			cacheStorage.saveAll(file);
		}

		bool HasSupplementalData(const std::string& baseDirectory) {
			auto path = GetStatePath(baseDirectory, Supplemental_Data_Filename);
			return boost::filesystem::exists(path);
		}

		std::string GetStorageFilename(const cache::CacheStorage& storage) {
			return storage.name() + ".dat";
		}
	}

	bool LoadState(const extensions::LocalNodeStateRef& stateRef, cache::SupplementalData& supplementalData) {
		std::string dataDirectory = stateRef.Config.User.DataDirectory;
		auto lockFilePath = GetStatePath(dataDirectory, State_Lock_Filename);
		io::FileLock stateLock(lockFilePath);
		if (!stateLock.try_lock()) {
			CATAPULT_LOG(warning) << "could not acquire state lock (" << lockFilePath << ") aborting load of state";
			return false;
		}

		if (!HasSupplementalData(dataDirectory))
			return false;

		utils::StackLogger stopwatch("load state", utils::LogLevel::Warning);

		for (const auto& pStorage : stateRef.Cache.storages())
			LoadCache(dataDirectory, GetStorageFilename(*pStorage), *pStorage);

		{
			auto path = GetStatePath(dataDirectory, Supplemental_Data_Filename);
			io::BufferedInputFileStream file(io::RawFile(path.c_str(), io::OpenMode::Read_Only));
			cache::LoadSupplementalData(file, supplementalData);
		}

		auto cacheDelta = stateRef.Cache.createDelta();
		stateRef.Cache.commit(supplementalData.ChainHeight);

		return true;
	}

	namespace {
		bool TryRemoveLockFile(const std::string& lockFilePath) {
			boost::system::error_code ignored_ec;
			auto isLockFilePresent = boost::filesystem::exists(lockFilePath);
			auto isLockFileRemoved = boost::filesystem::remove(lockFilePath, ignored_ec);
			CATAPULT_LOG(debug)
					<< "saving state with lock file " << lockFilePath
					<< " (exists = " << isLockFilePresent << ", removed = " << isLockFileRemoved << ")";
			return !boost::filesystem::exists(lockFilePath);
		}
	}

	void SaveState(const extensions::LocalNodeStateConstRef& stateRef) {
		// 1. if the previous SaveState crashed, an orphaned lock file will be present, which would have caused LoadState to be bypassed
		//    and instead triggered a rebuild of the cache by reloading all blocks
		// 2. in the current SaveState, delete any existing lock file (state is not written incrementally) and create a new one
		// 3. if successful, the lock file will be deleted and the next LoadState will load directly from the saved state
		std::string dataDirectory = stateRef.Config.User.DataDirectory;
		auto lockFilePath = GetStatePath(dataDirectory, State_Lock_Filename);
		io::FileLock stateLock(lockFilePath);
		if (TryRemoveLockFile(lockFilePath))
			stateLock.lock();
		else
			CATAPULT_LOG(warning) << "lock file could not be removed and must be removed manually";

		for (const auto& pStorage : stateRef.Cache.storages())
			SaveCache(dataDirectory, GetStorageFilename(*pStorage), *pStorage);

		{
			auto path = GetStatePath(dataDirectory, Supplemental_Data_Filename);
			io::BufferedOutputFileStream file(io::RawFile(path.c_str(), io::OpenMode::Read_Write));

			cache::SupplementalData data;
			data.ChainHeight = stateRef.Cache.createView().height();
			cache::SaveSupplementalData(data, file);
		}
	}
}}
