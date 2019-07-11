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

#include "ServerMain.h"
#include "catapult/extensions/LocalNodeBootstrapper.h"
#include "catapult/local/BasicLocalNode.h"

int main(int argc, const char** argv) {
	using namespace catapult;
	return server::ServerMain(argc, argv, [argc, argv](const auto& pConfigHolder, const auto& keyPair) {
		// create bootstrapper
		auto resourcesPath = config::LocalNodeConfigurationHolder::GetResourcesPath(argc, argv).generic_string();
		auto pBootstrapper = std::make_unique<extensions::LocalNodeBootstrapper>(pConfigHolder, resourcesPath, "server");

		// register extension(s)
		pBootstrapper->loadExtensions();

		// create the local node
		return local::CreateBasicLocalNode(keyPair, std::move(pBootstrapper));
	});
}
