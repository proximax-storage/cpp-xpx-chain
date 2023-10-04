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

#include "RocksDatabase.h"
#include "RocksInclude.h"
#include "RocksPruningFilter.h"
#include "RdbPropertyNames.h"
#include "catapult/utils/HexFormatter.h"
#include "catapult/utils/PathUtils.h"
#include "catapult/utils/StackLogger.h"
#include "catapult/exceptions.h"
#include <boost/filesystem.hpp>

namespace catapult { namespace cache {

	// region RdbDataIterator
	namespace {
		bool IsProperty(rocksdb::Iterator* pIterator) {
			auto key = pIterator->key().ToString();
			return std::any_of(property_names::AllProperties.cbegin(), property_names::AllProperties.cend(), [&key](const auto& property) { return (key == property); });
		}
	}

	struct RdbDataIterator::Impl {
		rocksdb::PinnableSlice Result;
		rocksdb::Iterator* IteratorPtr = nullptr;

		void updateResult() {
			Result.Reset();
			Result.PinSelf(IteratorPtr->value());
		}

		bool isProperty() const {
			return IsProperty(IteratorPtr);
		}

		~Impl() {
			if (IteratorPtr) {
				IteratorPtr->Reset();
				delete IteratorPtr;
			}
		}
	};

	RdbDataIterator::RdbDataIterator(StorageStrategy storageStrategy)
			: m_pImpl(StorageStrategy::Allocate == storageStrategy ? std::make_shared<Impl>() : nullptr)
			, m_isFound(false)
	{}

	RdbDataIterator::RdbDataIterator() : RdbDataIterator(StorageStrategy::Allocate)
	{}

	RdbDataIterator::~RdbDataIterator() = default;

	RdbDataIterator::RdbDataIterator(RdbDataIterator&&) = default;

	RdbDataIterator::RdbDataIterator(const RdbDataIterator&) noexcept = default;

	RdbDataIterator& RdbDataIterator::operator=(RdbDataIterator&&) = default;

	RdbDataIterator RdbDataIterator::End() {
		return RdbDataIterator(StorageStrategy::Do_Not_Allocate);
	}

	bool RdbDataIterator::operator==(const RdbDataIterator& rhs) const {
		if (!m_isFound) {
			return !rhs.m_isFound;
		}

		if (m_pImpl->IteratorPtr)
			return m_pImpl == rhs.m_pImpl;

		return true;
	}

	RdbDataIterator RdbDataIterator::next() const {
		if (iterable()) {
			m_pImpl->IteratorPtr->Next();
			auto status = m_pImpl->IteratorPtr->status();
			if (status.ok()) {
				if (m_pImpl->IteratorPtr->Valid()) {
					if (m_pImpl->isProperty())
						return next();

					m_pImpl->updateResult();
					return *this;
				} else {
					return End();
				}
			}
		}

		return End();
	}

	bool RdbDataIterator::iterable() const{
		return m_isFound && m_pImpl->IteratorPtr;
	}

	RdbDataIterator RdbDataIterator::prev() const {
		if (iterable()) {
			m_pImpl->IteratorPtr->Prev();
			auto status = m_pImpl->IteratorPtr->status();
			if (status.ok()) {
				if (m_pImpl->IteratorPtr->Valid()) {
					if (m_pImpl->isProperty())
						return prev();

					m_pImpl->updateResult();
					return *this;
				} else {
					return End();
				}
			}
		}

		return End();
	}

	bool RdbDataIterator::operator!=(const RdbDataIterator& rhs) const {
		return !(*this == rhs);
	}

	rocksdb::PinnableSlice& RdbDataIterator::storage() const {
		return m_pImpl->Result;
	}

	void RdbDataIterator::setFound(bool found) {
		m_isFound = found;
	}

	void RdbDataIterator::setIterator(rocksdb::Iterator* pIterator) {
		setFound(true);
		m_pImpl->IteratorPtr = pIterator;
		m_pImpl->updateResult();
	}

