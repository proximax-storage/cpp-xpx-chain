/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/PluginConfiguration.h"
#include "catapult/utils/BlockSpan.h"
#include <unordered_set>

namespace catapult { namespace utils { class ConfigurationBag; } }

namespace catapult { namespace config {

	/// Metadata plugin configuration settings.
	struct MetadataConfiguration : public model::PluginConfiguration {
	public:
		/// Maximum fields in metadata.
		uint8_t MaxFields;

		/// Maximum size of key in field.
		uint8_t MaxFieldKeySize;

		/// Maximum size of value in field.
		uint16_t MaxFieldValueSize;

	private:
		MetadataConfiguration() = default;

	public:
		/// Creates an uninitialized metadata configuration.
		static MetadataConfiguration Uninitialized();

		/// Loads a metadata configuration from \a bag.
		static MetadataConfiguration LoadFromBag(const utils::ConfigurationBag& bag);
	};
}}
