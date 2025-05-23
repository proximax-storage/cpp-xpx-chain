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

#include "sdk/src/extensions/ConversionExtensions.h"
#include "zeromq/src/PublisherUtils.h"
#include "catapult/model/Address.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionStatus.h"
#include "zeromq/tests/test/ZeroMqTestUtils.h"
#include "tests/test/core/BlockTestUtils.h"

namespace catapult { namespace zeromq {

#define TEST_CLASS ZeroMqEntityPublisherTests

	namespace {
		model::TransactionInfo ToTransactionInfo(model::UniqueEntityPtr<mocks::MockTransaction>&& pTransaction, const Height& height) {
			model::TransactionInfo transactionInfo(std::move(pTransaction), height);
			transactionInfo.EntityHash = test::GenerateRandomByteArray<Hash256>();
			transactionInfo.MerkleComponentHash = test::GenerateRandomByteArray<Hash256>();
			return transactionInfo;
		}

		model::TransactionElement ToTransactionElement(const mocks::MockTransaction& transaction) {
			model::TransactionElement transactionElement(transaction);
			transactionElement.EntityHash = test::GenerateRandomByteArray<Hash256>();
			transactionElement.MerkleComponentHash = test::GenerateRandomByteArray<Hash256>();
			return transactionElement;
		}

		class EntityPublisherContext : public test::MqContext {
		public:
			void publishBlockHeader(const model::BlockElement& blockElement) {
				publisher().publishBlockHeader(blockElement);
			}

			void publishDropBlocks(Height height) {
				publisher().publishDropBlocks(height);
			}

			void publishTransaction(TransactionMarker topicMarker, const model::TransactionInfo& transactionInfo) {
				publisher().publishTransaction(topicMarker, transactionInfo);
			}

			void publishTransaction(TransactionMarker topicMarker, const model::TransactionElement& transactionElement, Height height) {
				publisher().publishTransaction(topicMarker, transactionElement, height);
			}

			void publishTransactionHash(TransactionMarker topicMarker, const model::TransactionInfo& transactionInfo) {
				publisher().publishTransactionHash(topicMarker, transactionInfo);
			}

			void publishTransactionStatus(const model::Transaction& transaction, const Height& height, const Hash256& hash, uint32_t status) {
				publisher().publishTransactionStatus(transaction, height, hash, status);
			}

			void publishCosignature(const model::TransactionInfo& parentTransactionInfo, const Key& signer, const Signature& signature) {
				publisher().publishCosignature(parentTransactionInfo, signer, signature);
			}
		};

		std::shared_ptr<model::UnresolvedAddressSet> GenerateRandomExtractedAddresses() {
			// Arrange: generate three random addresses
			return test::GenerateRandomUnresolvedAddressSetPointer(3);
		}
	}

	// region basic tests

	TEST(TEST_CLASS, CanDestroyPublisherWithNonEmptyQueueWithoutCrash) {
		// Arrange:
		EntityPublisherContext context;
		context.subscribe(BlockMarker::Drop_Blocks_Marker);
		Height height(123);

		// Act + Assert:
		context.publishDropBlocks(height);
		context.destroyPublisher();
	}

	// endregion

	// region publishBlockHeader

	TEST(TEST_CLASS, CanPublishBlockHeader) {
		// Arrange:
		EntityPublisherContext context;
		context.subscribe(BlockMarker::Block_Marker);
		auto pBlock = test::GenerateEmptyRandomBlock();
		auto blockElement = test::BlockToBlockElement(*pBlock);

		// Act:
		context.publishBlockHeader(blockElement);

		// Assert:
		zmq::multipart_t message;
		test::ZmqReceive(message, context.zmqSocket());

		test::AssertBlockHeaderMessage(message, blockElement);
	}

	// endregion

	// region publishDropBlocks

	TEST(TEST_CLASS, CanPublishDropBlocks) {
		// Arrange:
		EntityPublisherContext context;
		context.subscribe(BlockMarker::Drop_Blocks_Marker);
		Height height(123);

		// Act:
		context.publishDropBlocks(height);

		// Assert:
		zmq::multipart_t message;
		test::ZmqReceive(message, context.zmqSocket());

		test::AssertDropBlocksMessage(message, height);
	}

	// endregion

	// region publishTransaction

