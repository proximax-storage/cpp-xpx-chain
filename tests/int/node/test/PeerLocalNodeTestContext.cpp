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

#include "PeerLocalNodeTestContext.h"
#include "LocalNodeRequestTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace test {


	PeerLocalNodeTestContext::PeerLocalNodeTestContext(
			NodeFlag nodeFlag,
			const consumer<config::BlockchainConfiguration&>& configTransform)
			: m_context(
					nodeFlag | NodeFlag::With_Partner,
					{ CreateLocalPartnerNode() },
					[configTransform](auto& config) {
						configTransform(config);
					},
					"",
				  "../seed/mijin-test-basic-extended")
	{}

	local::LocalNode& PeerLocalNodeTestContext::localNode() const {
		return m_context.localNode();
	}

	std::string PeerLocalNodeTestContext::dataDirectory() const {
		return m_context.dataDirectory();
	}

	std::shared_ptr<config::BlockchainConfigurationHolder> PeerLocalNodeTestContext::configHolder() {
		return m_context.configHolder();
	}
	PeerLocalNodeStats PeerLocalNodeTestContext::stats() const {
		return m_context.stats();
	}

	Height PeerLocalNodeTestContext::height() const {
		ExternalSourceConnection connection;
		return GetLocalNodeHeightViaApi(connection);
	}

	std::string PeerLocalNodeTestContext::resourcesDirectory() const {
		return m_context.resourcesDirectory();
	}

	Height PeerLocalNodeTestContext::loadSavedStateChainHeight() const {
		return m_context.loadSavedStateChainHeight();
	}

	void PeerLocalNodeTestContext::waitForHeight(Height height) const {
		ExternalSourceConnection connection;
		WaitForLocalNodeHeight(connection, height);
	}

	config::BlockchainConfiguration PeerLocalNodeTestContext::prepareFreshDataDirectory(const std::string& directory, const std::string& seedDir) const {
		return m_context.prepareFreshDataDirectory(directory, seedDir);
	}

	void PeerLocalNodeTestContext::assertSingleReaderConnection() const {
		AssertSingleReaderConnection(stats());
	}

	void PeerLocalNodeTestContext::AssertSingleReaderConnection(const PeerLocalNodeStats& stats) {
		// Assert: the external reader connection is still active
		EXPECT_EQ(1u, stats.NumActiveReaders);
		EXPECT_EQ(1u, stats.NumActiveWriters);
		EXPECT_EQ(0u, stats.NumActiveBroadcastWriters);
	}
}}
