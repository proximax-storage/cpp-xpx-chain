/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/types.h"
#include <vector>

namespace catapult { namespace state {

	// Catapult config entry.
	class CatapultConfigEntry {
	public:
		// Creates a catapult config entry around \a height and \a blockChainConfig.
		CatapultConfigEntry(const Height& height = Height{0}, const std::string& blockChainConfig = std::string{})
			: m_height(height)
			, m_blockChainConfig(blockChainConfig)
		{}

	public:
		/// Gets the height at which the config should be forced.
		const Height& height() const {
			return m_height;
		}

		/// Sets the \a height to force config at.
		void setHeight(const Height& height) {
			m_height = height;
		}

		/// Gets the catapult version.
		const std::string& blockChainConfig() const {
			return m_blockChainConfig;
		}

		/// Sets the \a blockChainConfig.
		void setBlockChainConfig(const std::string& blockChainConfig) {
			m_blockChainConfig = blockChainConfig;
		}

	private:
		Height m_height;
		std::string m_blockChainConfig;
	};
}}
