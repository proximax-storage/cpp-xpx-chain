/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/TransactionSender.h"
#include "plugins/txes/storage/src/model/DataModificationApprovalTransaction.h"
#include "plugins/txes/storage/src/model/DataModificationSingleApprovalTransaction.h"
#include "plugins/txes/storage/src/model/DownloadApprovalTransaction.h"
#include "plugins/txes/storage/src/model/EndDriveVerificationTransaction.h"
#include "tests/TestHarness.h"
#include <boost/dynamic_bitset.hpp>

namespace catapult { namespace storage {

#define TEST_CLASS ReplicatorEventHandlerTests

	namespace {
		auto CreateTransactionSender(handlers::TransactionRangeHandler transactionRangeHandler) {
			return TransactionSender(
				crypto::KeyPair::FromString("0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"),
				config::ImmutableConfiguration::Uninitialized(),
				StorageConfiguration::Uninitialized(),
				transactionRangeHandler);
		}

		auto CreateDataModificationApprovalTransactionInfo() {
			sirius::drive::ApprovalTransactionInfo transactionInfo;

			const Key clientKey({ 11 });
			auto& opinions = transactionInfo.m_opinions;
			opinions.emplace_back(Key({ 1 }).array());
			opinions[opinions.size() - 1].m_uploadLayout = {
				sirius::drive::KeyAndBytes{ Key({ 4 }).array(), 100u },
				sirius::drive::KeyAndBytes{ Key({ 7 }).array(), 200u },
				sirius::drive::KeyAndBytes{ clientKey.array(), 0u }
			};
			opinions[opinions.size() - 1].m_signature = sirius::Signature({ 1 });

			transactionInfo.m_opinions.emplace_back(Key({ 2 }).array());
			opinions[opinions.size() - 1].m_uploadLayout = {
				sirius::drive::KeyAndBytes{ Key({ 1 }).array(), 300u },
				sirius::drive::KeyAndBytes{ Key({ 3 }).array(), 400u },
				sirius::drive::KeyAndBytes{ Key({ 6 }).array(), 500u },
				sirius::drive::KeyAndBytes{ clientKey.array(), 600u }
			};
			opinions[opinions.size() - 1].m_signature = sirius::Signature({ 2 });

			transactionInfo.m_opinions.emplace_back(Key({ 3 }).array());
			opinions[opinions.size() - 1].m_uploadLayout = {
				sirius::drive::KeyAndBytes{ Key({ 4 }).array(), 700u },
				sirius::drive::KeyAndBytes{ Key({ 7 }).array(), 800u },
				sirius::drive::KeyAndBytes{ clientKey.array(), 0u }
			};
			opinions[opinions.size() - 1].m_signature = sirius::Signature({ 3 });

			transactionInfo.m_opinions.emplace_back(Key({ 5 }).array());
			opinions[opinions.size() - 1].m_uploadLayout = {
				sirius::drive::KeyAndBytes{ Key({ 1 }).array(), 900u },
				sirius::drive::KeyAndBytes{ Key({ 6 }).array(), 1000u },
				sirius::drive::KeyAndBytes{ Key({ 7 }).array(), 1100u },
				sirius::drive::KeyAndBytes{ clientKey.array(), 0u }
			};
			opinions[opinions.size() - 1].m_signature = sirius::Signature({ 5 });

			transactionInfo.m_opinions.emplace_back(Key({ 7 }).array());
			opinions[opinions.size() - 1].m_uploadLayout = {
					sirius::drive::KeyAndBytes{ clientKey.array(), 1200u }
			};
			opinions[opinions.size() - 1].m_signature = sirius::Signature({ 7 });

			return transactionInfo;
		}
	}

