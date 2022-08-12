/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "SupportedEntityVersions.h"
#include <boost/property_tree/json_parser.hpp>

namespace catapult { namespace config {

	namespace {
		SupportedEntityVersions LoadSupportedEntityVersions(boost::property_tree::ptree root) {
			SupportedEntityVersions supportedEntityVersions;

			auto entitiesJson = root.get_child("entities");

			for (boost::property_tree::ptree::value_type& entityJson : entitiesJson) {
				auto& entity = entityJson.second;
				auto entityType = static_cast<model::EntityType>(entity.get<uint16_t>("type"));

				for (boost::property_tree::ptree::value_type& versionJson : entity.get_child("supportedVersions")) {
					supportedEntityVersions[entityType].insert(versionJson.second.get_value<VersionType>());
				}
			}

			return supportedEntityVersions;
		}
	}

	SupportedEntityVersions LoadSupportedEntityVersions(const boost::filesystem::path& path) {
		if (!boost::filesystem::exists(path)) {
			auto message = "aborting load due to missing configuration file";
			CATAPULT_LOG(fatal) << message << ": " << path;
			CATAPULT_THROW_EXCEPTION(catapult_runtime_error(message));
		}

		boost::property_tree::ptree root;
		boost::property_tree::read_json(path.generic_string(), root);

		return LoadSupportedEntityVersions(root);
	}

	SupportedEntityVersions LoadSupportedEntityVersions(std::istream& inputStream) {
		boost::property_tree::ptree root;
		boost::property_tree::read_json(inputStream, root);

		return LoadSupportedEntityVersions(root);
	}
}}
