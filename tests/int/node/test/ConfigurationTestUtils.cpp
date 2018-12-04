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

#include "ConfigurationTestUtils.h"
#include "catapult/utils/Logging.h"
#include <boost/filesystem/path.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>

namespace pt = boost::property_tree;

namespace catapult { namespace test {

	namespace {
		std::string CopyFile(const boost::filesystem::path& destinationPath, const std::string& filename) {
			auto sourceFilePath = boost::filesystem::path("..") / "resources" / filename;
			auto destinationFilePath = destinationPath / filename;
			boost::filesystem::copy_file(sourceFilePath, destinationFilePath);
			return destinationFilePath.generic_string();
		}

		void SetAutoHarvesting(const std::string& configFilePath) {
			pt::ptree properties;
			pt::read_ini(configFilePath, properties);
			properties.put("harvesting.isAutoHarvestingEnabled", true);
			properties.put("harvesting.harvestKey", "3485D98EFD7EB07ADAFCFD1A157D89DE2796A95E780813C0258AF3F5F84ED8CB");
			pt::write_ini(configFilePath, properties);
		}
	}

	void PrepareConfiguration(const std::string& destination, NodeFlag nodeFlag) {
		auto destinationResourcesPath = boost::filesystem::path(destination) / "resources";
		boost::filesystem::create_directories(destinationResourcesPath);
		CopyFile(destinationResourcesPath, "config-networkheight.properties");
		CopyFile(destinationResourcesPath, "config-task.properties");

		// don't copy the harvesting configuration if an api node is being simulated
		if (HasFlag(NodeFlag::Simulated_Api, nodeFlag))
			return;

		auto harvestingConfigFilePath = CopyFile(destinationResourcesPath, "config-harvesting.properties");
		if (HasFlag(NodeFlag::Auto_Harvest, nodeFlag))
			SetAutoHarvesting(harvestingConfigFilePath);
	}
}}
