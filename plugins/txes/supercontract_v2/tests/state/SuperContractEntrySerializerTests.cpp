/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "tests/test/core/SerializerTestUtils.h"
#include "tests/test/SuperContractTestUtils.h"
#include "catapult/model/Mosaic.h"
#include "src/state/SuperContractEntry.h"
#include "src/state/SuperContractEntrySerializer.h"

namespace catapult { namespace state {

#define TEST_CLASS SuperContractEntrySerializerTests

	namespace {

		constexpr auto  Request_Count = 3;
		constexpr auto  ServicePayment_Count = 3;
		constexpr auto  Executor_Count = 3;
		constexpr auto  Batch_Count = 3;
		constexpr auto  CompletedCall_Count = 3;
		constexpr auto  ReleaseTransaction_Count = 3;
		constexpr auto  FileName_Size = 3;
		constexpr auto  FunctionName_Size = 3;
		constexpr auto  ArgumentName_Size = 3;
		crypto::CurvePoint curvePoint;

		constexpr auto Entry_Size =
			sizeof(VersionType) +
			Key_Size + // supercontract public key
			Key_Size + // drive key
			Key_Size + // execution payment key
			Key_Size + // assignee key
			Hash256_Size + // deployment base modification id
			sizeof(uint16_t) + //filename size
			sizeof(uint16_t) + //function name size
			FileName_Size + FunctionName_Size + sizeof(uint64_t) + sizeof(uint64_t) + sizeof(uint64_t) + sizeof(uint32_t) + sizeof(uint8_t) + // automatic executions info
			sizeof(uint16_t) + // contract call size
			Request_Count * ( Hash256_Size + Key_Size + sizeof(uint16_t) + FileName_Size + sizeof(uint16_t) + FunctionName_Size + sizeof(uint16_t) + ArgumentName_Size + sizeof(uint64_t) + sizeof(uint64_t) + sizeof(uint64_t) + // contract call
			sizeof(uint16_t) + // service payment size
		 	ServicePayment_Count * (sizeof(uint64_t) + sizeof(uint64_t) ) ) + // service payment inside contract call
			sizeof(uint32_t) + // executor size
			Executor_Count * (Key_Size * sizeof(uint64_t) + sizeof(crypto::Scalar) + sizeof(curvePoint.toBytes())) + // executor info
			sizeof(uint32_t) + // batches size
			Batch_Count * ( sizeof(uint64_t)  + sizeof(bool) + sizeof(curvePoint.toBytes()) + // batches
			sizeof(uint16_t) + // completed call size
		   	CompletedCall_Count * ( Hash256_Size + Key_Size + sizeof(uint16_t) + sizeof(uint64_t) + sizeof(uint64_t) ) ) + // completed call inside batches
			sizeof(uint32_t) + // release transaction size
			ReleaseTransaction_Count * Hash256_Size; // released transaction

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

		auto CreateSuperContractEntry() {
			return test::CreateSuperContractEntry(test::GenerateRandomByteArray<Key>());
		}

