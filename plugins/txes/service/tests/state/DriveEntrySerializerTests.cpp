/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/state/DriveEntrySerializer.h"
#include "catapult/utils/HexFormatter.h"
#include "catapult/utils/Casting.h"
#include "tests/test/core/SerializerOrderingTests.h"
#include "tests/test/core/SerializerTestUtils.h"
#include "tests/test/ServiceTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace state {

#define TEST_CLASS DriveEntrySerializerTests

	namespace {
		constexpr auto Billing_History_Count = 2;
		constexpr auto Drive_Payment_Count = 3;
		constexpr auto File_Count = 4;
		constexpr auto Replicator_Count = 5;
		constexpr auto Active_Files_Without_Deposit_Count = 6;
		constexpr auto Inactive_Files_Without_Deposit_Count = 7;
		constexpr auto Height_Count = 8;
		constexpr auto Removed_Replicator_Count = 9;
		constexpr auto Upload_Payment_Count = 10;

		constexpr auto Entry_Size =
			sizeof(VersionType) + // version
			Key_Size + // key
			sizeof(uint8_t) + // state
			Key_Size + // owner
			Hash256_Size + // rootHash
			sizeof(uint64_t) + // start
			sizeof(uint64_t) + // end
			sizeof(uint64_t) + // duration
			sizeof(uint64_t) + // billingPeriod
			sizeof(uint64_t) + // billingPrice

			// region billing history

			sizeof(uint16_t) + // billing history count
			Billing_History_Count * sizeof(uint64_t) + // Start
			Billing_History_Count * sizeof(uint64_t) + // End
			Billing_History_Count * sizeof(uint16_t) + // payment count
			Billing_History_Count * Drive_Payment_Count * Key_Size + // Receiver
			Billing_History_Count * Drive_Payment_Count * sizeof(uint64_t) + // Amount
			Billing_History_Count * Drive_Payment_Count * sizeof(uint64_t) + // Height

			// end region

			sizeof(uint64_t) + // size
			sizeof(uint64_t) + // occupied space
			sizeof(uint16_t) + // replicas
			sizeof(uint16_t) + // minReplicators
			sizeof(uint8_t) + // percentApprovers

			// region files

			sizeof(uint16_t) + // file count
			File_Count * Hash256_Size + // file hash
			File_Count * sizeof(uint64_t) + // file size

			// end region

			// region replicators

			2 * sizeof(uint16_t) + // replicator count
			(Replicator_Count + Removed_Replicator_Count) * Key_Size + // replicator public key
			(Replicator_Count + Removed_Replicator_Count) * sizeof(uint64_t) + // Start
			(Replicator_Count + Removed_Replicator_Count) * sizeof(uint64_t) + // End
			(Replicator_Count + Removed_Replicator_Count) * sizeof(uint16_t) + // active files without deposit count
			(Replicator_Count + Removed_Replicator_Count) * Active_Files_Without_Deposit_Count * Hash256_Size + // file hash
			(Replicator_Count + Removed_Replicator_Count) * sizeof(uint16_t) + // inactive files without deposit count
			(Replicator_Count + Removed_Replicator_Count) * Inactive_Files_Without_Deposit_Count * Hash256_Size + // file hash
			(Replicator_Count + Removed_Replicator_Count) * Inactive_Files_Without_Deposit_Count * sizeof(uint16_t) + // height count
			(Replicator_Count + Removed_Replicator_Count) * Inactive_Files_Without_Deposit_Count * Height_Count * sizeof(Height) + // heights

			// end region

			// region upload payments

			sizeof(uint16_t) + // payment count
			Upload_Payment_Count * Key_Size + // Receiver
			Upload_Payment_Count * sizeof(uint64_t) + // Amount
			Upload_Payment_Count * sizeof(uint64_t); // Height

			// end region

		class TestContext {
		public:
			explicit TestContext()
					: m_stream(m_buffer)
			{}

		public:
			auto& buffer() {
				return m_buffer;
			}

			auto& outputStream() {
				return m_stream;
			}

		private:
			std::vector<uint8_t> m_buffer;
			mocks::MockMemoryStream m_stream;
		};

		auto CreateDriveEntry() {
			return test::CreateDriveEntry(
				test::GenerateRandomByteArray<Key>(),
				Billing_History_Count,
				Drive_Payment_Count,
				File_Count,
				Replicator_Count,
				Active_Files_Without_Deposit_Count,
				Inactive_Files_Without_Deposit_Count,
				Height_Count,
				Removed_Replicator_Count,
				Upload_Payment_Count);
		}

		void AssertPayments(const std::vector<PaymentInformation>& payments, const uint8_t*& pData) {
			EXPECT_EQ(payments.size(), *reinterpret_cast<const uint16_t*>(pData));
			pData += sizeof(uint16_t);
			for (const auto& payment : payments) {
				EXPECT_EQ_MEMORY(payment.Receiver.data(), pData, Key_Size);
				pData += Key_Size;
				EXPECT_EQ(payment.Amount.unwrap(), *reinterpret_cast<const uint64_t*>(pData));
				pData += sizeof(uint64_t);
				EXPECT_EQ(payment.Height.unwrap(), *reinterpret_cast<const uint64_t*>(pData));
				pData += sizeof(uint64_t);
			}
		}

		template<typename T>
		void AssertReplicators(const T& replicators, const uint8_t*& pData) {
			EXPECT_EQ(replicators.size(), *reinterpret_cast<const uint16_t*>(pData));
			pData += sizeof(uint16_t);
			for (const auto& replicatorPair : replicators) {
				EXPECT_EQ_MEMORY(replicatorPair.first.data(), pData, Key_Size);
				pData += Key_Size;
				EXPECT_EQ(replicatorPair.second.Start.unwrap(), *reinterpret_cast<const uint64_t*>(pData));
				pData += sizeof(uint64_t);
				EXPECT_EQ(replicatorPair.second.End.unwrap(), *reinterpret_cast<const uint64_t*>(pData));
				pData += sizeof(uint64_t);

				EXPECT_EQ(replicatorPair.second.ActiveFilesWithoutDeposit.size(), *reinterpret_cast<const uint16_t*>(pData));
				pData += sizeof(uint16_t);
				for (const auto& fileHash : replicatorPair.second.ActiveFilesWithoutDeposit) {
					EXPECT_EQ_MEMORY(fileHash.data(), pData, Hash256_Size);
					pData += Hash256_Size;
				}

				EXPECT_EQ(replicatorPair.second.InactiveFilesWithoutDeposit.size(), *reinterpret_cast<const uint16_t*>(pData));
				pData += sizeof(uint16_t);
				for (const auto& pair : replicatorPair.second.InactiveFilesWithoutDeposit) {
					EXPECT_EQ_MEMORY(pair.first.data(), pData, Hash256_Size);
					pData += Hash256_Size;
					EXPECT_EQ(pair.second.size(), *reinterpret_cast<const uint16_t*>(pData));
					pData += sizeof(uint16_t);
					for (const auto& height : pair.second) {
						EXPECT_EQ(height.unwrap(), *reinterpret_cast<const uint64_t*>(pData));
						pData += sizeof(uint64_t);
					}
				}
			}
		}

		void AssertEntryBuffer(const state::DriveEntry& entry, const uint8_t* pData, size_t expectedSize, VersionType version) {
			const auto* pExpectedEnd = pData + expectedSize;
			EXPECT_EQ(version, *reinterpret_cast<const VersionType*>(pData));
			pData += sizeof(VersionType);
			EXPECT_EQ_MEMORY(entry.key().data(), pData, Key_Size);
			pData += Key_Size;
			EXPECT_EQ(entry.state(), static_cast<DriveState>(*pData));
			pData++;
			EXPECT_EQ_MEMORY(entry.owner().data(), pData, Key_Size);
			pData += Key_Size;
			EXPECT_EQ_MEMORY(entry.rootHash().data(), pData, Hash256_Size);
			pData += Hash256_Size;
			EXPECT_EQ(entry.start().unwrap(), *reinterpret_cast<const uint64_t*>(pData));
			pData += sizeof(uint64_t);
			EXPECT_EQ(entry.end().unwrap(), *reinterpret_cast<const uint64_t*>(pData));
			pData += sizeof(uint64_t);
			EXPECT_EQ(entry.duration().unwrap(), *reinterpret_cast<const uint64_t*>(pData));
			pData += sizeof(uint64_t);
			EXPECT_EQ(entry.billingPeriod().unwrap(), *reinterpret_cast<const uint64_t*>(pData));
			pData += sizeof(uint64_t);
			EXPECT_EQ(entry.billingPrice().unwrap(), *reinterpret_cast<const uint64_t*>(pData));
			pData += sizeof(uint64_t);

			// region billing history

			EXPECT_EQ(entry.billingHistory().size(), *reinterpret_cast<const uint16_t*>(pData));
			pData += sizeof(uint16_t);
			for (const auto& description : entry.billingHistory()) {
				EXPECT_EQ(description.Start.unwrap(), *reinterpret_cast<const uint64_t*>(pData));
				pData += sizeof(uint64_t);
				EXPECT_EQ(description.End.unwrap(), *reinterpret_cast<const uint64_t*>(pData));
				pData += sizeof(uint64_t);
				AssertPayments(description.Payments, pData);
			}

			// end region

			EXPECT_EQ(entry.size(), *reinterpret_cast<const uint64_t*>(pData));
			pData += sizeof(uint64_t);
			EXPECT_EQ(entry.occupiedSpace(), *reinterpret_cast<const uint64_t*>(pData));
			pData += sizeof(uint64_t);
			EXPECT_EQ(entry.replicas(), *reinterpret_cast<const uint16_t*>(pData));
			pData += sizeof(uint16_t);
			EXPECT_EQ(entry.minReplicators(), *reinterpret_cast<const uint16_t*>(pData));
			pData += sizeof(uint16_t);
			EXPECT_EQ(entry.percentApprovers(), *pData);
			pData++;

			// region files

			EXPECT_EQ(entry.files().size(), *reinterpret_cast<const uint16_t*>(pData));
			pData += sizeof(uint16_t);
			for (const auto& filePair : entry.files()) {
				EXPECT_EQ_MEMORY(filePair.first.data(), pData, Hash256_Size);
				pData += Hash256_Size;
				EXPECT_EQ(filePair.second.Size, *reinterpret_cast<const uint64_t*>(pData));
				pData += sizeof(uint64_t);
			}

			// end region

			// region replicators

			AssertReplicators(entry.replicators(), pData);
			AssertReplicators(entry.removedReplicators(), pData);

			// end region

			// region upload payments

			AssertPayments(entry.uploadPayments(), pData);

			// end region

			EXPECT_EQ(pExpectedEnd, pData);
		}

		void AssertCanSaveSingleEntry(VersionType version) {
			// Arrange:
			TestContext context;
			auto entry = CreateDriveEntry();

			// Act:
			DriveEntrySerializer::Save(entry, context.outputStream());

			// Assert:
			ASSERT_EQ(Entry_Size, context.buffer().size());
			AssertEntryBuffer(entry, context.buffer().data(), Entry_Size, version);
		}

		void AssertCanSaveMultipleEntries(VersionType version) {
			// Arrange:
			TestContext context;
			auto entry1 = CreateDriveEntry();
			auto entry2 = CreateDriveEntry();

			// Act:
			DriveEntrySerializer::Save(entry1, context.outputStream());
			DriveEntrySerializer::Save(entry2, context.outputStream());

			// Assert:
			ASSERT_EQ(2 * Entry_Size, context.buffer().size());
			const auto* pBuffer1 = context.buffer().data();
			const auto* pBuffer2 = pBuffer1 + Entry_Size;
			AssertEntryBuffer(entry1, pBuffer1, Entry_Size, version);
			AssertEntryBuffer(entry2, pBuffer2, Entry_Size, version);
		}
	}

	// region Save

	TEST(TEST_CLASS, CanSaveSingleEntry_v1) {
		AssertCanSaveSingleEntry(1);
	}

	TEST(TEST_CLASS, CanSaveMultipleEntries_v1) {
		AssertCanSaveMultipleEntries(1);
	}

	// endregion

	// region Load

	namespace {
		void SavePayments(const std::vector<PaymentInformation>& payments, uint8_t*& pData) {
			uint16_t paymentCount = utils::checked_cast<size_t, uint16_t>(payments.size());
			memcpy(pData, &paymentCount, sizeof(uint16_t));
			pData += sizeof(uint16_t);
			for (const auto& payment : payments) {
				memcpy(pData, payment.Receiver.data(), Key_Size);
				pData += Key_Size;
				memcpy(pData, &payment.Amount, sizeof(uint64_t));
				pData += sizeof(uint64_t);
				memcpy(pData, &payment.Height, sizeof(uint64_t));
				pData += sizeof(uint64_t);
			}
		}

		template<typename T>
		void SaveReplicators(const T& replicators, uint8_t*& pData) {
			uint16_t count = utils::checked_cast<size_t, uint16_t>(replicators.size());
			memcpy(pData, &count, sizeof(uint16_t));
			pData += sizeof(uint16_t);
			for (const auto& replicatorPair : replicators) {
				memcpy(pData, replicatorPair.first.data(), Key_Size);
				pData += Key_Size;
				memcpy(pData, &replicatorPair.second.Start, sizeof(uint64_t));
				pData += sizeof(uint64_t);
				memcpy(pData, &replicatorPair.second.End, sizeof(uint64_t));
				pData += sizeof(uint64_t);

				count = utils::checked_cast<size_t, uint16_t>(replicatorPair.second.ActiveFilesWithoutDeposit.size());
				memcpy(pData, &count, sizeof(uint16_t));
				pData += sizeof(uint16_t);
				for (const auto& fileHash : replicatorPair.second.ActiveFilesWithoutDeposit) {
					memcpy(pData, fileHash.data(), Hash256_Size);
					pData += Hash256_Size;
				}

				count = utils::checked_cast<size_t, uint16_t>(replicatorPair.second.InactiveFilesWithoutDeposit.size());
				memcpy(pData, &count, sizeof(uint16_t));
				pData += sizeof(uint16_t);
				for (const auto& pair : replicatorPair.second.InactiveFilesWithoutDeposit) {
					memcpy(pData, pair.first.data(), Hash256_Size);
					pData += Hash256_Size;
					count = utils::checked_cast<size_t, uint16_t>(pair.second.size());
					memcpy(pData, &count, sizeof(uint16_t));
					pData += sizeof(uint16_t);
					for (const auto& height : pair.second) {
						memcpy(pData, &height, sizeof(uint64_t));
						pData += sizeof(uint64_t);
					}
				}
			}
		}

		std::vector<uint8_t> CreateEntryBuffer(const state::DriveEntry& entry, VersionType version) {
			std::vector<uint8_t> buffer(Entry_Size);

			auto* pData = buffer.data();
			memcpy(pData, &version, sizeof(VersionType));
			pData += sizeof(VersionType);
			memcpy(pData, entry.key().data(), Key_Size);
			pData += Key_Size;
			*pData = utils::to_underlying_type(entry.state());
			pData++;
			memcpy(pData, entry.owner().data(), Key_Size);
			pData += Key_Size;
			memcpy(pData, entry.rootHash().data(), Hash256_Size);
			pData += Hash256_Size;
			memcpy(pData, &entry.start(), sizeof(uint64_t));
			pData += sizeof(uint64_t);
			memcpy(pData, &entry.end(), sizeof(uint64_t));
			pData += sizeof(uint64_t);
			memcpy(pData, &entry.duration(), sizeof(uint64_t));
			pData += sizeof(uint64_t);
			memcpy(pData, &entry.billingPeriod(), sizeof(uint64_t));
			pData += sizeof(uint64_t);
			memcpy(pData, &entry.billingPrice(), sizeof(uint64_t));
			pData += sizeof(uint64_t);

			uint16_t billingHistoryCount = utils::checked_cast<size_t, uint16_t>(entry.billingHistory().size());
			memcpy(pData, &billingHistoryCount, sizeof(uint16_t));
			pData += sizeof(uint16_t);
			for (const auto& description : entry.billingHistory()) {
				memcpy(pData, &description.Start, sizeof(uint64_t));
				pData += sizeof(uint64_t);
				memcpy(pData, &description.End, sizeof(uint64_t));
				pData += sizeof(uint64_t);
				SavePayments(description.Payments, pData);
			}

			memcpy(pData, &entry.size(), sizeof(uint64_t));
			pData += sizeof(uint64_t);
			memcpy(pData, &entry.occupiedSpace(), sizeof(uint64_t));
			pData += sizeof(uint64_t);
			memcpy(pData, &entry.replicas(), sizeof(uint16_t));
			pData += sizeof(uint16_t);
			memcpy(pData, &entry.minReplicators(), sizeof(uint16_t));
			pData += sizeof(uint16_t);
			*pData = entry.percentApprovers();
			pData++;

			uint16_t fileCount = utils::checked_cast<size_t, uint16_t>(entry.files().size());
			memcpy(pData, &fileCount, sizeof(uint16_t));
			pData += sizeof(uint16_t);
			for (const auto& filePair : entry.files()) {
				memcpy(pData, filePair.first.data(), Hash256_Size);
				pData += Hash256_Size;
				memcpy(pData, &filePair.second.Size, sizeof(uint64_t));
				pData += sizeof(uint64_t);
			}

			SaveReplicators(entry.replicators(), pData);
			SaveReplicators(entry.removedReplicators(), pData);

			SavePayments(entry.uploadPayments(), pData);

			return buffer;
		}

		void AssertCanLoadSingleEntry(VersionType version) {
			// Arrange:
			TestContext context;
			auto originalEntry = CreateDriveEntry();
			auto buffer = CreateEntryBuffer(originalEntry, version);

			// Act:
			state::DriveEntry result(test::GenerateRandomByteArray<Key>());
			test::RunLoadValueTest<DriveEntrySerializer>(buffer, result);

			// Assert:
			test::AssertEqualDriveData(originalEntry, result);
		}
	}

	TEST(TEST_CLASS, CanLoadSingleEntry_v1) {
		AssertCanLoadSingleEntry(1);
	}

	// endregion
}}