    TEST(TEST_CLASS, SendDataModificationApprovalTransaction_WithOwnerUpload) {
        // Arrange:
		std::shared_ptr<model::Transaction> pTransaction;
		auto transactionRangeHandler = [&pTransaction](model::AnnotatedEntityRange<catapult::model::Transaction>&& range) {
			pTransaction = model::EntityRange<model::Transaction>::ExtractEntitiesFromRange(std::move(range.Range))[0];
		};
		auto testee = CreateTransactionSender(transactionRangeHandler);
		std::vector<Key> expectedPublicKeys{ Key({ 5 }), Key({ 2 }), Key({ 7 }), Key({ 3 }), Key({ 1 }), Key({ 6 }), Key( {11} ), Key({ 4 }) };
		std::vector<uint64_t> expectedOpinions{ 1100, 900, 1000, 0, 400, 300, 500, 600, 1200, 800, 0, 700, 200, 0, 100 };
		std::vector<uint8_t> presentOpinions{ 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1};
		std::vector<uint8_t> expectedPresentOpinions;
		boost::dynamic_bitset<uint8_t> presentOpinionsBitset(30, 0u);
		for (auto i = 0u; i < presentOpinions.size(); ++i)
			presentOpinionsBitset[i] = presentOpinions[i];
		boost::to_block_range(presentOpinionsBitset, std::back_inserter(expectedPresentOpinions));
		std::vector<Signature> expectedSignatures{ Signature({ 5 }), Signature({ 2 }), Signature({ 7 }), Signature({ 3 }), Signature({ 1 }) };

		testee.sendDataModificationApprovalTransaction(CreateDataModificationApprovalTransactionInfo());

        // Assert:
		auto& transaction = static_cast<const model::DataModificationApprovalTransaction&>(*pTransaction);
		EXPECT_EQ_MEMORY(expectedPublicKeys.data(), transaction.PublicKeysPtr(), expectedPublicKeys.size() * Key_Size);
        EXPECT_EQ_MEMORY(expectedSignatures.data(), transaction.SignaturesPtr(), expectedSignatures.size() * Signature_Size);
		EXPECT_EQ_MEMORY(expectedPresentOpinions.data(), transaction.PresentOpinionsPtr(), expectedPresentOpinions.size());
		EXPECT_EQ_MEMORY(expectedOpinions.data(), transaction.OpinionsPtr(), expectedOpinions.size() * sizeof(uint64_t));
	}

	namespace {
		auto CreateDataModificationSingleApprovalTransactionInfo() {
			sirius::drive::ApprovalTransactionInfo transactionInfo;

			const Key clientKey({ 11 });
			auto& opinions = transactionInfo.m_opinions;
			opinions.emplace_back(Key({ 1 }).array());
			opinions[opinions.size() - 1].m_uploadLayout = {
				sirius::drive::KeyAndBytes{ Key({ 2 }).array(), 100u },
				sirius::drive::KeyAndBytes{ Key({ 3 }).array(), 200u },
				sirius::drive::KeyAndBytes{ clientKey.array(), 300u },
			};
			opinions[opinions.size() - 1].m_signature = sirius::Signature({ 1 });

			return transactionInfo;
		}
	}

    TEST(TEST_CLASS, SendDataModificationSingleApprovalTransaction) {
        // Arrange:
		std::shared_ptr<model::Transaction> pTransaction;
		auto transactionRangeHandler = [&pTransaction](model::AnnotatedEntityRange<catapult::model::Transaction>&& range) {
			pTransaction = model::EntityRange<model::Transaction>::ExtractEntitiesFromRange(std::move(range.Range))[0];
		};
		auto testee = CreateTransactionSender(transactionRangeHandler);
		std::vector<Key> expectedPublicKeys{ Key({ 2 }), Key({ 3 }), Key({ 11 }) };
		std::vector<uint64_t> expectedOpinions{ 100, 200, 300 };

		testee.sendDataModificationSingleApprovalTransaction(CreateDataModificationSingleApprovalTransactionInfo());

        // Assert:
		auto& transaction = static_cast<const model::DataModificationSingleApprovalTransaction&>(*pTransaction);
        EXPECT_EQ_MEMORY(expectedPublicKeys.data(), transaction.PublicKeysPtr(), expectedPublicKeys.size() * Key_Size);
		EXPECT_EQ_MEMORY(expectedOpinions.data(), transaction.OpinionsPtr(), expectedOpinions.size() * sizeof(uint64_t));
    }

