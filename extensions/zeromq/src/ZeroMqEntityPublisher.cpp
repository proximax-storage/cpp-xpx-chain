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

#include "PublisherUtils.h"
#include "catapult/model/Elements.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionStatus.h"
#include "catapult/model/TransactionUtils.h"
#include "catapult/thread/IoThreadPool.h"
#include <boost/asio.hpp>

namespace catapult { namespace zeromq {

	class MessageGroup {
	public:
		explicit MessageGroup(const supplier<std::string>& errorMessageGenerator) : m_errorMessageGenerator(errorMessageGenerator)
		{}

	public:
		void add(zmq::multipart_t&& message) {
			m_messages.push_back(std::move(message));
		}

		void flush(zmq::socket_t& zmqSocket) {
			bool result = true;
			for (auto& message : m_messages)
				result &= message.send(zmqSocket);

			if (!result)
				CATAPULT_LOG(warning) << m_errorMessageGenerator();
		}

	private:
		supplier<std::string> m_errorMessageGenerator;
		std::vector<zmq::multipart_t> m_messages;
	};

	class ZeroMqEntityPublisher::SynchronizedPublisher {
	public:
		explicit SynchronizedPublisher(unsigned short port)
				: m_zmqSocket(m_zmqContext, ZMQ_PUB)
				, m_pPool(thread::CreateIoThreadPool(1, "ZeroMqEntityPublisher")) {
			// note that we want closing the socket to be synchronous
			// setting linger to 0 means that all pending messages are discarded and the socket is closed immediately
			m_zmqSocket.setsockopt(ZMQ_LINGER, 0);
			m_zmqSocket.bind("tcp://*:" + std::to_string(port));

			m_pPool->start();
		}

		~SynchronizedPublisher() {
			// stop the pool first to prevent any work from being written to (closed) socket
			m_pPool->join();
			m_zmqSocket.close();
		}

	public:
		void queue(std::unique_ptr<MessageGroup>&& pMessageGroup) {
			// dispatch function needs to be copyable
			auto pMessageGroupShared = std::shared_ptr<MessageGroup>(std::move(pMessageGroup));
			boost::asio::dispatch(m_pPool->ioContext(), [&zmqSocket = m_zmqSocket, pMessageGroup{std::move(pMessageGroupShared)}]() {
				pMessageGroup->flush(zmqSocket);
			});
		}

	private:
		zmq::context_t m_zmqContext;
		zmq::socket_t m_zmqSocket;
		std::unique_ptr<thread::IoThreadPool> m_pPool;
	};

	struct ZeroMqEntityPublisher::WeakTransactionInfo {
	public:
		explicit WeakTransactionInfo(const model::TransactionInfo& transactionInfo)
				: Transaction(*transactionInfo.pEntity)
				, EntityHash(transactionInfo.EntityHash)
				, MerkleComponentHash(transactionInfo.MerkleComponentHash)
				, OptionalAddresses(transactionInfo.OptionalExtractedAddresses.get())
				, AssociatedHeight(transactionInfo.AssociatedHeight)
		{}

		explicit WeakTransactionInfo(const model::TransactionElement& element, const Height& height)
				: Transaction(element.Transaction)
				, EntityHash(element.EntityHash)
				, MerkleComponentHash(element.MerkleComponentHash)
				, OptionalAddresses(element.OptionalExtractedAddresses.get())
				, AssociatedHeight(height)
		{}

		explicit WeakTransactionInfo(const model::Transaction& transaction, const Hash256& hash, const Height& height)
				: Transaction(transaction)
				, EntityHash(hash)
				, MerkleComponentHash(hash)
				, OptionalAddresses(nullptr)
				, AssociatedHeight(height)
		{}

	public:
		const model::Transaction& Transaction;
		const Hash256& EntityHash;
		const Hash256& MerkleComponentHash;
		const model::UnresolvedAddressSet* OptionalAddresses;
		const Height& AssociatedHeight;
	};

	ZeroMqEntityPublisher::ZeroMqEntityPublisher(
			unsigned short port,
			std::unique_ptr<model::NotificationPublisher>&& pNotificationPublisher,
			const model::ExtractorContextFactoryFunc & contextFactory)
			: m_pNotificationPublisher(std::move(pNotificationPublisher))
			, m_pSynchronizedPublisher(std::make_unique<SynchronizedPublisher>(port))
			, m_extractorContextFactory(contextFactory)
	{}

	ZeroMqEntityPublisher::~ZeroMqEntityPublisher() = default;

	namespace {
		auto CreateHeightMessageGenerator(const std::string& topicName, Height height) {
			return [topicName, height]() {
				std::ostringstream out;
				out << "cannot publish " << topicName << " at height: " << height;
				return out.str();
			};
		}
	}

	void ZeroMqEntityPublisher::publishBlockHeader(const model::BlockElement& blockElement) {
		auto pMessageGroup = std::make_unique<MessageGroup>(CreateHeightMessageGenerator("block header", blockElement.Block.Height));

		zmq::multipart_t multipart;
		auto marker = BlockMarker::Block_Marker;
		multipart.addmem(&marker, sizeof(marker));
		multipart.addmem(static_cast<const void*>(&blockElement.Block), sizeof(model::BlockHeader));
		multipart.addmem(static_cast<const void*>(&blockElement.EntityHash), Hash256_Size);
		multipart.addmem(static_cast<const void*>(&blockElement.GenerationHash), Hash256_Size);

		pMessageGroup->add(std::move(multipart));
		m_pSynchronizedPublisher->queue(std::move(pMessageGroup));

		if (blockElement.OptionalStatement) {
			const auto& statements = blockElement.OptionalStatement->PublicKeyStatements;
			for (const auto& pair : statements) {
				publishStatement(pair.second, blockElement.Block.Height);
			}
		}
	}

