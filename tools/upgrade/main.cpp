/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/builders/NetworkConfigBuilder.h"
#include "catapult/builders/BlockchainUpgradeBuilder.h"
#include "catapult/crypto/KeyPair.h"
#include "catapult/crypto/KeyUtils.h"
#include "catapult/extensions/TransactionExtensions.h"
#include "catapult/ionet/Node.h"
#include "catapult/ionet/PacketPayloadFactory.h"
#include "catapult/ionet/PacketSocket.h"
#include "catapult/net/VerifyPeer.h"
#include "catapult/utils/FileSize.h"
#include "catapult/utils/HexParser.h"
#include "catapult/utils/NetworkTime.h"
#include "tools/ToolMain.h"
#include <boost/algorithm/string/split.hpp>
#include <boost/filesystem.hpp>

#include <list>

namespace catapult { namespace tools { namespace upgrade {

	namespace {

	class BlockchainUpgradeTool : public Tool, public std::enable_shared_from_this<BlockchainUpgradeTool> {
		public:
			std::string name() const override {
				return "Blockchain Upgrade Tool";
			}

			void prepareOptions(OptionsBuilder& optionsBuilder, OptionsPositional&) override {
				optionsBuilder("signer-key,s",
					OptionsValue<std::string>(m_signerKey)->required(),
					"nemesis private key required to sign transaction (no default)");

				optionsBuilder("generation-hash,g",
					OptionsValue<std::string>(m_generationHash)->required(),
					"nemesis generation hash required to sign transaction (no default)");

				optionsBuilder("network,n",
					OptionsValue<std::string>(m_network)->default_value("mijin-test"),
					"network identifier {mijin, mijin-test, private, private-test, public, public-test} (default: mijin-test)");

				optionsBuilder("bc-upgrade,b",
					OptionsSwitch(m_blockChainUpgrade),
					"flag to generate blockchain upgrade transaction");

				optionsBuilder("bc-upgrade-apply-height-delta,u",
					OptionsValue<uint64_t>(m_upgradeApplyHeightDelta)->default_value(360),
					"number of blocks since current chain height to apply blockchain upgrade (default: 360)");

				optionsBuilder("bc-version,v",
					OptionsValue<std::string>(m_blockChainVersion)->default_value("0.0.0.0"),
					"new blockchain version (default: 0.0.0.0)");

				optionsBuilder("config-update,c",
					OptionsSwitch(m_configUpdate),
					"flag to generate blockchain config update transaction");

				optionsBuilder("resources,r",
					OptionsValue<std::string>(m_resourcesPath)->default_value("../resources"),
					"path to the resources directory (default: ../resources)");

				optionsBuilder("config-apply-height-delta,d",
					OptionsValue<uint64_t>(m_configApplyHeightDelta)->default_value(360),
					"number of blocks since current chain height to apply config update (default: 360)");

				optionsBuilder("host,h",
					OptionsValue<std::string>(m_host)->default_value("127.0.0.1"),
					"host to connect to (default: 127.0.0.1)");

				optionsBuilder("port,p",
					OptionsValue<uint16_t>(m_port)->default_value(3000),
					"host port (default: 3000)");

				optionsBuilder("host-type,t",
					OptionsValue<std::string>(m_hostType)->default_value("rest"),
					"type of host {api, rest} (default: rest)");

				optionsBuilder("api-key,a",
					OptionsValue<std::string>(m_apiKey)->default_value(""),
					"public key of api node to connect to (default: <empty>)");

				optionsBuilder("rest-key,k",
					OptionsValue<std::string>(m_restKey)->default_value(""),
					"private key required for api node could verify connection (default: <empty>)");
			}

			int run(const Options&) override {
				try {
					sendTransactions();
				} catch (...) {
					return 1;
				}

				return 0;
			}

		private:
			template<typename TBuilder>
			std::shared_ptr<model::Transaction> generateTransaction(TBuilder& builder, crypto::KeyPair& signer) {
				builder.setDeadline(Timestamp(utils::NetworkTime().unwrap() + 60 * 60 * 1000));
				GenerationHash generationHash;
				utils::ParseHexStringIntoContainer(m_generationHash.data(), m_generationHash.size(), generationHash);
				auto pTransaction = builder.build();
				extensions::TransactionExtensions(generationHash).sign(signer, *pTransaction);

				return pTransaction;
			}