	namespace {
		auto CreateDownloadApprovalTransactionInfo() {
			sirius::drive::DownloadApprovalTransactionInfo transactionInfo;

			auto& opinions = transactionInfo.m_opinions;
			opinions.emplace_back(Key({ 1 }).array());
			opinions[opinions.size() - 1].m_downloadLayout = {
				sirius::drive::KeyAndBytes{ Key({ 4 }).array(), 100u },
				sirius::drive::KeyAndBytes{ Key({ 7 }).array(), 200u },
			};
			opinions[opinions.size() - 1].m_signature = sirius::Signature({ 1 });

			transactionInfo.m_opinions.emplace_back(Key({ 2 }).array());
			opinions[opinions.size() - 1].m_downloadLayout = {
				sirius::drive::KeyAndBytes{ Key({ 1 }).array(), 300u },
				sirius::drive::KeyAndBytes{ Key({ 3 }).array(), 400u },
				sirius::drive::KeyAndBytes{ Key({ 6 }).array(), 500u },
			};
			opinions[opinions.size() - 1].m_signature = sirius::Signature({ 2 });

			transactionInfo.m_opinions.emplace_back(Key({ 3 }).array());
			opinions[opinions.size() - 1].m_downloadLayout = {
				sirius::drive::KeyAndBytes{ Key({ 4 }).array(), 600u },
				sirius::drive::KeyAndBytes{ Key({ 7 }).array(), 700u },
			};
			opinions[opinions.size() - 1].m_signature = sirius::Signature({ 3 });

			transactionInfo.m_opinions.emplace_back(Key({ 5 }).array());
			opinions[opinions.size() - 1].m_downloadLayout = {
				sirius::drive::KeyAndBytes{ Key({ 1 }).array(), 800u },
				sirius::drive::KeyAndBytes{ Key({ 6 }).array(), 900u },
				sirius::drive::KeyAndBytes{ Key({ 7 }).array(), 1000u },
			};
			opinions[opinions.size() - 1].m_signature = sirius::Signature({ 5 });

			transactionInfo.m_opinions.emplace_back(Key({ 7 }).array());
			opinions[opinions.size() - 1].m_signature = sirius::Signature({ 7 });

			return transactionInfo;
		}
	}

    TEST(TEST_CLASS, SendDownloadApprovalTransaction) {
        // Arrange:
		std::shared_ptr<model::Transaction> pTransaction;
		auto transactionRangeHandler = [&pTransaction](model::AnnotatedEntityRange<catapult::model::Transaction>&& range) {
			pTransaction = model::EntityRange<model::Transaction>::ExtractEntitiesFromRange(std::move(range.Range))[0];
		};
		auto testee = CreateTransactionSender(transactionRangeHandler);
		std::vector<Key> expectedPublicKeys{ Key({ 5 }), Key({ 2 }), Key({ 3 }), Key({ 1 }), Key({ 6 }), Key({ 7 }), Key({ 4 }) };
		std::vector<uint64_t> expectedOpinions{ 800, 900, 1000, 400, 300, 500, 700, 600, 200, 100 };
		std::vector<uint8_t> presentOpinions{ 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1 };
		std::vector<uint8_t> expectedPresentOpinions;
		boost::dynamic_bitset<uint8_t> presentOpinionsBitset(20, 0u);
		for (auto i = 0u; i < presentOpinions.size(); ++i)
			presentOpinionsBitset[i] = presentOpinions[i];
		boost::to_block_range(presentOpinionsBitset, std::back_inserter(expectedPresentOpinions));
		std::vector<Signature> expectedSignatures{ Signature({ 5 }), Signature({ 2 }), Signature({ 3 }), Signature({ 1 }) };

		testee.sendDownloadApprovalTransaction(CreateDownloadApprovalTransactionInfo());

        // Assert:
		auto& transaction = static_cast<const model::DownloadApprovalTransaction&>(*pTransaction);
        EXPECT_EQ_MEMORY(expectedPublicKeys.data(), transaction.PublicKeysPtr(), expectedPublicKeys.size() * Key_Size);
        EXPECT_EQ_MEMORY(expectedSignatures.data(), transaction.SignaturesPtr(), expectedSignatures.size() * Signature_Size);
        EXPECT_EQ_MEMORY(expectedPresentOpinions.data(), transaction.PresentOpinionsPtr(), expectedPresentOpinions.size());
		EXPECT_EQ_MEMORY(expectedOpinions.data(), transaction.OpinionsPtr(), expectedOpinions.size() * sizeof(uint64_t));
    }

