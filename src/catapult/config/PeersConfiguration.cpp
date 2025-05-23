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

#include "PeersConfiguration.h"
#include "catapult/crypto/KeyUtils.h"

#ifdef _MSC_VER
#include <boost/config/compiler/visualc.hpp>
#endif

#include <boost/property_tree/json_parser.hpp>

namespace pt = boost::property_tree;

namespace catapult { namespace config {

	namespace {
		template<typename T>
		auto GetOptional(const pt::ptree& tree, const std::string& key) {
			auto value = tree.get_optional<T>(key);
			return value.has_value() ? value.value() : T();
		}

		template<typename T>
		auto Get(const pt::ptree& tree, const std::string& key) {
			// use get_optional instead of get in order to allow better error messages to propagate out
			auto value = tree.get_optional<T>(key);
			if (!value.has_value()) {
				std::ostringstream message;
				message << "required property '" << key << "' was not found in json";
				CATAPULT_THROW_RUNTIME_ERROR(message.str().c_str());
			}

			return value.value();
		}

		auto GetChild(const pt::ptree& tree, const std::string& key) {
			// use get_child_optional instead of get_child in order to allow better error messages to propagate out
			auto value = tree.get_child_optional(key);
			if (!value.has_value()) {
				std::ostringstream message;
				message << "required child '" << key << "' was not found in json";
				CATAPULT_THROW_RUNTIME_ERROR(message.str().c_str());
			}

			return value.value();
		}

		ionet::NodeRoles ParseRoles(const std::string& str) {
			ionet::NodeRoles roles;
			if (!ionet::TryParseValue(str, roles)) {
				std::ostringstream message;
				message << "roles property has unsupported value: " << str;
				CATAPULT_THROW_RUNTIME_ERROR(message.str().c_str());
			}

			return roles;
		}

		std::vector<ionet::Node> LoadPeersFromProperties(const pt::ptree& properties, model::NetworkIdentifier networkIdentifier, bool dbrbPortOptional) {
			if (!GetOptional<std::string>(properties, "knownPeers").empty())
				CATAPULT_THROW_RUNTIME_ERROR("knownPeers must be an array");

			std::vector<ionet::Node> peers;
			for (const auto& peerJson : GetChild(properties, "knownPeers")) {
				const auto& endpointJson = GetChild(peerJson.second, "endpoint");
				const auto& metadataJson = GetChild(peerJson.second, "metadata");

				auto identityKey = crypto::ParseKey(Get<std::string>(peerJson.second, "publicKey"));
				auto dbrbPort = dbrbPortOptional ? GetOptional<unsigned short>(endpointJson, "dbrbPort") : Get<unsigned short>(endpointJson, "dbrbPort");
				auto endpoint = ionet::NodeEndpoint{ Get<std::string>(endpointJson, "host"), Get<unsigned short>(endpointJson, "port"), dbrbPort };
				auto metadata = ionet::NodeMetadata(networkIdentifier, GetOptional<std::string>(metadataJson, "name"));
				metadata.Roles = ParseRoles(Get<std::string>(metadataJson, "roles"));
				peers.push_back({ identityKey, endpoint, metadata });
			}

			return peers;
		}
	}

	std::vector<ionet::Node> LoadPeersFromStream(std::istream& input, model::NetworkIdentifier networkIdentifier, bool dbrbPortOptional) {
		pt::ptree properties;
		pt::read_json(input, properties);
		return LoadPeersFromProperties(properties, networkIdentifier, dbrbPortOptional);
	}

	std::vector<ionet::Node> LoadPeersFromPath(const std::string& path, model::NetworkIdentifier networkIdentifier, bool dbrbPortOptional) {
		std::ifstream inputStream(path);
		return LoadPeersFromStream(inputStream, networkIdentifier, dbrbPortOptional);
	}
}}