	namespace {
		constexpr TransactionMarker Marker = TransactionMarker(12);

		template<typename TAddressesGenerator>
		void AssertCanPublishTransactionInfo(TAddressesGenerator generateAddresses) {
			// Arrange:
			EntityPublisherContext context;
			Height height(123);
			auto transactionInfo = ToTransactionInfo(mocks::CreateMockTransaction(0), height);
			auto addresses = generateAddresses(transactionInfo);
			context.subscribeAll(Marker, addresses);

			// Act:
			context.publishTransaction(Marker, transactionInfo);

			// Assert:
			auto& zmqSocket = context.zmqSocket();
			test::AssertMessages(zmqSocket, Marker, addresses, [&transactionInfo, height](const auto& message, const auto& topic) {
				test::AssertTransactionInfoMessage(message, topic, transactionInfo, height);
			});
		}

		template<typename TAddressesGenerator>
		void AssertCanPublishTransactionElement(TAddressesGenerator generateAddresses) {
			// Arrange:
			EntityPublisherContext context;
			auto pTransaction = mocks::CreateMockTransaction(0);
			auto transactionElement = ToTransactionElement(*pTransaction);
			Height height(123);
			auto addresses = generateAddresses(transactionElement);
			context.subscribeAll(Marker, addresses);

			// Act:
			context.publishTransaction(Marker, transactionElement, height);

			// Assert:
			auto& zmqSocket = context.zmqSocket();
			test::AssertMessages(zmqSocket, Marker, addresses, [&transactionElement, height](const auto& message, const auto& topic) {
				test::AssertTransactionElementMessage(message, topic, transactionElement, height);
			});
		}
	}

	TEST(TEST_CLASS, CanPublishTransaction_TransactionInfo) {
		// Assert:
		AssertCanPublishTransactionInfo([](const auto& transactionInfo) {
			return test::ExtractAddresses(test::ToMockTransaction(*transactionInfo.pEntity));
		});
	}

	TEST(TEST_CLASS, CanPublishTransactionToCustomAddresses_TransactionInfo) {
		// Assert:
		AssertCanPublishTransactionInfo([](auto& transactionInfo) {
			transactionInfo.OptionalExtractedAddresses = GenerateRandomExtractedAddresses();
			return *transactionInfo.OptionalExtractedAddresses;
		});
	}

	TEST(TEST_CLASS, CanPublishTransaction_TransactionElement) {
		// Assert:
		AssertCanPublishTransactionElement([](const auto& transactionElement) {
			return test::ExtractAddresses(test::ToMockTransaction(transactionElement.Transaction));
		});
	}

	TEST(TEST_CLASS, CanPublishTransactionToCustomAddresses_TransactionElement) {
		// Assert:
		AssertCanPublishTransactionElement([](auto& transactionElement) {
			transactionElement.OptionalExtractedAddresses = GenerateRandomExtractedAddresses();
			return *transactionElement.OptionalExtractedAddresses;
		});
	}

	TEST(TEST_CLASS, PublishTransactionDeliversMessagesOnlyToRegisteredSubscribers) {
		// Arrange:
		EntityPublisherContext context;
		auto pTransaction = mocks::CreateMockTransaction(0);
		auto recipientAddress = model::PublicKeyToAddress(pTransaction->Recipient, model::NetworkIdentifier(pTransaction->Network()));
		auto unresolvedRecipientAddress = extensions::CopyToUnresolvedAddress(recipientAddress);
		Height height(123);
		auto transactionInfo = ToTransactionInfo(std::move(pTransaction), height);

		// - only subscribe to the recipient address (and not to other addresses like the sender)
		context.subscribeAll(Marker, { unresolvedRecipientAddress });

		// Act:
		context.publishTransaction(Marker, transactionInfo);

		// Assert:
		zmq::multipart_t message;
		test::ZmqReceive(message, context.zmqSocket());

		// - only a single message is sent to the recipient address (because that is the only subscribed address)
		auto topic = CreateTopic(Marker, unresolvedRecipientAddress);
		test::AssertTransactionInfoMessage(message, topic, transactionInfo, height);

		// - no other message is pending (e.g. to sender)
		test::AssertNoPendingMessages(context.zmqSocket());
	}

