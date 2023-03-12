/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

extern "C" {
#include <external/ref10/ge.h>
}

#include "Scalar.h"

namespace catapult::crypto {

	class CurvePoint {
	private:
		ge_p3 m_ge_p3;

	private:
		static CurvePoint ConstructBasePoint();

	public:
		CurvePoint();

		static CurvePoint BasePoint();

		CurvePoint operator+(const CurvePoint& a) const;

		CurvePoint operator-(const CurvePoint& a) const;

		CurvePoint operator-() const;

		CurvePoint operator*(const Scalar& a) const;

		friend CurvePoint operator*(const Scalar& a, const CurvePoint& b);

		CurvePoint& operator+=(const CurvePoint& a);

		CurvePoint& operator-=(const CurvePoint& a);

		CurvePoint& operator*=(const Scalar& a);

		bool operator==(const CurvePoint& a) const;

		bool operator!=(const CurvePoint& a) const;

		std::array<uint8_t, 32> toBytes() const;

		void fromBytes(const std::array<uint8_t, 32>& buffer);

		template<class Archive>
		void save(Archive& archive) const {
			archive(toBytes());
		}

		template<class Archive>
		void load(Archive& archive) {
			std::array<uint8_t, 32> buffer {};
			archive(buffer);
			fromBytes(buffer);
		}
	};

}