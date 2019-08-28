/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/types.h"
#include <vector>

namespace catapult { namespace state {

	// Network config entry.
	class NetworkConfigEntry {
	public:
		// Creates a network config entry around \a height, \a networkConfig and \a supportedEntityVersions.
		NetworkConfigEntry(const Height& height = Height{0}, const std::string& networkConfig = std::string{},
				const std::string& supportedEntityVersions = std::string{})
			: m_height(height)
			, m_networkConfig(networkConfig)
			, m_supportedEntityVersions(supportedEntityVersions)
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

		/// Gets the network config.
		const std::string& networkConfig() const {
			return m_networkConfig;
		}

		/// Sets the \a networkConfig.
		void setBlockChainConfig(const std::string& networkConfig) {
			m_networkConfig = networkConfig;
		}

		/// Gets the supported versions.
		const std::string& supportedEntityVersions() const {
			return m_supportedEntityVersions;
		}

		/// Sets the \a supportedEntityVersions.
		void setSupportedEntityVersions(const std::string& supportedEntityVersions) {
			m_supportedEntityVersions = supportedEntityVersions;
		}

	private:
		Height m_height;
		std::string m_networkConfig;
		std::string m_supportedEntityVersions;
	};
}}