			std::shared_ptr<model::Transaction> generateBlockchainUpgradeTransaction(model::NetworkIdentifier networkIdentifier, crypto::KeyPair& signer) {
				builders::BlockchainUpgradeBuilder builder(networkIdentifier, signer.publicKey());
				builder.setUpgradePeriod(BlockDuration{m_upgradeApplyHeightDelta});
				std::vector<std::string> parts;
				boost::algorithm::split(parts, m_blockChainVersion, [](char ch) { return ch == '.'; });
				BlockchainVersion version = BlockchainVersion{
					boost::lexical_cast<uint64_t >(parts[0]) << 48 |
					boost::lexical_cast<uint64_t >(parts[1]) << 32 |
					boost::lexical_cast<uint64_t >(parts[2]) << 16 |
					boost::lexical_cast<uint64_t >(parts[3])
				};
				builder.setNewBlockchainVersion(version);

				return generateTransaction(builder, signer);
			}

			std::shared_ptr<model::Transaction> generateConfigTransaction(model::NetworkIdentifier networkIdentifier, crypto::KeyPair& signer) {
				builders::NetworkConfigBuilder builder(networkIdentifier, signer.publicKey());
				builder.setApplyHeightDelta(BlockDuration{m_configApplyHeightDelta});
				auto resourcesPath = boost::filesystem::path(m_resourcesPath);
				builder.setBlockChainConfig((resourcesPath / "config-network.properties").generic_string());
				builder.setSupportedVersionsConfig((resourcesPath / "supported-entities.json").generic_string());

				return generateTransaction(builder, signer);
			}

			void sendTransactions() {
				model::NetworkIdentifier networkIdentifier;
				if (!model::TryParseValue(m_network, networkIdentifier))
					CATAPULT_THROW_INVALID_ARGUMENT_1("wrong network identifier", m_network);
				auto signer = crypto::KeyPair::FromString(m_signerKey);
				std::list<std::shared_ptr<model::Transaction>> transactions;

				if (m_blockChainUpgrade) {
					transactions.push_back(generateBlockchainUpgradeTransaction(networkIdentifier, signer));
				}

				if (m_configUpdate) {
					transactions.push_back(generateConfigTransaction(networkIdentifier, signer));
				}

				if (m_hostType == "api") {
					sendTransactionsToApiNode(transactions);
				} else if (m_hostType == "rest") {
					sendTransactionsToRestServer(transactions);
				} else {
					CATAPULT_THROW_INVALID_ARGUMENT_1("wrong host type", m_hostType);
				}
			}

			void writeTransactionsToApiSocket(
				std::shared_ptr<ionet::PacketSocket> pConnectedSocket,
				std::list<std::shared_ptr<model::Transaction>>& transactions,
				boost::asio::executor_work_guard<boost::asio::io_service::executor_type>& guard) {
				CATAPULT_LOG(info) << "writing transaction " << transactions.front()->Type;
				auto packet = ionet::PacketPayloadFactory::FromEntity(ionet::PacketType::Push_Transactions, transactions.front());
				transactions.pop_front();
				pConnectedSocket->write(packet, [this, pConnectedSocket, &transactions, &guard](auto code) {
					if (code != catapult::ionet::SocketOperationCode::Success)
						CATAPULT_THROW_RUNTIME_ERROR_1("failed to send transaction to api node", code);

					if (transactions.size()) {
						writeTransactionsToApiSocket(pConnectedSocket, transactions, guard);
					} else {
						guard.reset();
					}
				});
			}

			void sendTransactionsToApiNode(std::list<std::shared_ptr<model::Transaction>> transactions) {
				boost::asio::io_service service;
				boost::asio::executor_work_guard guard(boost::asio::make_work_guard(service));

				crypto::KeyPair keyPair = crypto::KeyPair::FromString(m_restKey);
				net::VerifiedPeerInfo serverPeerInfo;
				serverPeerInfo.PublicKey = crypto::ParseKey(m_apiKey);
				serverPeerInfo.SecurityMode = ionet::ConnectionSecurityMode::None;

				catapult::ionet::NodeEndpoint endpoint;
				endpoint.Host = m_host;
				endpoint.Port = m_port;

				catapult::ionet::PacketSocketOptions packetSocketOptions;
				packetSocketOptions.WorkingBufferSize = catapult::utils::FileSize::FromKilobytes(512).bytes();
				packetSocketOptions.WorkingBufferSensitivity = 100;
				packetSocketOptions.MaxPacketDataSize = catapult::utils::FileSize::FromMegabytes(5).bytes();

				auto cancel = ionet::Connect(
					service,
					packetSocketOptions,
					endpoint,
					[&](auto connectResult, const std::shared_ptr<ionet::PacketSocket>& pConnectedSocket) {
						if (connectResult != ionet::ConnectResult::Connected)
							CATAPULT_THROW_RUNTIME_ERROR_1("failed to connect to api node", connectResult);

						net::VerifyServer(pConnectedSocket, serverPeerInfo, keyPair, [this, pConnectedSocket, &transactions, &guard](auto verifyResult, const auto&) {
							if (verifyResult != net::VerifyResult::Success)
								CATAPULT_THROW_RUNTIME_ERROR_1("failed to verify api node", verifyResult);

							writeTransactionsToApiSocket(pConnectedSocket, transactions, guard);
						});
					}
				);

				service.run();
			}

