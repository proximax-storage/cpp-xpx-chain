/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/types.h"
#include <vector>

namespace catapult { namespace state {

	// Catapult upgrade entry.
	class BlockchainUpgradeEntry {
	public:
		// Creates a catapult upgrade entry around \a height and \a blockChainVersion.
		BlockchainUpgradeEntry(const Height& height = Height{0}, const BlockchainVersion& blockChainVersion = BlockchainVersion{0})
			: m_height(height)
			, m_blockChainVersion(blockChainVersion)
		{}

	public:
		/// Gets the height at which the upgrade should be forced.
		const Height& height() const {
			return m_height;
		}

		/// Sets the \a height to force upgrade at.
		void setHeight(const Height& height) {
			m_height = height;
		}

		/// Gets the catapult version.
		const BlockchainVersion& blockChainVersion() const {
			return m_blockChainVersion;
		}

		/// Sets the \a blockChainVersion.
		void setBlockchainVersion(const BlockchainVersion& blockChainVersion) {
			m_blockChainVersion = blockChainVersion;
		}

	private:
		Height m_height;
		BlockchainVersion m_blockChainVersion;
	};
}}