	RawBuffer RdbDataIterator::buffer() const {
		return { reinterpret_cast<const uint8_t*>(storage().data()), storage().size() };
	}

	// endregion

	// region RocksDatabaseSettings

	RocksDatabaseSettings::RocksDatabaseSettings() : PruningMode(FilterPruningMode::Disabled)
	{}

	RocksDatabaseSettings::RocksDatabaseSettings(
			const std::string& databaseDirectory,
			const std::vector<std::string>& columnFamilyNames,
			utils::FileSize maxDatabaseWriteBatchSize,
			FilterPruningMode pruningMode)
			: DatabaseDirectory(databaseDirectory)
			, ColumnFamilyNames(columnFamilyNames)
			, MaxDatabaseWriteBatchSize(maxDatabaseWriteBatchSize)
			, PruningMode(pruningMode)
	{}

	// endregion

	RocksDatabase::RocksDatabase() = default;

	RocksDatabase::RocksDatabase(const RocksDatabaseSettings& settings)
			: m_settings(settings)
			, m_pruningFilter(m_settings.PruningMode)
			, m_pWriteBatch(std::make_unique<rocksdb::WriteBatch>()) {
		if (settings.ColumnFamilyNames.empty())
			CATAPULT_THROW_INVALID_ARGUMENT("missing column family names")

		if (0 != settings.MaxDatabaseWriteBatchSize.bytes() && settings.MaxDatabaseWriteBatchSize < utils::FileSize::FromKilobytes(100))
			CATAPULT_THROW_INVALID_ARGUMENT("too small setting of DatabaseWriteBatchSize")

		boost::system::error_code ec;
		boost::filesystem::create_directories(m_settings.DatabaseDirectory, ec);

		m_pruningFilter.setPruningBoundary(0);

		rocksdb::DB* pDb;
		rocksdb::Options dbOptions;
		dbOptions.create_if_missing = true;
		dbOptions.create_missing_column_families = true;

		rocksdb::ColumnFamilyOptions defaultColumnOptions;
		defaultColumnOptions.compaction_filter = m_pruningFilter.compactionFilter();

		std::vector<rocksdb::ColumnFamilyDescriptor> columnFamilies;
		for (const auto& columnFamilyName : settings.ColumnFamilyNames)
			columnFamilies.emplace_back(rocksdb::ColumnFamilyDescriptor(columnFamilyName, defaultColumnOptions));

		auto status = rocksdb::DB::Open(dbOptions, m_settings.DatabaseDirectory, columnFamilies, &m_handles, &pDb);
		m_pDb.reset(pDb);
		if (!status.ok())
			CATAPULT_THROW_RUNTIME_ERROR_2("couldn't open database", m_settings.DatabaseDirectory, status.ToString());
	}

	RocksDatabase::~RocksDatabase() {
		for (auto* pHandle : m_handles)
			m_pDb->DestroyColumnFamilyHandle(pHandle);
	}

	const std::vector<std::string>& RocksDatabase::columnFamilyNames() const {
		return m_settings.ColumnFamilyNames;
	}

	bool RocksDatabase::canPrune() const {
		return FilterPruningMode::Enabled == m_settings.PruningMode;
	}

	namespace {
		[[noreturn]]
		void ThrowError(const std::string& message, const std::string& columnName, const rocksdb::Slice& key) {
			CATAPULT_THROW_RUNTIME_ERROR_2(message.c_str(), columnName, utils::HexFormat(key.data(), key.data() + key.size()));
		}
	}

#define CATAPULT_THROW_DB_KEY_ERROR(message) \
	ThrowError(std::string(message) + " " + status.ToString() + " (column, key)", m_settings.ColumnFamilyNames[columnId], key)

	void RocksDatabase::get(size_t columnId, const rocksdb::Slice& key, RdbDataIterator& result) {
		if (!m_pDb)
			CATAPULT_THROW_INVALID_ARGUMENT("RocksDatabase has not been initialized");

		auto status = m_pDb->Get(rocksdb::ReadOptions(), m_handles[columnId], key, &result.storage());
		result.setFound(status.ok());

		if (status.ok())
			return;

		if (!status.IsNotFound())
			CATAPULT_THROW_DB_KEY_ERROR("could not retrieve value for get");
	}

