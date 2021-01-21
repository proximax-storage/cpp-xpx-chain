/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/PluginConfiguration.h"
#include "catapult/types.h"
#include <stdint.h>

namespace catapult { namespace utils { class ConfigurationBag; } }

namespace catapult { namespace config {

	/// Committee plugin configuration settings.
	struct CommitteeConfiguration : public model::PluginConfiguration {
	public:
		DEFINE_CONFIG_CONSTANTS(committee)

	public:
		/// Whether the plugin is enabled.
		bool Enabled;

		/// Min greed value when selecting block proposer.
		double MinGreed;

		/// Activity of the new harvesters.
		double InitialActivity;

		/// Activity value subtracted from activity of both committee and non-committee harvesters after each block.
		double ActivityDelta;

		/// Activity value added to activity of committee members that cosigned the block.
		double ActivityCommitteeCosignedDelta;

		/// Activity value subtracted from activity of committee members that didn't cosign the block.
		double ActivityCommitteeNotCosignedDelta;

	private:
		CommitteeConfiguration() = default;

	public:
		/// Creates an uninitialized committee configuration.
		static CommitteeConfiguration Uninitialized();

		/// Loads an committee configuration from \a bag.
		static CommitteeConfiguration LoadFromBag(const utils::ConfigurationBag& bag);
	};
}}
