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
	class CatapultUpgradeEntry {
	public:
		// Creates a catapult upgrade entry around \a height and \a catapultVersion.
		CatapultUpgradeEntry(const Height& height = Height{0}, const CatapultVersion catapultVersion = CatapultVersion{0})
			: m_height(height)
			, m_catapultVersion(catapultVersion)
		{}

	public:
		/// Gets the height of upgrade application.
		const Height& height() const {
			return m_height;
		}

		/// Sets the \a height to apply upgrade at.
		void setHeight(const Height& height) {
			m_height = height;
		}

		/// Gets the catapult version.
		const CatapultVersion& catapultVersion() const {
			return m_catapultVersion;
		}

		/// Sets the \a catapultVersion.
		void setCatapultVersion(const CatapultVersion& catapultVersion) {
			m_catapultVersion = catapultVersion;
		}

	private:
		Height m_height;
		CatapultVersion m_catapultVersion;
	};
}}
