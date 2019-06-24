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

#include "catapult/io/TransactionInfoSerializer.h"
#include "catapult/io/PodIoUtils.h"
#include "tests/test/core/AddressTestUtils.h"
#include "tests/test/core/TransactionInfoTestUtils.h"
#include "tests/test/core/TransactionTestUtils.h"
#include "tests/test/core/mocks/MockMemoryStream.h"
#include "tests/TestHarness.h"

namespace catapult { namespace io {

#define TEST_CLASS TransactionInfoSerializerTests

	// region ReadTransactionInfo

	namespace {
		void AssertCanReadTransactionInfo(uint64_t addressCount, const model::UnresolvedAddressSet& addresses) {
			// Arrange:
			auto entityHash = test::GenerateRandomByteArray<Hash256>();
			auto merkleComponentHash = test::GenerateRandomByteArray<Hash256>();
			auto pTransaction = test::GenerateRandomTransactionWithSize(140);

			std::vector<uint8_t> buffer(2 * Hash256_Size + sizeof(uint64_t) + addresses.size() * Address_Decoded_Size + 140);
			auto offset = 0u;
			std::memcpy(buffer.data() + offset, &entityHash, Hash256_Size);
			offset += Hash256_Size;

			std::memcpy(buffer.data() + offset, &merkleComponentHash, Hash256_Size);
			offset += Hash256_Size;

			std::memcpy(buffer.data() + offset, &addressCount, sizeof(uint64_t));
			offset += sizeof(uint64_t);

			for (const auto& address : addresses) {
				std::memcpy(buffer.data() + offset, &address, Address_Decoded_Size);
				offset += Address_Decoded_Size;
			}

			std::memcpy(buffer.data() + offset, pTransaction.get(), pTransaction->Size);

			// Act:
			mocks::MockMemoryStream inputStream(buffer);
			model::TransactionInfo transactionInfo;
			ReadTransactionInfo(inputStream, transactionInfo);

			// Assert:
			EXPECT_EQ(entityHash, transactionInfo.EntityHash);
			EXPECT_EQ(merkleComponentHash, transactionInfo.MerkleComponentHash);

			if (std::numeric_limits<uint64_t>::max() == addressCount) {
				EXPECT_FALSE(!!transactionInfo.OptionalExtractedAddresses);
			} else {
				ASSERT_TRUE(!!transactionInfo.OptionalExtractedAddresses);
				EXPECT_EQ(addresses, *transactionInfo.OptionalExtractedAddresses);
			}

			EXPECT_EQ(*pTransaction, *transactionInfo.pEntity);
		}
	}

	TEST(TEST_CLASS, CanReadTransactionInfoWithoutOptionalExtractedAddresses) {
		// Assert:
		AssertCanReadTransactionInfo(std::numeric_limits<size_t>::max(), {});
	}

	TEST(TEST_CLASS, CanReadTransactionInfoWithZeroExtractedAddresses) {
		// Assert:
		AssertCanReadTransactionInfo(0, {});
	}

	TEST(TEST_CLASS, CanReadTransactionInfoWithSingleExtractedAddress) {
		// Assert:
		AssertCanReadTransactionInfo(1, *test::GenerateRandomUnresolvedAddressSetPointer(1));
	}

	TEST(TEST_CLASS, CanReadTransactionInfoWithMultipleExtractedAddresses) {
		// Assert:
		AssertCanReadTransactionInfo(3, *test::GenerateRandomUnresolvedAddressSetPointer(3));
	}

	// endregion

	// region WriteTransactionInfo

	namespace {
		model::TransactionInfo CreateTransactionInfoWithSize(uint32_t entitySize) {
			auto transactionInfo = model::TransactionInfo(test::GenerateRandomTransactionWithSize(entitySize));
			test::FillWithRandomData(transactionInfo.EntityHash);
			test::FillWithRandomData(transactionInfo.MerkleComponentHash);
			return transactionInfo;
		}

		void AssertCanWriteTransactionInfo(
				uint64_t expectedAddressCount,
				uint64_t expectedAddressSize,
				const std::shared_ptr<const model::UnresolvedAddressSet>& pAddresses) {
			// Arrange:
			auto transactionInfo = CreateTransactionInfoWithSize(123);
			transactionInfo.OptionalExtractedAddresses = pAddresses;

			// Act:
			std::vector<uint8_t> buffer;
			mocks::MockMemoryStream outputStream(buffer);
			WriteTransactionInfo(outputStream, transactionInfo);

			// Assert:
			auto expectedSize = 2u * Hash256_Size + sizeof(uint64_t) + expectedAddressSize + 123;
			ASSERT_EQ(expectedSize, buffer.size());

			auto offset = 0u;
			EXPECT_EQ(transactionInfo.EntityHash, reinterpret_cast<const Hash256&>(buffer[offset]));
			offset += Hash256_Size;

			EXPECT_EQ(transactionInfo.MerkleComponentHash, reinterpret_cast<const Hash256&>(buffer[offset]));
			offset += Hash256_Size;

			ASSERT_EQ(expectedAddressCount, reinterpret_cast<uint64_t&>(buffer[offset]));
			offset += sizeof(uint64_t);

			if (0 != expectedAddressSize) {
				for (const auto& address : *transactionInfo.OptionalExtractedAddresses) {
					EXPECT_EQ(address, reinterpret_cast<const UnresolvedAddress&>(buffer[offset]))
							<< "address at offset " << offset;
					offset += Address_Decoded_Size;
				}
			}

			EXPECT_EQ(reinterpret_cast<const model::Transaction&>(buffer[offset]), *transactionInfo.pEntity);
		}
	}

