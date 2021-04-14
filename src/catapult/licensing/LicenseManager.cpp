/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "LicenseManager.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "catapult/crypto/KeyUtils.h"
#include "catapult/crypto/Signer.h"
#include "catapult/utils/HexParser.h"
#include <boost/asio.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace catapult { namespace licensing {

	namespace {
		constexpr auto License_File = "license.json";

		struct License {
			Height MaxHeight;
		};

		std::unique_ptr<License> ReadLicense(
				const boost::property_tree::ptree& licenseJson,
				const std::string& expectedNetwork,
				const std::string& expectedNode,
				const Key& licenseKey) {
			auto network = licenseJson.get<std::string>("network");
			if (expectedNetwork != network) {
				CATAPULT_THROW_RUNTIME_ERROR_2("invalid network in license (expected, actual)", expectedNetwork, network)
			}

			auto node = licenseJson.get<std::string>("node");
			if (expectedNode != node) {
				CATAPULT_THROW_RUNTIME_ERROR_2("invalid node in license (expected, actual)", expectedNode, node)
			}

			auto signatureStr = licenseJson.get<std::string>("signature");
			Signature signature;
			if (!utils::TryParseHexStringIntoContainer<Signature>(signatureStr.data(), signatureStr.size(), signature)) {
				CATAPULT_THROW_RUNTIME_ERROR("error parsing license signature")
			}

			Height maxHeight(licenseJson.get<uint64_t>("maxheight"));

			auto buffer = network + node + std::to_string(maxHeight.unwrap());
			if (!crypto::Verify(licenseKey, { reinterpret_cast<const uint8_t*>(buffer.data()), buffer.size() }, signature)) {
				CATAPULT_THROW_RUNTIME_ERROR("invalid license signature")
			}

			return std::make_unique<License>(License{ maxHeight });
		}

		std::unique_ptr<License> ReadLicense(
				const boost::filesystem::path& licenseFile,
				const std::string& expectedNetwork,
				const std::string& expectedNode,
				const Key& licenseKey) {
			if (!boost::filesystem::exists(licenseFile)) {
				CATAPULT_LOG(warning) << "license file not found: " << licenseFile;
				return nullptr;
			}

			try {
				boost::property_tree::ptree licenseJson;
				boost::property_tree::read_json(licenseFile.generic_string(), licenseJson);

				return ReadLicense(licenseJson, expectedNetwork, expectedNode, licenseKey);

			} catch (const std::exception& err) {
				CATAPULT_LOG(warning) << "error parsing license file: " << err.what();
			}

			return nullptr;
		}

		struct DefaultLicenseManager : public LicenseManager {
		public:
			explicit DefaultLicenseManager(
					const std::string& licenseKey,
					const LicensingConfiguration& licensingConfig,
					const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder)
				: m_pConfigHolder(pConfigHolder)
				, m_licenseKey(crypto::ParseKey(licenseKey))
				, m_node(crypto::FormatKeyAsString(crypto::KeyPair::FromString(pConfigHolder->Config().User.BootKey).publicKey()))
				, m_licenseFile(boost::filesystem::path(pConfigHolder->Config().User.DataDirectory) / License_File)
				, m_pLicense(ReadLicense(m_licenseFile, crypto::FormatKeyAsString(pConfigHolder->Config().Network.Info.PublicKey), m_node, m_licenseKey))
				, m_serverHost(licensingConfig.LicenseServerHost)
				, m_serverPort(std::to_string(licensingConfig.LicenseServerPort))
				, m_licenseRequestTimeout(std::chrono::milliseconds(licensingConfig.LicenseRequestTimeout.millis()))
			{}

		public:
			bool blockAllowedAt(const Height& height) override {
				std::lock_guard<std::mutex> lock(m_mutex);

				const Height maxRollbackBlocks { m_pConfigHolder->Config().Network.MaxRollbackBlocks };
				const Height currentHeight = height <= maxRollbackBlocks
					? Height(1)
					: height - maxRollbackBlocks;

				if (!m_pLicense) {
					if (currentHeight == Height(1)) {
						auto blockElement = m_supplier(Height(1));
						requestLicense(Height(1), blockElement->Block.StateHash);
					} else {
						const Height closestHeight = requestClosestHeight(currentHeight);
						// if a license server responded with a failure
						if (closestHeight > Height(0)) {
							auto blockElement = m_supplier(closestHeight);
							requestLicense(closestHeight, blockElement->Block.StateHash);
						}
					}
				}

				if (m_pLicense && currentHeight == m_pLicense->MaxHeight) {
					auto blockElement = m_supplier(currentHeight);
					requestLicense(currentHeight, blockElement->Block.StateHash);
				}

				return m_pLicense && (currentHeight < m_pLicense->MaxHeight);
			}

			void setBlockElementSupplier(BlockElementSupplier supplier) override {
				m_supplier = supplier;
			}

		private:
			using RequestCallback = const std::function<void(bool, const boost::property_tree::ptree&)>;

			Height requestClosestHeight(const Height& height) {
				boost::property_tree::ptree requestJson = createRequest(height);

				std::ostringstream out;
				boost::property_tree::write_json(out, requestJson);
				auto request = out.str();

				Height closestHeight(0);
				doRequest(request, [this, &closestHeight](bool isSuccess, const boost::property_tree::ptree& responseJson) {
					if (isSuccess) {
						closestHeight = Height(responseJson.get<uint64_t>("height"));
					}
				});

				return closestHeight;
			}

			void requestLicense(const Height& height, const Hash256& stateHash) {
				boost::property_tree::ptree requestJson = createRequest(height);
				requestJson.add("stateHash", utils::HexFormat(stateHash.begin(), stateHash.end()));

				std::ostringstream out;
				boost::property_tree::write_json(out, requestJson);
				auto request = out.str();

				doRequest(request, [this](bool isSuccess, const boost::property_tree::ptree& responseJson) {
					if (isSuccess) {
						try {
							auto licenseJson = responseJson.get_child("license");

							m_pLicense = ReadLicense(licenseJson, m_network, m_node, m_licenseKey);

							boost::property_tree::write_json(m_licenseFile.generic_string(), responseJson);
						} catch (const std::exception& err) {
							CATAPULT_LOG(warning) << "failed to parse license server response: " << err.what();
						}
					}
				});
			}

			boost::property_tree::ptree createRequest(const Height& height) {
				boost::property_tree::ptree json;
				m_network = crypto::FormatKeyAsString(m_pConfigHolder->Config().Network.Info.PublicKey);
				json.add("network", m_network);
				json.add("node", m_node);
				json.add("height", height);

				return json;
			}

			void doRequest(const std::string& request, RequestCallback callback) {
				boost::asio::io_context io_context;
				boost::asio::ip::tcp::resolver resolver(io_context);
				m_pSocket = std::make_unique<boost::asio::ip::tcp::socket>(io_context);
				m_pBuffer = std::make_unique<boost::asio::streambuf>();

				resolver.async_resolve(m_serverHost, m_serverPort,
									   boost::bind(
											   &DefaultLicenseManager::handleResolve,
											   this,
											   boost::asio::placeholders::error,
											   boost::asio::placeholders::iterator,
											   request,
											   callback));

				io_context.run_for(m_licenseRequestTimeout);

				m_pSocket.reset(nullptr);
				m_pBuffer.reset(nullptr);
			}

			void handleResolve(const boost::system::error_code& err,
							   const boost::asio::ip::tcp::resolver::results_type& iter,
							   const std::string& request,
							   RequestCallback callback) {
				if (err) {
					CATAPULT_LOG(warning) << "couldn't resolve address " << m_serverHost << ": " << err;
					callback(false, boost::property_tree::ptree());
					return;
				}

				m_pSocket->async_connect(iter->endpoint(),
					boost::bind(
						&DefaultLicenseManager::handleConnect,
						this,
						boost::asio::placeholders::error,
						request,
						callback));
			}

			void handleConnect(const boost::system::error_code& err,
							   const std::string& request,
							   RequestCallback callback) {
				if (err) {
					CATAPULT_LOG(warning) << "couldn't connect to the license server: " << err;
					callback(false, boost::property_tree::ptree());
					return;
				}

				boost::asio::async_write(*m_pSocket, boost::asio::buffer(request),
					boost::bind(
						&DefaultLicenseManager::handleWrite,
						this,
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred,
						callback));
			}

			void handleWrite(const boost::system::error_code& err, size_t, RequestCallback callback) {
				if (err) {
					CATAPULT_LOG(warning) << "couldn't send request to the license server: " << err;
					callback(false, boost::property_tree::ptree());
					return;
				}

				boost::asio::async_read(*m_pSocket, *m_pBuffer, boost::asio::transfer_all(),
					boost::bind(
						&DefaultLicenseManager::handleRead,
						this,
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred,
						callback));
			}

			void handleRead(const boost::system::error_code& err, size_t, RequestCallback callback) {
				if (err && err != boost::asio::error::eof) {
					CATAPULT_LOG(warning) << "couldn't receive response from the license server: " << err;
					callback(false, boost::property_tree::ptree());
					return;
				}

				auto response = std::string(boost::asio::buffer_cast<const char*>(m_pBuffer->data()));
				boost::property_tree::ptree responseJson;
				std::istringstream in(response);
				boost::property_tree::read_json(in, responseJson);

				auto failure = responseJson.get_child_optional("failure");
				if (failure.has_value()) {
					auto description = failure->get<std::string>("description");
					CATAPULT_LOG(warning) << "license server responded with error: " << description;
					callback(false, responseJson);
				} else {
					callback(true, responseJson);
				}
			}

		private:
			std::shared_ptr<config::BlockchainConfigurationHolder> m_pConfigHolder;

			boost::filesystem::path m_licenseFile;
			Key m_licenseKey;

			std::string m_network;
			std::string m_node;

			std::unique_ptr<License> m_pLicense;

			std::string m_serverHost;
			std::string m_serverPort;
			std::chrono::milliseconds m_licenseRequestTimeout;

			std::unique_ptr<boost::asio::ip::tcp::socket> m_pSocket;
			std::unique_ptr<boost::asio::streambuf> m_pBuffer;

			std::mutex m_mutex;

			BlockElementSupplier m_supplier;
		};
	}

	std::shared_ptr<LicenseManager> CreateDefaultLicenseManager(
			const std::string& licenseKey,
			const LicensingConfiguration& licensingConfig,
			const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) {
		return std::make_shared<DefaultLicenseManager>(licenseKey, licensingConfig, pConfigHolder);
	}
}}
