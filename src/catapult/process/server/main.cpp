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

#include "catapult/extensions/ProcessBootstrapper.h"
#include "catapult/local/server/LocalNode.h"
#include "catapult/process/ProcessMain.h"
#include "catapult/config/ValidateConfiguration.h"

namespace {
	constexpr auto Process_Name = "server";
}

int main(int argc, const char** argv) {
	using namespace catapult;
	return process::ProcessMain(argc, argv, Process_Name,  [](const boost::filesystem::path& path, const std::string& host){
				auto config = config::BlockchainConfiguration::LoadFromPath(path, host);
				auto pConfigHolder = std::make_shared<config::BlockchainConfigurationHolder>(config);
				config::ValidateLocalConfiguration(config);
				return pConfigHolder;
			},
			[argc, argv](const auto& pConfigHolder, const auto& keyPair) {
		// create bootstrapper
		auto resourcesPath = config::BlockchainConfigurationHolder::GetResourcesPath(argc, argv).generic_string();
		auto disposition = extensions::ProcessDisposition::Production;
		auto pBootstrapper = std::make_unique<extensions::ProcessBootstrapper>(pConfigHolder, resourcesPath, disposition, Process_Name);

		// register extension(s)
		pBootstrapper->loadExtensions();

		// create the local node
		return local::CreateLocalNode(keyPair, std::move(pBootstrapper));
	});
}