		void AssertEntryBuffer(const state::SuperContractEntry& entry, const uint8_t* pData, size_t expectedSize, VersionType version) {
			const auto* pExpectedEnd = pData + expectedSize;
			EXPECT_EQ(version, *reinterpret_cast<const VersionType*>(pData));
			pData += sizeof(VersionType);
			EXPECT_EQ_MEMORY(entry.key().data(), pData, Key_Size);
			pData += Key_Size;

			EXPECT_EQ_MEMORY(entry.driveKey().data(), pData, Key_Size);
			pData += Key_Size;
			EXPECT_EQ_MEMORY(entry.executionPaymentKey().data(), pData, Key_Size);
			pData += Key_Size;
			EXPECT_EQ_MEMORY(entry.assignee().data(), pData, Key_Size);
			pData += Key_Size;
			EXPECT_EQ_MEMORY(entry.deploymentBaseModificationId().data(), pData, Hash256_Size);
			pData += Hash256_Size;

			// automatic executions info
			EXPECT_EQ(entry.automaticExecutionsInfo().AutomaticExecutionFileName.size(), *reinterpret_cast<const uint16_t*>(pData));
			pData += sizeof(uint16_t);
			EXPECT_EQ_MEMORY(entry.automaticExecutionsInfo().AutomaticExecutionFileName.data(), pData, entry.automaticExecutionsInfo().AutomaticExecutionFileName.size());
			pData += entry.automaticExecutionsInfo().AutomaticExecutionFileName.size();
			EXPECT_EQ(entry.automaticExecutionsInfo().AutomaticExecutionsFunctionName.size(), *reinterpret_cast<const uint16_t*>(pData));
			pData += sizeof(uint16_t);
			EXPECT_EQ_MEMORY(entry.automaticExecutionsInfo().AutomaticExecutionsFunctionName.data(), pData, entry.automaticExecutionsInfo().AutomaticExecutionsFunctionName.size());
			pData += entry.automaticExecutionsInfo().AutomaticExecutionsFunctionName.size();
			EXPECT_EQ(entry.automaticExecutionsInfo().AutomaticExecutionsNextBlockToCheck.unwrap(), *reinterpret_cast<const uint64_t*>(pData));
			pData += sizeof(uint64_t);
			EXPECT_EQ(entry.automaticExecutionsInfo().AutomaticExecutionCallPayment.unwrap(), *reinterpret_cast<const uint64_t*>(pData));
			pData += sizeof(uint64_t);
			EXPECT_EQ(entry.automaticExecutionsInfo().AutomaticDownloadCallPayment.unwrap(), *reinterpret_cast<const uint64_t*>(pData));
			pData += sizeof(uint64_t);
			EXPECT_EQ(entry.automaticExecutionsInfo().AutomaticDownloadCallPayment.unwrap(), *reinterpret_cast<const uint64_t*>(pData));
			pData += sizeof(uint64_t);
			EXPECT_EQ(entry.automaticExecutionsInfo().AutomatedExecutionsNumber, *reinterpret_cast<const uint32_t*>(pData));
			pData += sizeof(uint32_t);

			// requested calls
			EXPECT_EQ(entry.requestedCalls().size(),  *reinterpret_cast<const uint16_t*>(pData));
			pData += sizeof(uint16_t);
			for (const auto& it : entry.requestedCalls()){
				EXPECT_EQ_MEMORY(it.CallId.data(), pData, Hash256_Size);
				pData += Hash256_Size;
				EXPECT_EQ_MEMORY(it.Caller.data(), pData, Key_Size );
				pData += Key_Size;
				EXPECT_EQ(it.FileName.size(), *reinterpret_cast<const uint16_t*>(pData));
				pData += sizeof(uint16_t);
				EXPECT_EQ_MEMORY(it.FileName.data(), pData, it.FileName.size());
				pData += it.FileName.size();
				EXPECT_EQ(it.FunctionName.size(), *reinterpret_cast<const uint16_t*>(pData));
				pData += sizeof(uint16_t);
				EXPECT_EQ_MEMORY(it.FunctionName.data(), pData, it.FunctionName.size());
				pData += it.FunctionName.size();
				EXPECT_EQ(it.ActualArguments.size(), *reinterpret_cast<const uint16_t*>(pData));
				pData += sizeof(uint16_t);
				EXPECT_EQ_MEMORY(it.ActualArguments.data(), pData, it.ActualArguments.size());
				pData += it.ActualArguments.size();
				EXPECT_EQ(it.ExecutionCallPayment.unwrap(), *reinterpret_cast<const uint64_t*>(pData));
				pData += sizeof(uint64_t);
				EXPECT_EQ(it.DownloadCallPayment.unwrap(), *reinterpret_cast<const uint64_t*>(pData));
				pData += sizeof(uint64_t);
				EXPECT_EQ(it.BlockHeight.unwrap(), *reinterpret_cast<const uint64_t*>(pData));
				pData += sizeof(uint64_t);
				// service payments
				EXPECT_EQ(it.ServicePayments.size(), *reinterpret_cast<const uint16_t*>(pData));
				pData += sizeof(uint16_t);
				for(const auto& iter : it.ServicePayments){
					EXPECT_EQ(iter.MosaicId.unwrap(), *reinterpret_cast<const uint64_t*>(pData));
					pData += sizeof(uint64_t);
					EXPECT_EQ(iter.Amount.unwrap(), *reinterpret_cast<const uint64_t*>(pData));
					pData += sizeof(uint64_t);
				}
			}

			//executors info
			EXPECT_EQ(entry.executorsInfo().size(),  *reinterpret_cast<const uint16_t*>(pData));
			pData += sizeof(uint16_t);
			for (const auto& pair : entry.executorsInfo()){
				EXPECT_EQ_MEMORY(pair.first.data(), pData, Key_Size);
				pData += Key_Size;
				const auto& info =  pair.second;
				EXPECT_EQ(info.NextBatchToApprove, *reinterpret_cast<const uint64_t*>(pData));
				pData += sizeof(uint64_t);
				EXPECT_EQ(info.PoEx.StartBatchId, *reinterpret_cast<const uint64_t*>(pData));
				auto curvePoint = info.PoEx.T.toBytes();
				EXPECT_EQ_MEMORY(&curvePoint, pData, sizeof(curvePoint));
				pData += sizeof(curvePoint);
				EXPECT_EQ_MEMORY(&info.PoEx.R.array(), pData, sizeof(crypto::Scalar));
				pData += sizeof(crypto::Scalar);
			}

			// batches
			EXPECT_EQ(entry.batches().size(),  *reinterpret_cast<const uint16_t*>(pData));
			pData += sizeof(uint16_t);
			for (const auto& pair : entry.batches()){
				EXPECT_EQ(pair.first,  *reinterpret_cast<const uint64_t*>(pData));
				pData += sizeof(uint64_t);
				EXPECT_EQ(pair.second.Success, *reinterpret_cast<const bool*>(pData));
				pData += sizeof(bool);
				auto curvePoint = pair.second.PoExVerificationInformation.toBytes();
				EXPECT_EQ_MEMORY(&curvePoint, pData, sizeof(curvePoint));
				pData += sizeof(curvePoint);
				EXPECT_EQ(pair.second.CompletedCalls.size(),  *reinterpret_cast<const uint16_t*>(pData));
				pData += sizeof(uint16_t);
				// completed call
				for (const auto& it : pair.second.CompletedCalls){
					EXPECT_EQ_MEMORY(it.CallId.data(), pData, Hash256_Size);
					pData += Hash256_Size;
					EXPECT_EQ_MEMORY(it.Caller.data(), pData, Key_Size);
					pData += Key_Size;
					EXPECT_EQ(it.Status,  *reinterpret_cast<const uint16_t*>(pData));
					pData += sizeof(uint16_t);
					EXPECT_EQ(it.ExecutionWork.unwrap(), *reinterpret_cast<const uint64_t*>(pData));
					pData += sizeof(uint64_t);
					EXPECT_EQ(it.DownloadWork.unwrap(), *reinterpret_cast<const uint64_t*>(pData));
					pData += sizeof(uint64_t);
				}
			}

			EXPECT_EQ(pExpectedEnd, pData);
		}

