/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "tests/test/core/SerializerTestUtils.h"
#include "tests/test/StorageTestUtils.h"

namespace catapult { namespace state {

#define TEST_CLASS BcDriveEntrySerializerTests

//	namespace {
//		constexpr auto Replicator_Count = 10;
//        constexpr auto Active_Data_Modifications_Count = 5;
//        constexpr auto Completed_Data_Modifications_Count = 5;
//        constexpr auto Verifications_Count = 1;
//
//        constexpr auto Entry_Size =
//            sizeof(VersionType) + // version
//            Key_Size + // drive key
//            Key_Size + // owner
//            Hash256_Size + // root hash
//            sizeof(uint64_t) + // size
//            sizeof(uint64_t) + // used size
//            sizeof(uint64_t) + // meta files size
//            sizeof(uint16_t) + // replicator count
//            sizeof(uint16_t) + // active data modifications count
//            Active_Data_Modifications_Count * (Hash256_Size + Key_Size + Hash256_Size + sizeof(uint64_t)) + // active data modifications
//            sizeof(uint16_t) + // completed data modifications count
//            Completed_Data_Modifications_Count * (Hash256_Size + Key_Size + Hash256_Size + sizeof(uint64_t) + sizeof(uint8_t)) + // completed data modifications
//            sizeof(uint16_t) + // verifications count
//            Verifications_Count * (Hash256_Size + sizeof(uint16_t) + Replicator_Count * sizeof(uint16_t)); // verification data
//
//        class TestContext {
//        public:
//            explicit TestContext()
//                    : m_stream(m_buffer)
//            {}
//
//        public:
//            auto& buffer() {
//                return m_buffer;
//            }
//
//            auto& outputStream() {
//                return m_stream;
//            }
//
//        private:
//            std::vector<uint8_t> m_buffer;
//            mocks::MockMemoryStream m_stream;
//        };
//
//        auto CreateBcDriveEntry() {
//			auto key = test::GenerateRandomByteArray<Key>();
//            return test::CreateBcDriveEntry(
//                test::GenerateRandomByteArray<Key>(),
//                test::GenerateRandomByteArray<Key>(),
//                test::GenerateRandomByteArray<Hash256>(),
//                test::Random(),
//				Replicator_Count,
//                Active_Data_Modifications_Count,
//                Completed_Data_Modifications_Count,
//				Verifications_Count,
//				test::Random16(),
//				test::Random16());
//        }
//
//		void AssertActiveDataModification(const ActiveDataModification& active, const uint8_t*& pData) {
//			EXPECT_EQ_MEMORY(active.Id.data(), pData, Hash256_Size);
//			pData += Hash256_Size;
//			EXPECT_EQ_MEMORY(active.Owner.data(), pData, Key_Size);
//			pData += Key_Size;
//			EXPECT_EQ_MEMORY(active.DownloadDataCdi.data(), pData, Hash256_Size);
//			pData += Hash256_Size;
//			ASSERT_EQ(active.ExpectedUploadSizeMegabytes, *reinterpret_cast<const uint64_t*>(pData));
//			pData += sizeof(uint64_t);
//			ASSERT_EQ(active.ActualUploadSizeMegabytes, *reinterpret_cast<const uint64_t*>(pData));
//			pData += sizeof(uint64_t);
//			auto folderNameSize = active.FolderName.size();
//			ASSERT_EQ(folderNameSize, *reinterpret_cast<const uint16_t*>(pData));
//			pData += sizeof(uint16_t);
//			ASSERT_EQ(active.ReadyForApproval, *reinterpret_cast<const uint8_t*>(pData));
//			pData += sizeof(uint8_t);
//			EXPECT_EQ_MEMORY(active.FolderName.c_str(), pData, folderNameSize);
//			pData += folderNameSize;
//		}
//
//        void AssertActiveDataModifications(const ActiveDataModifications& activeDataModifications, const uint8_t*& pData) {
//            ASSERT_EQ(activeDataModifications.size(), *reinterpret_cast<const uint16_t*>(pData));
//            pData += sizeof(uint16_t);
//            for (const auto& active : activeDataModifications) {
//				AssertActiveDataModification(active, pData);
//            }
//        }
//
//       	void AssertCompletedDataModifications(const CompletedDataModifications& completedDataModifications, const uint8_t*& pData) {
//			ASSERT_EQ(completedDataModifications.size(), *reinterpret_cast<const uint16_t*>(pData));
//            pData += sizeof(uint16_t);
//            for (const auto& completed : completedDataModifications) {
//				AssertActiveDataModification(completed, pData);
//                ASSERT_EQ(completed.State, static_cast<DataModificationState>(*pData));
//                pData++;
//            }
//		}
//
//		void AssertConfirmedUsedSizes(const SizeMap& confirmedUsedSizes, const uint8_t*& pData) {
//			ASSERT_EQ(confirmedUsedSizes.size(), *reinterpret_cast<const uint16_t*>(pData));
//			pData += sizeof(uint16_t);
//			for (const auto& pair : confirmedUsedSizes) {
//				EXPECT_EQ_MEMORY(pair.first.data(), pData, Key_Size);
//				pData += Key_Size;
//				ASSERT_EQ(pair.second, *reinterpret_cast<const uint64_t*>(pData));
//				pData += sizeof(uint64_t);
//			}
//		}
//
//		void AssertReplicators(const utils::SortedKeySet& replicators, const uint8_t*& pData) {
//			ASSERT_EQ(replicators.size(), *reinterpret_cast<const uint16_t*>(pData));
//			pData += sizeof(uint16_t);
//			for (const auto& replicator : replicators) {
//				EXPECT_EQ_MEMORY(replicator.data(), pData, Key_Size);
//				pData += Key_Size;
//			}
//		}
//
//        void AssertVerifications(const Verifications& verifications, const uint8_t*& pData) {
//            ASSERT_EQ(verifications.size(), *reinterpret_cast<const uint16_t*>(pData));
//            pData += sizeof(uint16_t);
//            for (const auto& verification : verifications) {
//                EXPECT_EQ_MEMORY(verification.VerificationTrigger.data(), pData, Hash256_Size);
//                pData += Hash256_Size;
//                ASSERT_EQ(verification.Expiration.unwrap(), *reinterpret_cast<const uint64_t*>(pData));
//				pData += sizeof(Timestamp);
//				ASSERT_EQ(verification.Expired, *reinterpret_cast<const uint8_t*>(pData));
//				pData += sizeof(uint8_t);
//				ASSERT_EQ(verification.Shards.size(), *reinterpret_cast<const uint16_t*>(pData));
//                pData += sizeof(uint16_t);
//				for (const auto& shard : verification.Shards) {
//					ASSERT_EQ(shard.size(), *reinterpret_cast<const uint8_t*>(pData));
//					pData += sizeof(uint8_t);
//					for (const auto& key : shard) {
//						EXPECT_EQ_MEMORY(key.data(), pData, Key_Size);
//						pData += Key_Size;
//					}
//				}
//            }
//        }
//
//		template<class T>
//		void AssertShard(const T& shard, const uint8_t*& pData) {
//			ASSERT_EQ(shard.size(), *pData);
//			pData += sizeof(uint8_t);
//
//        	for (const auto& key: shard) {
//        		ASSERT_EQ(key, *reinterpret_cast<const Key*>(pData));
//				pData += Key_Size;
//			}
//        	auto nextChannelId = *reinterpret_cast<const Hash256*>(pData);
//		}
//
//        void AssertDownloadShards(const DownloadShards& downloadShards, const uint8_t*& pData) {
//        	ASSERT_EQ(downloadShards.size(), *reinterpret_cast<const uint16_t*>(pData));
//        	pData += sizeof(uint16_t);
//        	for (const auto& channelId: downloadShards) {
//				ASSERT_EQ(channelId, *reinterpret_cast<const Hash256*>(pData));
//			}
//		}
//
//		void AssertUploadInfo(const std::map<Key, uint64_t>& info, const uint8_t*& pData) {
//        	ASSERT_EQ(info.size(), *reinterpret_cast<const uint16_t*>(pData));
//			pData += sizeof(uint16_t);
//			for (const auto& [key, uploadSize]: info) {
//				ASSERT_EQ(key, *reinterpret_cast<const Key *>(pData));
//				pData += sizeof(key);
//				ASSERT_EQ(uploadSize, *reinterpret_cast<const uint64_t *>(pData));
//				pData += sizeof(uint64_t);
//			}
//		}
//
//		void AssertModificationShards(const ModificationShards& modificationShards, const uint8_t*& pData) {
//        	ASSERT_EQ(modificationShards.size(), *reinterpret_cast<const uint16_t*>(pData));
//        	pData += sizeof(uint16_t);
//        	for (const auto& [key, shard]: modificationShards) {
//        		ASSERT_EQ(key, *reinterpret_cast<const Key *>(pData));
//        		pData += Key_Size;
//        		AssertUploadInfo(shard.m_actualShardMembers, pData);
//				AssertUploadInfo(shard.m_formerShardMembers, pData);
//				ASSERT_EQ(shard.m_ownerUpload, *reinterpret_cast<const uint64_t *>(pData));
//				pData += sizeof(uint64_t);
//        	}
//		}
//
//		void AssertConfirmedInfos(const ConfirmedStorageInfos& confirmedInfos, const uint8_t*& pData) {
//        	ASSERT_EQ(confirmedInfos.size(), *reinterpret_cast<const uint16_t*>(pData));
//        	pData += sizeof(uint16_t);
//        	for (const auto& [key, info]: confirmedInfos) {
//				CATAPULT_LOG( error ) << "assert confirmed infos " << key;
//        		ASSERT_EQ(key, *reinterpret_cast<const Key *>(pData));
//        		pData += Key_Size;
//        		ASSERT_EQ(info.m_timeInConfirmedStorage.unwrap(), *reinterpret_cast<const uint64_t *>(pData));
//        		pData += sizeof(uint64_t);
//        		ASSERT_EQ(info.m_confirmedStorageSince.has_value(), *reinterpret_cast<const bool *>(pData));
//				pData += sizeof(bool);
//				if (info.m_confirmedStorageSince) {
//					ASSERT_EQ(info.m_confirmedStorageSince->unwrap(), *reinterpret_cast<const uint64_t *>(pData));
//					pData += sizeof(uint64_t);
//				}
//        	}
//        }
//
//        void AssertEntryBuffer(const state::BcDriveEntry& entry, const uint8_t* pData, size_t expectedSize, VersionType version) {
//            const auto* const pExpectedEnd = pData + expectedSize;
//            ASSERT_EQ(version, *reinterpret_cast<const VersionType*>(pData));
//			pData += sizeof(VersionType);
//            EXPECT_EQ_MEMORY(entry.key().data(), pData, Key_Size);
//            pData += Key_Size;
//            EXPECT_EQ_MEMORY(entry.owner().data(), pData, Key_Size);
//			pData += Key_Size;
//            EXPECT_EQ_MEMORY(entry.rootHash().data(), pData, Hash256_Size);
//			pData += Hash256_Size;
//            ASSERT_EQ(entry.size(), *reinterpret_cast<const uint64_t*>(pData));
//            pData += sizeof(uint64_t);
//			ASSERT_EQ(entry.usedSizeBytes(), *reinterpret_cast<const uint64_t*>(pData));
//			pData += sizeof(uint64_t);
//			ASSERT_EQ(entry.metaFilesSizeBytes(), *reinterpret_cast<const uint64_t*>(pData));
//			pData += sizeof(uint64_t);
//            ASSERT_EQ(entry.replicatorCount(), *reinterpret_cast<const uint16_t*>(pData));
//            pData += sizeof(uint16_t);
//
//			ASSERT_EQ(entry.getQueuePrevious(), *reinterpret_cast<const Key *>(pData));
//			pData += Key_Size;
//
//			ASSERT_EQ(entry.getQueueNext(), *reinterpret_cast<const Key *>(pData));
//			pData += Key_Size;
//
//			ASSERT_EQ(entry.getLastPayment(), *reinterpret_cast<const Timestamp *>(pData));
//			pData += sizeof(Timestamp);
//
//            AssertActiveDataModifications(entry.activeDataModifications(), pData);
//            AssertCompletedDataModifications(entry.completedDataModifications(), pData);
//			AssertConfirmedUsedSizes(entry.confirmedUsedSizes(), pData);
//			AssertReplicators(entry.replicators(), pData);
//			AssertReplicators(entry.offboardingReplicators(), pData);
//            AssertVerifications(entry.verifications(), pData);
//            AssertDownloadShards(entry.downloadShards(), pData);
//			AssertModificationShards(entry.dataModificationShards(), pData);
//			AssertConfirmedInfos(entry.confirmedStorageInfos(), pData);
//
//			ASSERT_EQ(pExpectedEnd, pData);
//        }
//
//		void AssertCanSaveSingleEntry(VersionType version) {
//			// Arrange:
//			TestContext context;
//			auto entry = CreateBcDriveEntry();
//
//			// Act:
//			BcDriveEntrySerializer::Save(entry, context.outputStream());
//
//			// Assert:
//			AssertEntryBuffer(entry, context.buffer().data(), context.buffer().size(), version);
//		}
//
//		void AssertCanSaveMultipleEntries(VersionType version) {
//			// Arrange:
//			TestContext context;
//			auto entry1 = CreateBcDriveEntry();
//			auto entry2 = CreateBcDriveEntry();
//
//			// Act:
//			BcDriveEntrySerializer::Save(entry1, context.outputStream());
//			auto entryBufferSize1 = context.buffer().size();
//			BcDriveEntrySerializer::Save(entry2, context.outputStream());
//			auto entryBufferSize2 = context.buffer().size() - entryBufferSize1;
//
//			// Assert:
//			const auto* pBuffer1 = context.buffer().data();
//			const auto* pBuffer2 = pBuffer1 + entryBufferSize1;
//			AssertEntryBuffer(entry1, pBuffer1, entryBufferSize1, version);
//			AssertEntryBuffer(entry2, pBuffer2, entryBufferSize2, version);
//		}
//    }
//
//    // region Save
///*
//    TEST(TEST_CLASS, CanSaveSingleEntry_v1) {
//        AssertCanSaveSingleEntry(1);
//    }
//
//    TEST(TEST_CLASS, CanSaveMultipleEntries_v1) {
//        AssertCanSaveMultipleEntries(1);
//    }
//*/
//    // endregion
//
//    // region Load
//
//    namespace {
//
//		void CopyToVector(std::vector<uint8_t>& data, const uint8_t * p, size_t bytes) {
//			data.insert(data.end(), p, p + bytes);
//		}
//
//		void SaveActiveDataModification(const ActiveDataModification& modification, std::vector<uint8_t>& data) {
//			CopyToVector(data, modification.Id.data(), Hash256_Size);
//			CopyToVector(data, modification.Owner.data(), Key_Size);
//			CopyToVector(data, modification.DownloadDataCdi.data(), Hash256_Size);
//			CopyToVector(data, (const uint8_t *) &modification.ExpectedUploadSizeMegabytes, sizeof(uint64_t));
//			CopyToVector(data, (const uint8_t *) &modification.ActualUploadSizeMegabytes, sizeof(uint64_t));
//			auto folderNameSize = (uint16_t) modification.FolderName.size();
//			CopyToVector(data, (const uint8_t *) &folderNameSize, sizeof(folderNameSize));
//			CopyToVector(data, (const uint8_t *) &modification.ReadyForApproval, sizeof(bool));
//			CopyToVector(data, (const uint8_t *) modification.FolderName.c_str(), folderNameSize);
//		}
//
//        void SaveActiveDataModifications(const ActiveDataModifications& activeDataModifications, std::vector<uint8_t>& data) {
//            uint16_t activeDataModificationsCount = utils::checked_cast<size_t, uint16_t>(activeDataModifications.size());
//            CopyToVector(data, (const uint8_t *) &activeDataModificationsCount, sizeof(uint16_t));
//			for (const auto& modification : activeDataModifications) {
//				SaveActiveDataModification(modification, data);
//			}
//		}
//
//        void SaveCompletedDataModifications(const CompletedDataModifications& completedDataModifications, std::vector<uint8_t>& data) {
//		    uint16_t completedDataModificationsCount = utils::checked_cast<size_t, uint16_t>(completedDataModifications.size());
//			CopyToVector(data, (const uint8_t *) &completedDataModificationsCount, sizeof(uint16_t));
//            for (const auto& modification : completedDataModifications) {
//				SaveActiveDataModification(modification, data);
//                data.push_back(utils::to_underlying_type(modification.State));
//            }
//		}
//
//		void SaveConfirmedUsedSizes(const SizeMap& confirmedUsedSizes, std::vector<uint8_t>& data) {
//			uint16_t pairsCount = utils::checked_cast<size_t, uint16_t>(confirmedUsedSizes.size());
//			CopyToVector(data, (const uint8_t *) &pairsCount, sizeof(uint16_t));
//			for (const auto& pair : confirmedUsedSizes) {
//				CopyToVector(data, pair.first.data(), Key_Size);
//				CopyToVector(data, (const uint8_t *) &pair.second, sizeof(uint64_t));
//			}
//		}
//
//		void SaveCumulativeUploadSizes(const SizeMap& cumulativeUploadsSizes, std::vector<uint8_t>& data) {
//			uint16_t pairsCount = utils::checked_cast<size_t, uint16_t>(cumulativeUploadsSizes.size());
//			CopyToVector(data, (const uint8_t *) &pairsCount, sizeof(uint16_t));
//			for (const auto& pair : cumulativeUploadsSizes) {
//				CopyToVector(data, pair.first.data(), Key_Size);
//				CopyToVector(data, (const uint8_t *) &pair.second, sizeof(uint64_t));
//				CATAPULT_LOG( error ) << "saved size " << pair.second << " " << pair.first;
//			}
//		}
//
//		void SaveReplicators(const utils::SortedKeySet& replicators, std::vector<uint8_t>& data) {
//			uint16_t replicatorsCount = utils::checked_cast<size_t, uint16_t>(replicators.size());
//			CopyToVector(data, (const uint8_t *) &replicatorsCount, sizeof(uint16_t));
//			for (const auto& replicator : replicators)
//				CopyToVector(data, replicator.data(), Key_Size);
//		}
//
//        void SaveVerifications(const Verifications& verifications, std::vector<uint8_t>& data) {
//            uint16_t verificationsCount = utils::checked_cast<size_t, uint16_t>(verifications.size());
//			CATAPULT_LOG( error ) << "save verifications " << verificationsCount;
//			CopyToVector(data, (const uint8_t *) &verificationsCount, sizeof(uint16_t));
//            for (const auto& verification : verifications) {
//				CopyToVector(data, verification.VerificationTrigger.data(), Hash256_Size);
//
//				auto expiration = verification.Expiration.unwrap();
//				CopyToVector(data, (const uint8_t* ) &expiration, sizeof(uint64_t));
//				CopyToVector(data, (const uint8_t* ) &verification.Expired, sizeof(bool));
//				uint16_t shardCount = utils::checked_cast<size_t, uint16_t>(verification.Shards.size());
//				CopyToVector(data, (const uint8_t *) &shardCount, sizeof(uint16_t));
//                for (const auto& shard : verification.Shards) {
//					uint8_t shardSize = utils::checked_cast<size_t, uint8_t>(shard.size());
//					CopyToVector(data, (const uint8_t *) &shardSize, sizeof(uint8_t));
//					for (const auto& key : shard)
//						CopyToVector(data, key.data(), Key_Size);
//				}
//            }
//        }
//
//        template<typename TContainer>
//        void SaveShard(const TContainer& shard, std::vector<uint8_t>& data) {
//			uint8_t size = shard.size();
//			CopyToVector(data, (const uint8_t *) &size, sizeof(uint8_t));
//			for (auto key : shard) {
//				CopyToVector(data, (const uint8_t *) &key, Key_Size);
//			}
//		}
//
//        void SaveDownloadShards(const DownloadShards& shards, std::vector<uint8_t>& data) {
//			uint16_t shardsCount = utils::checked_cast<size_t, uint16_t>(shards.size());
//			CopyToVector(data, (const uint8_t *) &shardsCount, sizeof(uint16_t));
//			for (const auto& key : shards) {
//				CopyToVector(data, (const uint8_t*) &key, Hash256_Size);
//			}
//		}
//
//		void SaveModificationUploadInfo(const std::map<Key, uint64_t>& info, std::vector<uint8_t>& data) {
//			uint64_t size = info.size();
//			CopyToVector(data, (const uint8_t *) &size, sizeof(uint16_t));
//			for (const auto& [key, uploadSize]: info) {
//				CopyToVector(data, (const uint8_t*) &key, Key_Size);
//				CopyToVector(data, (const uint8_t*) &uploadSize, sizeof(uint64_t));
//			}
//		}
//
//		void SaveModificationShards(const ModificationShards& shards, std::vector<uint8_t>& data) {
//			uint16_t shardsCount = utils::checked_cast<size_t, uint16_t>(shards.size());
//			CopyToVector(data, (const uint8_t *) &shardsCount, sizeof(uint16_t));
//			for (const auto& [key, shard] : shards) {
//				CopyToVector(data, (const uint8_t*) &key, Key_Size);
//				SaveModificationUploadInfo(shard.m_actualShardMembers, data);
//				SaveModificationUploadInfo(shard.m_formerShardMembers, data);
//				CopyToVector(data, (const uint8_t*) &shard.m_ownerUpload, sizeof(uint64_t));
//			}
//		}
//
//		void SaveConfirmedStorageInfos(const ConfirmedStorageInfos& infos, std::vector<uint8_t>& data) {
//			uint16_t size = infos.size();
//			CopyToVector(data, (const uint8_t *) &size, sizeof(uint16_t));
//			for (const auto& [key, info] : infos) {
//				CopyToVector(data, (const uint8_t*) &key, Key_Size);
//				CopyToVector(data, (const uint8_t* ) &info.m_timeInConfirmedStorage, sizeof(Timestamp));
//
//				bool inConfirmed = info.m_confirmedStorageSince.has_value();
//				CopyToVector(data, (const uint8_t* ) &inConfirmed, sizeof(bool));
//				if (inConfirmed) {
//					Timestamp confirmedSince = *info.m_confirmedStorageSince;
//					CopyToVector(data, (const uint8_t*) &confirmedSince, sizeof(Timestamp));
//				}
//			}
//		}
//
//        std::vector<uint8_t> CreateEntryBuffer(const state::BcDriveEntry& entry, VersionType version) {
//            std::vector<uint8_t> data;
//			CopyToVector(data, (const uint8_t*)&version, sizeof(version));
//			CopyToVector(data, entry.key().data(), Key_Size);
//			CopyToVector(data, entry.owner().data(), Key_Size);
//			CopyToVector(data, entry.rootHash().data(), Hash256_Size);
//			CopyToVector(data, (const uint8_t*) &entry.size(), sizeof(uint64_t));
//			CopyToVector(data, (const uint8_t*) &entry.usedSizeBytes(), sizeof(uint64_t));
//			CopyToVector(data, (const uint8_t*) &entry.metaFilesSizeBytes(), sizeof(uint64_t));
//			CopyToVector(data, (const uint8_t*) &entry.replicatorCount(), sizeof(uint16_t));
//
//			CopyToVector(data, (const uint8_t*) &entry.getQueuePrevious(), Key_Size);
//			CopyToVector(data, (const uint8_t*) &entry.getQueueNext(), Key_Size);
//			CopyToVector(data,  (const uint8_t*) &entry.getLastPayment(), sizeof(uint64_t));
//
//            SaveActiveDataModifications(entry.activeDataModifications(), data);
//            SaveCompletedDataModifications(entry.completedDataModifications(), data);
//			SaveConfirmedUsedSizes(entry.confirmedUsedSizes(), data);
//			SaveReplicators(entry.replicators(), data);
//			SaveReplicators(entry.offboardingReplicators(), data);
//            SaveVerifications(entry.verifications(), data);
//			SaveDownloadShards(entry.downloadShards(), data);
//			SaveModificationShards(entry.dataModificationShards(), data);
//			SaveConfirmedStorageInfos(entry.confirmedStorageInfos(), data);
//
//
//            return data;
//        }
//
//        void AssertCanLoadSingleEntry(VersionType version) {
//            // Arrange:
//            TestContext context;
//            auto originalEntry = CreateBcDriveEntry();
//            auto buffer = CreateEntryBuffer(originalEntry, version);
//
//            // Act:
//            state::BcDriveEntry result(test::GenerateRandomByteArray<Key>());
//            test::RunLoadValueTest<BcDriveEntrySerializer>(buffer, result);
//
//            // Assert:
//            test::AssertEqualBcDriveData(originalEntry, result);
//        }
//    }
//
//    TEST(TEST_CLASS, CanLoadSingleEntry_v1) {
//        AssertCanLoadSingleEntry(1);
//    }
//
//    // endregion

}}