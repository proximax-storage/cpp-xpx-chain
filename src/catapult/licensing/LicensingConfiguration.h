/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include <boost/filesystem/path.hpp>
#include <string>

namespace catapult { namespace utils { class ConfigurationBag; } }

namespace catapult { namespace licensing {

	/// Licensing configuration settings.
	struct LicensingConfiguration {
	public:
		/// License server address.
		std::string LicenseServerHost;

		/// License server port.
		uint16_t LicenseServerPort;

	private:
		LicensingConfiguration() = default;

	public:
		/// Creates an uninitialized licensing configuration.
		static LicensingConfiguration Uninitialized();

	public:
		/// Loads a licensing configuration from \a bag.
		static LicensingConfiguration LoadFromBag(const utils::ConfigurationBag& bag);

		/// Loads a licensing configuration from \a resourcesPath.
		static LicensingConfiguration LoadFromPath(const boost::filesystem::path& resourcesPath);
	};
}}