	void ZeroMqEntityPublisher::publishDropBlocks(Height height) {
		auto pMessageGroup = std::make_unique<MessageGroup>(CreateHeightMessageGenerator("drop blocks", height));

		zmq::multipart_t multipart;
		auto marker = BlockMarker::Drop_Blocks_Marker;
		multipart.addmem(&marker, sizeof(marker));
		multipart.addmem(static_cast<const void*>(&height), sizeof(Height));
		pMessageGroup->add(std::move(multipart));
		m_pSynchronizedPublisher->queue(std::move(pMessageGroup));
	}

	namespace {
		auto CreateHashMessageGenerator(const std::string& topicName, const Hash256& hash) {
			return [topicName, hash]() {
				std::ostringstream out;
				out << "cannot publish " << topicName << " with hash: " << hash;
				return out.str();
			};
		}
	}

	void ZeroMqEntityPublisher::publishTransaction(
			TransactionMarker topicMarker,
			const model::TransactionElement& transactionElement,
			Height height) {
		publishTransaction(topicMarker, WeakTransactionInfo(transactionElement, height));
	}

	void ZeroMqEntityPublisher::publishTransaction(
			TransactionMarker topicMarker,
			const model::TransactionInfo& transactionInfo) {
		publishTransaction(topicMarker, WeakTransactionInfo(transactionInfo));
	}

	void ZeroMqEntityPublisher::publishTransactionHash(TransactionMarker topicMarker, const model::TransactionInfo& transactionInfo) {
		const auto& hash = transactionInfo.EntityHash;
		publish("transaction hash", topicMarker, WeakTransactionInfo(transactionInfo), [&hash](auto& multipart) {
			multipart.addmem(static_cast<const void*>(&hash), Hash256_Size);
		});
	}

	void ZeroMqEntityPublisher::publishTransaction(
			TransactionMarker topicMarker,
			const WeakTransactionInfo& transactionInfo) {
		publish("transaction", topicMarker, transactionInfo, [&transactionInfo](auto& multipart) {
			const auto& transaction = transactionInfo.Transaction;
			multipart.addmem(static_cast<const void*>(&transaction), transaction.Size);
			multipart.addmem(static_cast<const void*>(&transactionInfo.EntityHash), Hash256_Size);
			multipart.addmem(static_cast<const void*>(&transactionInfo.MerkleComponentHash), Hash256_Size);
			multipart.addtyp(transactionInfo.AssociatedHeight);
		});
	}

	void ZeroMqEntityPublisher::publishTransactionStatus(const model::Transaction& transaction, const Height& height, const Hash256& hash, uint32_t status) {
		auto topicMarker = TransactionMarker::Transaction_Status_Marker;
		model::TransactionStatus transactionStatus(hash, status, transaction.Deadline);
		publish("transaction status", topicMarker, WeakTransactionInfo(transaction, hash, height), [&transactionStatus](auto& multipart) {
			multipart.addmem(static_cast<const void*>(&transactionStatus), sizeof(transactionStatus));
		});
	}

	void ZeroMqEntityPublisher::publishCosignature(
			const model::TransactionInfo& parentTransactionInfo,
			const Key& signer,
			const Signature& signature) {
		auto topicMarker = TransactionMarker::Cosignature_Marker;
		model::DetachedCosignature detachedCosignature(signer, signature, parentTransactionInfo.EntityHash);
		publish("detached cosignature", topicMarker, WeakTransactionInfo(parentTransactionInfo), [&detachedCosignature](auto& multipart) {
			multipart.addmem(static_cast<const void*>(&detachedCosignature), sizeof(detachedCosignature));
		});
	}

	void ZeroMqEntityPublisher::publishStatement(const model::PublicKeyStatement& statement, const Height& height) {
		TransactionMarker topic = TransactionMarker::Receipt_Marker;
		for (auto i = 0u; i < statement.size(); ++i){
			auto pMessageGroup = std::make_unique<MessageGroup>(CreateHeightMessageGenerator("publish statement", height));
			const auto& receipt = statement.receiptAt(i);
			zmq::multipart_t multipart;
			multipart.addmem(&topic, sizeof(TransactionMarker));
			multipart.addmem(&receipt, receipt.Size);
			pMessageGroup->add(std::move(multipart));
			m_pSynchronizedPublisher->queue(std::move(pMessageGroup));
		}
	}

	void ZeroMqEntityPublisher::publish(
			const std::string& topicName,
			TransactionMarker topicMarker,
			const WeakTransactionInfo& transactionInfo,
			const MessagePayloadBuilder& payloadBuilder) {
		auto pMessageGroup = std::make_unique<MessageGroup>(CreateHashMessageGenerator(topicName, transactionInfo.EntityHash));

		const auto& addresses = transactionInfo.OptionalAddresses
				? *transactionInfo.OptionalAddresses
				: model::ExtractAddresses(transactionInfo.Transaction, transactionInfo.EntityHash, transactionInfo.AssociatedHeight, *m_pNotificationPublisher, m_extractorContextFactory());

		if (addresses.empty())
			CATAPULT_LOG(warning) << "no addresses are associated with transaction " << transactionInfo.EntityHash;

		for (const auto& address : addresses) {
			zmq::multipart_t multipart;
			auto topic = CreateTopic(topicMarker, address);
			multipart.addmem(topic.data(), topic.size());
			payloadBuilder(multipart);
			pMessageGroup->add(std::move(multipart));
		}

		m_pSynchronizedPublisher->queue(std::move(pMessageGroup));
	}
}}
