/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "plugins/txes/lock_shared/tests/state/LockInfoSerializerTests.h"
#include "tests/test/OperationTestUtils.h"

namespace catapult { namespace state {

#define TEST_CLASS OperationEntrySerializerTests

	namespace {
		constexpr auto Num_Executors = 2u;
		constexpr auto Num_Transaction_Hashes = 2u;

		// region PackedOperationEntry

#pragma pack(push, 1)

		struct PackedOperationEntry : public PackedLockInfo<2> {
		public:
			explicit PackedOperationEntry(const OperationEntry& entry)
					: PackedLockInfo<2>(entry)
					, Version(1)
					, OperationToken(entry.OperationToken)
					, ExecutorCount(Num_Executors)
					, TransactionHashCount(Num_Transaction_Hashes)
					, Result(entry.Result) {
				auto i = 0u;
				for (const auto& executor : entry.Executors)
					Executors[i++] = executor;
				i = 0u;
				for (const auto& transactionHash : entry.TransactionHashes)
					TransactionHashes[i++] = transactionHash;
			}

		public:
			VersionType Version;
			Hash256 OperationToken;
			uint16_t ExecutorCount;
			std::array<Key, Num_Executors> Executors;
			uint16_t TransactionHashCount;
			std::array<Hash256, Num_Transaction_Hashes> TransactionHashes;
			model::OperationResult Result;
		};

#pragma pack(pop)

		// endregion

		struct OperationEntryStorageTraits : public test::BasicOperationTestTraits {
			using PackedValueType = PackedOperationEntry;
			using SerializerType = OperationEntrySerializer;

			static size_t ValueTypeSize() {
				return sizeof(PackedValueType);
			}
		};
	}

	DEFINE_LOCK_INFO_SERIALIZER_TESTS(OperationEntryStorageTraits)
}}
