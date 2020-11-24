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
			Height MinHeight;
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

			Height minHeight(licenseJson.get<uint64_t>("minheight"));
			Height maxHeight(licenseJson.get<uint64_t>("maxheight"));

			auto buffer = network + node + std::to_string(minHeight.unwrap()) + std::to_string(maxHeight.unwrap());
			if (!crypto::Verify(licenseKey, { reinterpret_cast<const uint8_t*>(buffer.data()), buffer.size() }, signature)) {
				CATAPULT_THROW_RUNTIME_ERROR("invalid license signature")
			}

			return std::make_unique<License>(License{ minHeight, maxHeight });
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
				, m_node(crypto::FormatKeyAsString(crypto::KeyPair::FromString(pConfigHolder->Config().User.BootKey).publicKey()))
				, m_licenseKey(crypto::ParseKey(licenseKey))
				, m_licenseFile(boost::filesystem::path(pConfigHolder->Config().User.DataDirectory) / License_File)
				, m_pLicense(ReadLicense(m_licenseFile, crypto::FormatKeyAsString(pConfigHolder->Config().Network.Info.PublicKey), m_node, m_licenseKey))
				, m_licensingConfig(licensingConfig)
			{}

		public:
			bool blockGeneratingAllowedAt(const Height& height) const override {
				std::lock_guard<std::mutex> lock(m_mutex);

				if (m_pLicense && (m_pLicense->MinHeight <= height && height <= m_pLicense->MaxHeight))
					return true;

				requestLicense();

				return m_pLicense && (m_pLicense->MinHeight <= height && height <= m_pLicense->MaxHeight);
			}

			bool blockConsumingAllowedAt(const Height& height) const override {
				std::lock_guard<std::mutex> lock(m_mutex);

				if (m_pLicense && (height <= m_pLicense->MaxHeight))
					return true;

				requestLicense();

				return m_pLicense && (height <= m_pLicense->MaxHeight);
			}

		private:
			void requestLicense() const {
				try {
					boost::asio::io_context io_context;
					boost::system::error_code err;

					boost::asio::ip::tcp::resolver resolver(io_context);
					auto iter = resolver.resolve(
						m_licensingConfig.LicenseServerHost, std::to_string(m_licensingConfig.LicenseServerPort), err);
					if (err) {
						CATAPULT_LOG(warning) << "couldn't resolve address " << m_licensingConfig.LicenseServerHost << ": " << err;
						return;
					}

					boost::asio::ip::tcp::socket socket(io_context);
					socket.connect(iter->endpoint(), err);
					if (err) {
						CATAPULT_LOG(warning) << "couldn't connect to the license server: " << err;
						return;
					}

					boost::property_tree::ptree requestJson;
					auto network = crypto::FormatKeyAsString(m_pConfigHolder->Config().Network.Info.PublicKey);
					requestJson.add("network", network);
					requestJson.add("node", m_node);
					std::ostringstream out;
					boost::property_tree::write_json(out, requestJson);
					auto request = out.str();

					boost::asio::write(socket, boost::asio::buffer(request), err);
					if (err) {
						CATAPULT_LOG(warning) << "couldn't send request to the license server: " << err;
						return;
					}

					boost::asio::streambuf buffer;
					boost::asio::read(socket, buffer, boost::asio::transfer_all(), err);
					if (err && err != boost::asio::error::eof) {
						CATAPULT_LOG(warning) << "couldn't receive response from the license server: " << err;
						return;
					}

					const char* response = boost::asio::buffer_cast<const char*>(buffer.data());
					boost::property_tree::ptree responseJson;
					std::istringstream in(response);
					boost::property_tree::read_json(in, responseJson);

					auto failure = responseJson.get_child_optional("failure");
					if (failure.has_value()) {
						auto description = failure->get<std::string>("description");
						CATAPULT_LOG(warning) << "failed to get license: " << description;
						return;
					}

					auto licenseJson = responseJson.get_child("license");

					m_pLicense = ReadLicense(licenseJson, network, m_node, m_licenseKey);

					boost::property_tree::write_json(m_licenseFile.generic_string(), responseJson);

				} catch (const std::exception& err) {
					CATAPULT_LOG(warning) << err.what();
					return;
				}
			}

		private:
			std::shared_ptr<config::BlockchainConfigurationHolder> m_pConfigHolder;
			std::string m_node;
			Key m_licenseKey;
			boost::filesystem::path m_licenseFile;
			mutable std::unique_ptr<License> m_pLicense;
			LicensingConfiguration m_licensingConfig;
			mutable std::mutex m_mutex;
		};
	}

	std::shared_ptr<LicenseManager> CreateDefaultLicenseManager(
			const std::string& licenseKey,
			const LicensingConfiguration& licensingConfig,
			const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) {
		return std::make_shared<DefaultLicenseManager>(licenseKey, licensingConfig, pConfigHolder);
	}
}}