	TEST(TEST_CLASS, PublishTransactionDeliversNoMessagesWhenNoAddressesAreAssociatedWithTransaction) {
		// Arrange:
		EntityPublisherContext context;
		Height height(123);
		auto transactionInfo = ToTransactionInfo(mocks::CreateMockTransaction(0), height);
		auto addresses = test::ExtractAddresses(test::ToMockTransaction(*transactionInfo.pEntity));
		context.subscribeAll(Marker, addresses);

		// - associate no addresses with the transaction
		transactionInfo.OptionalExtractedAddresses = std::make_shared<model::UnresolvedAddressSet>();

		// Act:
		context.publishTransaction(Marker, transactionInfo);

		// Assert: no messages are pending
		test::AssertNoPendingMessages(context.zmqSocket());
	}

	// endregion

	// region publishTransactionHash

	namespace {
		template<typename TAddressesGenerator>
		void AssertCanPublishTransactionHash(TAddressesGenerator generateAddresses) {
			// Arrange:
			EntityPublisherContext context;
			auto pTransaction = mocks::CreateMockTransaction(0);
			auto transactionInfo = ToTransactionInfo(mocks::CreateMockTransaction(0), Height());
			auto addresses = generateAddresses(transactionInfo);
			context.subscribeAll(Marker, addresses);

			// Act:
			context.publishTransactionHash(Marker, transactionInfo);

			// Assert:
			const auto& hash = transactionInfo.EntityHash;
			test::AssertMessages(context.zmqSocket(), Marker, addresses, [&hash](const auto& message, const auto& topic) {
				test::AssertTransactionHashMessage(message, topic, hash);
			});
		}
	}

	TEST(TEST_CLASS, CanPublishTransactionHash) {
		// Assert:
		AssertCanPublishTransactionHash([](const auto& transactionInfo) {
			return test::ExtractAddresses(test::ToMockTransaction(*transactionInfo.pEntity));
		});
	}

	TEST(TEST_CLASS, CanPublishTransactionHashToCustomAddresses) {
		// Assert:
		AssertCanPublishTransactionHash([](auto& transactionInfo) {
			transactionInfo.OptionalExtractedAddresses = GenerateRandomExtractedAddresses();
			return *transactionInfo.OptionalExtractedAddresses;
		});
	}

	// endregion

	// region publishTransactionStatus

	TEST(TEST_CLASS, CanPublishTransactionStatus) {
		// Arrange:
		EntityPublisherContext context;
		auto pTransaction = mocks::CreateMockTransaction(0);
		auto hash = test::GenerateRandomByteArray<Hash256>();
		auto addresses = test::ExtractAddresses(*pTransaction);
		TransactionMarker marker = TransactionMarker::Transaction_Status_Marker;
		context.subscribeAll(marker, addresses);

		// Act:
		context.publishTransactionStatus(*pTransaction, Height(), hash, 123);

		// Assert:
		model::TransactionStatus expectedTransactionStatus(hash, 123, pTransaction->Deadline);
		test::AssertMessages(context.zmqSocket(), marker, addresses, [&expectedTransactionStatus](const auto& message, const auto& topic) {
			test::AssertTransactionStatusMessage(message, topic, expectedTransactionStatus);
		});
	}

	// endregion

	// region publishCosignature

	TEST(TEST_CLASS, CanPublishCosignature) {
		// Arrange:
		EntityPublisherContext context;
		auto transactionInfo = ToTransactionInfo(mocks::CreateMockTransaction(0), Height());
		auto signer = test::GenerateRandomByteArray<Key>();
		auto signature = test::GenerateRandomByteArray<Signature>();
		auto addresses = test::ExtractAddresses(test::ToMockTransaction(*transactionInfo.pEntity));
		TransactionMarker marker = TransactionMarker::Cosignature_Marker;
		context.subscribeAll(marker, addresses);

		// Act:
		context.publishCosignature(transactionInfo, signer, signature);

		// Assert:
		model::DetachedCosignature expectedDetachedCosignature(signer, signature, transactionInfo.EntityHash);
		auto& zmqSocket = context.zmqSocket();
		test::AssertMessages(zmqSocket, marker, addresses, [&expectedDetachedCosignature](const auto& message, const auto& topic) {
			test::AssertDetachedCosignatureMessage(message, topic, expectedDetachedCosignature);
		});
	}

	// endregion
}}