	TEST(TEST_CLASS, CanWriteTransactionInfoWithoutOptionalExtractedAddresses) {
		// Assert:
		AssertCanWriteTransactionInfo(std::numeric_limits<size_t>::max(), 0, nullptr);
	}

	TEST(TEST_CLASS, CanWriteTransactionInfoWithZeroExtractedAddresses) {
		// Assert:
		AssertCanWriteTransactionInfo(0, 0, std::make_shared<model::UnresolvedAddressSet>());
	}

	TEST(TEST_CLASS, CanWriteTransactionInfoWithSingleExtractedAddress) {
		// Assert:
		AssertCanWriteTransactionInfo(1, Address_Decoded_Size, test::GenerateRandomUnresolvedAddressSetPointer(1));
	}

	TEST(TEST_CLASS, CanWriteTransactionInfoWithMultipleExtractedAddresses) {
		// Assert:
		AssertCanWriteTransactionInfo(3, 3 * Address_Decoded_Size, test::GenerateRandomUnresolvedAddressSetPointer(3));
	}

	// endregion

	// region ReadTransactionInfos

	namespace {
		std::vector<uint8_t> SerializeTransactionInfo(const model::TransactionInfo& transactionInfo) {
			std::vector<uint8_t> buffer;
			mocks::MockMemoryStream stream(buffer);

			WriteTransactionInfo(stream, transactionInfo);
			return buffer;
		}

		model::TransactionInfosSet CreateTransactionInfosSetWithOptionalAddresses(size_t count) {
			auto transactionInfos = test::CreateTransactionInfosWithOptionalAddresses(count);
			return test::CopyTransactionInfosToSet(transactionInfos);
		}
	}

	TEST(TEST_CLASS, CanReadEmptyTransactionInfos) {
		// Arrange: prepare input stream
		std::vector<uint8_t> buffer(sizeof(uint32_t), 0);
		BufferInputStreamAdapter inputStream(buffer);

		// Act:
		model::TransactionInfosSet transactionInfos;
		ReadTransactionInfos(inputStream, transactionInfos);

		// Assert:
		EXPECT_TRUE(transactionInfos.empty());
	}

	TEST(TEST_CLASS, CanReadTransactionInfos) {
		// Arrange: prepare input stream
		auto expectedTransactionInfos = CreateTransactionInfosSetWithOptionalAddresses(3);
		std::vector<uint8_t> buffer;
		mocks::MockMemoryStream outputStream(buffer);

		// use tested above WriteTransactionInfo to prepare input buffer
		Write32(outputStream, 3);
		for (const auto& transactionInfo : expectedTransactionInfos)
			outputStream.write(SerializeTransactionInfo(transactionInfo));

		BufferInputStreamAdapter inputStream(buffer);

		// Act:
		model::TransactionInfosSet transactionInfos;
		ReadTransactionInfos(inputStream, transactionInfos);

		// Assert:
		test::AssertEquivalent(expectedTransactionInfos, transactionInfos);
	}

	// endregion

	// region WriteTransactionInfos

	TEST(TEST_CLASS, CanWriteEmptyTransactionInfos) {
		// Arrange:
		model::TransactionInfosSet transactionInfos;
		std::vector<uint8_t> outputBuffer;
		mocks::MockMemoryStream outputStream(outputBuffer);

		// Act:
		WriteTransactionInfos(outputStream, transactionInfos);

		// Assert:
		EXPECT_EQ(std::vector<uint8_t>(sizeof(uint32_t), 0), outputBuffer);
	}

	TEST(TEST_CLASS, CanWriteTransactionInfos) {
		// Arrange:
		auto expectedTransactionInfos = CreateTransactionInfosSetWithOptionalAddresses(3);
		std::vector<uint8_t> outputBuffer;
		mocks::MockMemoryStream outputStream(outputBuffer);

		// Act:
		WriteTransactionInfos(outputStream, expectedTransactionInfos);

		// Assert: all expected transaction infos are in outputBuffer
		BufferInputStreamAdapter inputStream(outputBuffer);

		auto numTransactionInfos = Read32(inputStream);
		ASSERT_EQ(expectedTransactionInfos.size(), numTransactionInfos);

		model::TransactionInfosSet transactionInfos;
		for (auto i = 0u; i < numTransactionInfos; ++i) {
			model::TransactionInfo transactionInfo;
			ReadTransactionInfo(inputStream, transactionInfo);
			transactionInfos.insert(std::move(transactionInfo));
		}

		test::AssertEquivalent(expectedTransactionInfos, transactionInfos);
	}

	// endregion
}}