	void RocksDatabase::getIteratorAtStart(size_t columnId, RdbDataIterator& result) {
		if (!m_pDb)
			CATAPULT_THROW_INVALID_ARGUMENT("RocksDatabase has not been initialized");

		auto iterator = m_pDb->NewIterator(rocksdb::ReadOptions(), m_handles[columnId]);

		iterator->SeekToFirst();

		auto status = iterator->status();
		while (status.ok() && iterator->Valid()) {
			if (IsProperty(iterator)) {
				iterator->Next();
				status = iterator->status();
			} else {
				result.setIterator(iterator);
				return;
			}
		}

		result.setFound(false);
		iterator->Reset();
		delete iterator;
	}

	void RocksDatabase::getLowerOrEqual(size_t columnId, const rocksdb::Slice& key, RdbDataIterator& result) {
		if (!m_pDb)
			CATAPULT_THROW_INVALID_ARGUMENT("RocksDatabase has not been initialized");

		auto iterator = m_pDb->NewIterator(rocksdb::ReadOptions(), m_handles[columnId]);

		iterator->SeekForPrev(key);

		auto status = iterator->status();
		if (status.ok()) {
			if (iterator->Valid()) {
				result.storage().PinSelf(iterator->value());
				result.setFound(true);
			} else {
				result.setFound(false);
			}
		}

		iterator->Reset();
		delete iterator;

		if (status.ok())
			return;

		if (status.IsNotFound())
			CATAPULT_THROW_DB_KEY_ERROR("could not retrieve value for getLowerOrEqual");
	}

	void RocksDatabase::put(size_t columnId, const rocksdb::Slice& key, const std::string& value) {
		if (!m_pDb)
			CATAPULT_THROW_INVALID_ARGUMENT("RocksDatabase has not been initialized");

		auto status = m_pWriteBatch->Put(m_handles[columnId], key, value);
		if (!status.ok())
			CATAPULT_THROW_DB_KEY_ERROR("could not add put operation to batch");

		saveIfBatchFull();
	}

	void RocksDatabase::del(size_t columnId, const rocksdb::Slice& key) {
		if (!m_pDb)
			CATAPULT_THROW_INVALID_ARGUMENT("RocksDatabase has not been initialized");

		// note: using SingleDelete can result in undefined result if value has ever been overwritten
		// that can't be guaranteed, so Delete is used instead
		auto status = m_pWriteBatch->Delete(m_handles[columnId], key);
		if (!status.ok())
			CATAPULT_THROW_DB_KEY_ERROR("could not add delete operation to batch");

		saveIfBatchFull();
	}

	size_t RocksDatabase::prune(size_t columnId, uint64_t boundary) {
		if (!m_pruningFilter.compactionFilter())
			return 0;

		m_pruningFilter.setPruningBoundary(boundary);
		m_pDb->CompactRange({}, m_handles[columnId], nullptr, nullptr);
		return m_pruningFilter.numRemoved();
	}

	void RocksDatabase::flush() {
		if (0 == m_pWriteBatch->GetDataSize())
			return;

		rocksdb::WriteOptions writeOptions;
		writeOptions.sync = true;

		auto directory = m_settings.DatabaseDirectory + "/";
		utils::SlowOperationLogger logger(utils::ExtractDirectoryName(directory.c_str()).pData, utils::LogLevel::Warning);
		auto status = m_pDb->Write(writeOptions, m_pWriteBatch.get());
		if (!status.ok())
			CATAPULT_THROW_RUNTIME_ERROR_1("could not store batch in db", status.ToString());

		m_pWriteBatch->Clear();
	}

	void RocksDatabase::saveIfBatchFull() {
		if (m_pWriteBatch->GetDataSize() < m_settings.MaxDatabaseWriteBatchSize.bytes())
			return;

		flush();
	}
}}
