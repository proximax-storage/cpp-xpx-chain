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

		constexpr auto  ContractCall_Count = 3;
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
			Key_Size + // creator key
			Hash256_Size + // deployment base modification id
			sizeof(uint16_t) + // filename size ( AUTOMATIC EXECUTIONS INFO )
			FileName_Size + // automatic executions filename
			sizeof(uint16_t) + //function name size
			FunctionName_Size + // function name
			sizeof(uint64_t) + // next block to check
			sizeof(uint64_t) + // execution call payment
			sizeof(uint64_t) + // download call payment
			sizeof(uint32_t) + // executions number
			sizeof(uint8_t) + // executions prepaid since hasValue()
			sizeof(uint16_t) + // contract call size ( CONTRACT CALL )
			ContractCall_Count * (
		 	Hash256_Size + // call id
			Key_Size + // caller
		  	sizeof(uint16_t) + // filename size
		  	FileName_Size + // filename
		  	sizeof(uint16_t) + // function name size
		  	FunctionName_Size + //function
		 	sizeof(uint16_t) + // argument name size
		  	ArgumentName_Size + // argument
		  	sizeof(uint64_t) + // execution call payment
		  	sizeof(uint64_t) + // download call payment
		  	sizeof(uint64_t) + // block height
			sizeof(uint16_t) + // service payment size
		 	ServicePayment_Count * (
			sizeof(uint64_t) + // mosaic id
		  	sizeof(uint64_t) ) ) + // amount
			sizeof(uint32_t) + // executor size ( EXECUTORS INFO )
			Executor_Count * (
		 	Key_Size + // key
		  	sizeof(uint64_t) + // next batch to approve
		 	sizeof(uint64_t) + // poex start batch id
			sizeof(crypto::Scalar) + // poex R
		 	sizeof(curvePoint.toBytes())) + // poex T
			sizeof(uint32_t) + // batches size ( BATCHES )
			Batch_Count * (
		  	sizeof(uint64_t) + // batch.first
		  	sizeof(uint8_t) +  // success
		  	sizeof(curvePoint.toBytes()) + // poex verification information
			sizeof(uint16_t) + // completed call size
		   	CompletedCall_Count * (
			Hash256_Size + // call id
			Key_Size + // key
			sizeof(uint16_t) + // call status
			sizeof(uint64_t) + // execution work
			sizeof(uint64_t)) ) + // download work
			sizeof(uint32_t) + // release transaction size ( RELEASED TRANSACTION )
			ReleaseTransaction_Count * Hash256_Size; // hash

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
			return test::CreateSuperContractEntrySerializer(
					test::GenerateRandomByteArray<Key>(),
			        ContractCall_Count,
					ServicePayment_Count,
					Executor_Count,
					Batch_Count,
					CompletedCall_Count,
					ReleaseTransaction_Count);
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
			EXPECT_EQ_MEMORY(entry.creator().data(), pData, Key_Size);
			pData += Key_Size;
			EXPECT_EQ_MEMORY(entry.deploymentBaseModificationId().data(), pData, Hash256_Size);
			pData += Hash256_Size;

			// automatic executions info
			EXPECT_EQ(entry.automaticExecutionsInfo().AutomaticExecutionsFileName.size(), *reinterpret_cast<const uint16_t*>(pData));
			pData += sizeof(uint16_t);
			EXPECT_EQ_MEMORY(entry.automaticExecutionsInfo().AutomaticExecutionsFileName.data(), pData, entry.automaticExecutionsInfo().AutomaticExecutionsFileName.size());
			pData += entry.automaticExecutionsInfo().AutomaticExecutionsFileName.size();
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
			EXPECT_EQ(entry.automaticExecutionsInfo().AutomatedExecutionsNumber, *reinterpret_cast<const uint32_t*>(pData));
			pData += sizeof(uint32_t);
			EXPECT_EQ(entry.automaticExecutionsInfo().AutomaticExecutionsPrepaidSince.has_value(), *reinterpret_cast<const uint8_t*>(pData));
			pData += sizeof(uint8_t);

			// contract calls
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
				// service payments
				EXPECT_EQ(it.ServicePayments.size(), *reinterpret_cast<const uint16_t*>(pData));
				pData += sizeof(uint16_t);
				for(const auto& iter : it.ServicePayments){
					EXPECT_EQ(iter.MosaicId.unwrap(), *reinterpret_cast<const uint64_t*>(pData));
					pData += sizeof(uint64_t);
					EXPECT_EQ(iter.Amount.unwrap(), *reinterpret_cast<const uint64_t*>(pData));
					pData += sizeof(uint64_t);
				}
				EXPECT_EQ(it.BlockHeight.unwrap(), *reinterpret_cast<const uint64_t*>(pData));
				pData += sizeof(uint64_t);
			}

			// executors info
			EXPECT_EQ(entry.executorsInfo().size(),  *reinterpret_cast<const uint32_t*>(pData));
			pData += sizeof(uint32_t);
			for (const auto& pair : entry.executorsInfo()){
				EXPECT_EQ_MEMORY(pair.first.data(), pData, Key_Size);
				pData += Key_Size;
				const auto& info =  pair.second;
				EXPECT_EQ(info.NextBatchToApprove, *reinterpret_cast<const uint64_t*>(pData));
				pData += sizeof(uint64_t);
				EXPECT_EQ(info.PoEx.StartBatchId, *reinterpret_cast<const uint64_t*>(pData));
				pData += sizeof(uint64_t);
				auto curvePoint = info.PoEx.T.toBytes();
				EXPECT_EQ_MEMORY(&curvePoint, pData, sizeof(curvePoint));
				pData += sizeof(curvePoint);
				EXPECT_EQ_MEMORY(&info.PoEx.R.array(), pData, sizeof(crypto::Scalar));
				pData += sizeof(crypto::Scalar);
			}

			// batches
			EXPECT_EQ(entry.batches().size(),  *reinterpret_cast<const uint32_t*>(pData));
			pData += sizeof(uint32_t);
			for (const auto& pair : entry.batches()){
				EXPECT_EQ(pair.first,  *reinterpret_cast<const uint64_t*>(pData));
				pData += sizeof(uint64_t);
				EXPECT_EQ(pair.second.Success, *reinterpret_cast<const uint8_t *>(pData));
				pData += sizeof(uint8_t);
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

			// released transaction
			EXPECT_EQ(entry.releasedTransactions().size(),  *reinterpret_cast<const uint32_t*>(pData));
			pData += sizeof(uint32_t);
			for (const auto& it : entry.releasedTransactions()){
				EXPECT_EQ_MEMORY(it.data(), pData, Hash256_Size);
				pData += Hash256_Size;
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

		void AssertCanSaveMultipleEntries(VersionType version) {
			// Arrange:
			TestContext context;
			auto entry1 = CreateSuperContractEntry();
			auto entry2 = CreateSuperContractEntry();

			// Act:
			SuperContractEntrySerializer::Save(entry1, context.outputStream());
			SuperContractEntrySerializer::Save(entry2, context.outputStream());

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
		std::vector<uint8_t> CreateEntryBuffer(const state::SuperContractEntry& entry, VersionType version) {
			std::vector<uint8_t> buffer(Entry_Size);

			auto* pData = buffer.data();
			memcpy(pData, &version, sizeof(VersionType));
			pData += sizeof(VersionType);
			memcpy(pData, &entry.key(), Key_Size);
			pData += Key_Size;

			memcpy(pData, &entry.driveKey(), Key_Size);
			pData += Key_Size;
			memcpy(pData, &entry.executionPaymentKey(), Key_Size);
			pData += Key_Size;
			memcpy(pData, &entry.assignee(), Key_Size);
			pData += Key_Size;
			memcpy(pData, &entry.creator(), Key_Size);
			pData += Key_Size;
			memcpy(pData, &entry.deploymentBaseModificationId(), Hash256_Size);
			pData += Hash256_Size;

			// automatic executions info
			uint16_t filenameSize = entry.automaticExecutionsInfo().AutomaticExecutionsFileName.size();
			uint16_t functionSize = entry.automaticExecutionsInfo().AutomaticExecutionsFileName.size();
			bool hasValue = entry.automaticExecutionsInfo().AutomaticExecutionsPrepaidSince.has_value();

			memcpy(pData, &filenameSize, sizeof(uint16_t));
			pData += sizeof(uint16_t);
			memcpy(pData, entry.automaticExecutionsInfo().AutomaticExecutionsFileName.data(), filenameSize);
			pData += filenameSize;
			memcpy(pData, &functionSize, sizeof(uint16_t));
			pData += sizeof(uint16_t);
			memcpy(pData, entry.automaticExecutionsInfo().AutomaticExecutionsFunctionName.data(), functionSize);
			pData += functionSize;
			memcpy(pData, &entry.automaticExecutionsInfo().AutomaticExecutionsNextBlockToCheck, sizeof(uint64_t));
			pData += sizeof(uint64_t);
			memcpy(pData, &entry.automaticExecutionsInfo().AutomaticExecutionCallPayment, sizeof(uint64_t));
			pData += sizeof(uint64_t);
			memcpy(pData, &entry.automaticExecutionsInfo().AutomaticDownloadCallPayment, sizeof(uint64_t));
			pData += sizeof(uint64_t);
			memcpy(pData, &entry.automaticExecutionsInfo().AutomatedExecutionsNumber, sizeof(uint32_t));
			pData += sizeof(uint32_t);
			memcpy(pData, &hasValue, sizeof(uint8_t));
			pData += sizeof(uint8_t);

			// contract calls
			uint16_t contractCallsCount = utils::checked_cast<size_t, uint16_t>(entry.requestedCalls().size());
			memcpy(pData, &contractCallsCount, sizeof(uint16_t));
			pData += sizeof(uint16_t);
			for (const auto& it : entry.requestedCalls()){
				uint16_t filenameSize = it.FileName.size();
				uint16_t functionSize = it.FunctionName.size();
				uint16_t argumentsSize = it.ActualArguments.size();

				memcpy(pData, &it.CallId, Hash256_Size);
				pData += Hash256_Size;
				memcpy(pData, &it.Caller, Key_Size);
				pData += Key_Size;
				memcpy(pData, &filenameSize, sizeof(uint16_t));
				pData += sizeof(uint16_t);
				memcpy(pData, it.FileName.data(), filenameSize);
				pData += filenameSize;
				memcpy(pData, &functionSize, sizeof(uint16_t));
				pData += sizeof(uint16_t);
				memcpy(pData, it.FunctionName.data(), functionSize);
				pData += functionSize;
				memcpy(pData, &argumentsSize, sizeof(uint16_t));
				pData += sizeof(uint16_t);
				memcpy(pData, it.ActualArguments.data(), argumentsSize);
				pData += argumentsSize;
				memcpy(pData, &it.ExecutionCallPayment, sizeof(uint64_t));
				pData += sizeof(uint64_t);
				memcpy(pData, &it.DownloadCallPayment, sizeof(uint64_t));
				pData += sizeof(uint64_t);
				// service payments
				uint16_t servicePaymentSize = it.ServicePayments.size();
				memcpy(pData, &servicePaymentSize, sizeof(uint16_t));
				pData += sizeof(uint16_t);
				for (const auto& iter : it.ServicePayments){
					memcpy(pData, &iter.MosaicId, sizeof(uint64_t));
					pData += sizeof(uint64_t);
					memcpy(pData, &iter.Amount, sizeof(uint64_t));
					pData += sizeof(uint64_t);
				}
				memcpy(pData, &it.BlockHeight, sizeof(uint64_t));
				pData += sizeof(uint64_t);
			}

			// executors info
			uint32_t executorsInfoSize = entry.executorsInfo().size();
			memcpy(pData, &executorsInfoSize, sizeof(uint32_t));
			pData += sizeof(uint32_t);
			for (const auto& pair : entry.executorsInfo()){
				memcpy(pData, &pair.first, Key_Size);
				pData += Key_Size;
				memcpy(pData, &pair.second.NextBatchToApprove, sizeof(uint64_t));
				pData += sizeof(uint64_t);
				memcpy(pData, &pair.second.PoEx.StartBatchId, sizeof(uint64_t));
				pData += sizeof(uint64_t);
				auto curvePoint = pair.second.PoEx.T.toBytes();
				memcpy(pData, &curvePoint, sizeof(curvePoint));
				pData += sizeof(curvePoint);
				memcpy(pData, &pair.second.PoEx.R, sizeof(crypto::Scalar));
				pData += crypto::Scalar_Size;
			}

			// batches
			uint32_t batchesSize = entry.batches().size();
			memcpy(pData, &batchesSize, sizeof(uint32_t));
			pData += sizeof(uint32_t);
			for (const auto& pair : entry.batches()){
				memcpy(pData, &pair.first, sizeof(uint64_t));
				pData += sizeof(uint64_t);
				memcpy(pData, &pair.second.Success, sizeof(uint8_t));
				pData += sizeof(uint8_t);
				auto curvePoint = pair.second.PoExVerificationInformation.toBytes();
				memcpy(pData, &curvePoint, sizeof(curvePoint));
				pData += sizeof(curvePoint);
				// completed call
				uint16_t completedCallSize = pair.second.CompletedCalls.size();
				memcpy(pData, &completedCallSize, sizeof(uint16_t));
				pData += sizeof(uint16_t);
				for (const auto& it : pair.second.CompletedCalls){
					memcpy(pData, &it.CallId, Hash256_Size);
					pData += Hash256_Size;
					memcpy(pData, &it.Caller, Key_Size);
					pData += Key_Size;
					memcpy(pData, &it.Status, sizeof(uint16_t));
					pData += sizeof(uint16_t);
					memcpy(pData, &it.ExecutionWork, sizeof(uint64_t));
					pData += sizeof(uint64_t);
					memcpy(pData, &it.DownloadWork, sizeof(uint64_t));
					pData += sizeof(uint64_t);
				}
			}

			// released transaction
			uint32_t releasedTransactionSize = entry.releasedTransactions().size();
			memcpy(pData, &releasedTransactionSize, sizeof(uint32_t));
			pData += sizeof(uint32_t);
			for (const auto& it : entry.releasedTransactions()){
				memcpy(pData, &it, Hash256_Size);
				pData += Hash256_Size;
			}

			return buffer;
		}

		void AssertCanLoadSingleEntry(VersionType version) {
			// Arrange:
			TestContext context;
			auto originalEntry = CreateSuperContractEntry();
			auto buffer = CreateEntryBuffer(originalEntry, version);

			// Act:
			state::SuperContractEntry result(test::GenerateRandomByteArray<Key>());
			test::RunLoadValueTest<SuperContractEntrySerializer>(buffer, result);

			// Assert:
			test::AssertEqualSuperContractData(originalEntry, result);
		}
	}

	TEST(TEST_CLASS, CanLoadSingleEntry_v1) {
		AssertCanLoadSingleEntry(1);
	}

	// endregion
}}