		void AssertCanSaveSingleEntry(VersionType version) {
			// Arrange:
			TestContext context;
			auto entry = CreateSuperContractEntry();

			// Act:
			SuperContractEntrySerializer::Save(entry, context.outputStream());

			// Assert:
			ASSERT_EQ(Entry_Size, context.buffer().size());
			AssertEntryBuffer(entry, context.buffer().data(), Entry_Size, version);
		}

//		void AssertCanSaveMultipleEntries(VersionType version) {
//			// Arrange:
//			TestContext context;
//			auto entry1 = CreateSuperContractEntry();
//			auto entry2 = CreateSuperContractEntry();
//
//			// Act:
//			SuperContractEntrySerializer::Save(entry1, context.outputStream());
//			SuperContractEntrySerializer::Save(entry2, context.outputStream());
//
//			// Assert:
//			ASSERT_EQ(2 * Entry_Size, context.buffer().size());
//			const auto* pBuffer1 = context.buffer().data();
//			const auto* pBuffer2 = pBuffer1 + Entry_Size;
//			AssertEntryBuffer(entry1, pBuffer1, Entry_Size, version);
//			AssertEntryBuffer(entry2, pBuffer2, Entry_Size, version);
//		}
	}

	// region Save

	TEST(TEST_CLASS, CanSaveSingleEntry_v1) {
		AssertCanSaveSingleEntry(1);
	}

//	TEST(TEST_CLASS, CanSaveMultipleEntries_v1) {
//		AssertCanSaveMultipleEntries(1);
//	}

	// endregion

	// region Load
}}