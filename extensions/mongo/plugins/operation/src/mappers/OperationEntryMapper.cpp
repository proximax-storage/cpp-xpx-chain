/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "OperationEntryMapper.h"
#include "mongo/plugins/lock_shared/src/mappers/LockInfoMapper.h"
#include "src/catapult/functions.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	namespace {
		template<typename TArray>
		void StreamArray(bson_stream::document& builder, const std::string& name, const TArray& array) {
			auto arrayBuilder = builder << name << bson_stream::open_array;
			for (const auto& item : array) {
				arrayBuilder << ToBinary(item);
			}
			arrayBuilder << bson_stream::close_array;
		}

		template<typename TValue>
		void ReadArray(const bsoncxx::array::view& dbArray, consumer<const TValue&> inserter) {
			for (const auto& dbItem : dbArray) {
				TValue value;
				DbBinaryToModelArray(value, dbItem.get_binary());
				inserter(value);
			}
		}
	}

	namespace {
		class OperationEntryMapperTraits {
		public:
			using LockInfoType = state::OperationEntry;
			static constexpr VersionType Version = 2;
			static constexpr char IdName[] = "operation";

		public:
			static void StreamLockInfo(bson_stream::document& builder, const state::OperationEntry& entry) {
				builder << "token" << ToBinary(entry.OperationToken);
				StreamArray(builder, "executors", entry.Executors);
				StreamArray(builder, "transactionHashes", entry.TransactionHashes);
			}

			static void ReadLockInfo(state::OperationEntry& entry, const bsoncxx::document::element dbOperationEntry) {
				DbBinaryToModelArray(entry.OperationToken, dbOperationEntry["token"].get_binary());
				ReadArray<Key>(dbOperationEntry["executors"].get_array().value,
					[&executors = entry.Executors](const auto& executor) { executors.insert(executor); });
				ReadArray<Hash256>(dbOperationEntry["transactionHashes"].get_array().value,
					[&hashes = entry.TransactionHashes](const auto& hash) { hashes.push_back(hash); });
			}
		};
	}

	bsoncxx::document::value ToDbModel(const state::OperationEntry& entry, const Address& accountAddress) {
		return LockInfoMapper<OperationEntryMapperTraits>::ToDbModel(entry, accountAddress);
	}

	void ToLockInfo(const bsoncxx::document::view& document, state::OperationEntry& entry) {
		LockInfoMapper<OperationEntryMapperTraits>::ToLockInfo(document, entry);
	}
}}}
