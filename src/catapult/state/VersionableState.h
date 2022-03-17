/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/ImportanceHeight.h"
#include <atomic>

namespace catapult { namespace state {

	/// Version extension for state classes
	struct VersionableState {
	public:
		/// Default version 1.
		VersionableState()
				: Version(1)
		{}

		VersionableState(const uint32_t version)
				: Version(version)
		{}

		VersionableState(const VersionableState& rhs)
				: Version(rhs.Version)
		{}

		VersionableState& operator=(const VersionableState& rhs) {
			Version = rhs.Version;
			return *this;
		}

	protected:
		/// Version of this state.
		uint32_t Version;

	public:
		uint32_t GetVersion() const{ return Version; }
	};
}}