			void writeTransactionsToRestSocket(
					boost::asio::ip::tcp::socket& socket,
					std::list<std::shared_ptr<model::Transaction>>& transactions,
					boost::asio::executor_work_guard<boost::asio::io_service::executor_type>& guard) {
				auto packet = ionet::PacketPayloadFactory::FromEntity(ionet::PacketType::Push_Transactions, transactions.front());
				transactions.pop_front();
				const uint8_t* data = packet.buffers()[0].pData;
				unsigned long dataSize = packet.buffers()[0].Size;

				std::stringstream jsonStream;
				jsonStream << "{\"payload\":\"";
				for (uint32_t j = 0; j < dataSize; ++j, ++data) {
					if (!(*data & 0xF0)) {
						jsonStream << std::hex << 0;
					}
					jsonStream << std::hex << *data;
				}
				jsonStream << "\"}";

				std::string json = jsonStream.str();

				boost::asio::streambuf request;
				std::ostream request_stream(&request);
				request_stream << "PUT " << "/transaction" << " HTTP/1.0\r\n";
				request_stream << "Host: " << m_host << ":" << m_port << "\r\n";
				request_stream << "Content-Type: " << "application/json" << "\r\n";
				request_stream << "Content-Length: " << json.size() << "\r\n";
				request_stream << "Connection: Keep-Alive\r\n";
				request_stream << "Keep-Alive: timeout=100, max=100000\r\n\r\n";
				request_stream << json << "\r\n\r\n";

				socket.async_send(boost::asio::buffer(request.data(), request.size()), 0, [&](auto& sendError, auto&) {
					if (sendError) {
						CATAPULT_THROW_RUNTIME_ERROR_1("failed to send transaction to rest", sendError);
					}

					char response[1024];
					boost::system::error_code receiveError;
					socket.receive(boost::asio::buffer(response), {}, receiveError);
					if (receiveError) {
						CATAPULT_THROW_RUNTIME_ERROR_1("failed to receive response from rest", receiveError);
					}

					if (transactions.size()) {
						writeTransactionsToRestSocket(socket, transactions, guard);
					} else {
						guard.reset();
					}
				});
			}

			void sendTransactionsToRestServer(std::list<std::shared_ptr<model::Transaction>> transactions) {
				boost::asio::io_service service;
				boost::asio::executor_work_guard guard(boost::asio::make_work_guard(service));
				boost::asio::ip::tcp::socket socket(service);
				socket.open(boost::asio::ip::tcp::v4());
				socket.set_option(boost::asio::ip::tcp::no_delay(false));
				socket.set_option(boost::asio::socket_base::keep_alive(true));
				socket.set_option(boost::asio::socket_base::reuse_address(true));
				boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(m_host), m_port);
				socket.connect(endpoint);

				writeTransactionsToRestSocket(socket, transactions, guard);

				service.run();
			}

		private:
			std::string m_signerKey;
			std::string m_network;
			bool m_blockChainUpgrade;
			std::string m_blockChainVersion;
			uint64_t m_upgradeApplyHeightDelta;
			bool m_configUpdate;
			std::string m_resourcesPath;
			std::string m_generationHash;
			uint64_t m_configApplyHeightDelta;
			std::string m_hostType;
			std::string m_host;
			uint16_t m_port;
			std::string m_apiKey;
			std::string m_restKey;
		};
	}
}}}

int main(int argc, const char** argv) {
	catapult::tools::upgrade::BlockchainUpgradeTool tool;
	return catapult::tools::ToolMain(argc, argv, tool);
}