    namespace {
		auto CreateEndDriveVerifyTransactionInfo(uint8_t numberOfCosigners, uint8_t shardSize) {

			sirius::drive::VerifyApprovalTxInfo transactionInfo;

			transactionInfo.m_driveKey = test::GenerateRandomByteArray<Key>().array();
			transactionInfo.m_tx = test::GenerateRandomByteArray<Hash256>().array();
			transactionInfo.m_shardId = 0;

			auto& opinions = transactionInfo.m_opinions;
			for (int i = 0; i < numberOfCosigners; i++) {
				sirius::drive::VerifyOpinion opinion;
				opinion.m_publicKey = test::GenerateRandomByteArray<Key>().array();
				opinion.m_signature = test::GenerateRandomByteArray<Signature>().array();
				for (int j = 0; j < shardSize; j++) {
					opinion.m_opinions.push_back(1);
				}
				opinions.push_back(opinion);
			}

			return transactionInfo;
		}
	}

	TEST(TEST_CLASS, SendEndDriveVerifyApprovalTransactionWithPartialOpinions) {
		// Arrange:
		std::shared_ptr<model::Transaction> pTransaction;
		auto transactionRangeHandler = [&pTransaction](model::AnnotatedEntityRange<catapult::model::Transaction>&& range) {
			pTransaction = model::EntityRange<model::Transaction>::ExtractEntitiesFromRange(std::move(range.Range))[0];
		};
		auto testee = CreateTransactionSender(transactionRangeHandler);

		auto transactionInfo = CreateEndDriveVerifyTransactionInfo(4, 5);
		testee.sendEndDriveVerificationTransaction(transactionInfo);

		boost::dynamic_bitset<uint8_t> opinionsBitset;
		std::vector<uint8_t> expectedOpinions;
		boost::to_block_range(opinionsBitset, std::back_inserter(expectedOpinions));


		std::vector<Key> expectedPublicKeys;
		std::vector<Signature> expectedSignatures;

		for (const auto& opinion: transactionInfo.m_opinions) {
			expectedPublicKeys.emplace_back(opinion.m_publicKey);
			expectedSignatures.emplace_back(opinion.m_signature.array());
			for (const auto& d: opinion.m_opinions) {
				opinionsBitset.push_back(d);
			}
		}

		// Assert:
		auto& transaction = static_cast<const model::EndDriveVerificationTransaction&>(*pTransaction);
		EXPECT_EQ_MEMORY(expectedPublicKeys.data(), transaction.PublicKeysPtr(), expectedPublicKeys.size() * Key_Size);
		EXPECT_EQ_MEMORY(expectedSignatures.data(), transaction.SignaturesPtr(), expectedSignatures.size() * Signature_Size);
		EXPECT_EQ_MEMORY(expectedOpinions.data(), transaction.OpinionsPtr(), expectedOpinions.size());
	}

	TEST(TEST_CLASS, SendEndDriveVerifyApprovalTransactionWitFullOpinions) {
		// Arrange:
		std::shared_ptr<model::Transaction> pTransaction;
		auto transactionRangeHandler = [&pTransaction](model::AnnotatedEntityRange<catapult::model::Transaction>&& range) {
			pTransaction = model::EntityRange<model::Transaction>::ExtractEntitiesFromRange(std::move(range.Range))[0];
		};
		auto testee = CreateTransactionSender(transactionRangeHandler);

		auto transactionInfo = CreateEndDriveVerifyTransactionInfo(5, 5);
		testee.sendEndDriveVerificationTransaction(transactionInfo);

		std::vector<Key> expectedPublicKeys;
		std::vector<Signature> expectedSignatures;

		boost::dynamic_bitset<uint8_t> opinionsBitset;
		std::vector<uint8_t> expectedOpinions;
		boost::to_block_range(opinionsBitset, std::back_inserter(expectedOpinions));

		for (const auto& opinion: transactionInfo.m_opinions) {
			expectedPublicKeys.emplace_back(opinion.m_publicKey);
			expectedSignatures.emplace_back(opinion.m_signature.array());
			for (const auto& d: opinion.m_opinions) {
				opinionsBitset.push_back(d);
			}
		}

		// Assert:
		auto& transaction = static_cast<const model::EndDriveVerificationTransaction&>(*pTransaction);
		EXPECT_EQ_MEMORY(expectedPublicKeys.data(), transaction.PublicKeysPtr(), expectedPublicKeys.size() * Key_Size);
		EXPECT_EQ_MEMORY(expectedSignatures.data(), transaction.SignaturesPtr(), expectedSignatures.size() * Signature_Size);
		EXPECT_EQ_MEMORY(expectedOpinions.data(), transaction.OpinionsPtr(), expectedOpinions.size());
	}
}}
