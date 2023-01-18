/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "CurvePoint.h"

namespace catapult::crypto {

	CurvePoint::CurvePoint() {
		ge_p3 a;
		ge_p3_0(&a);
		m_ge_p3 = a;
	}

	CurvePoint CurvePoint::BasePoint() {
		CurvePoint temp;
		ge_p3 a;
		Scalar one;
		one[0] = 1;
		ge_scalarmult_base(&a, one.data());
		temp.m_ge_p3 = a;
		return temp;
	}

	CurvePoint CurvePoint::operator+(const CurvePoint& a) const {
		CurvePoint temp;
		ge_cached cache;
		ge_p3_to_cached(&cache, &a.m_ge_p3);

		ge_p1p1 ans;
		ge_add(&ans, &m_ge_p3, &cache);
		ge_p1p1_to_p3(&temp.m_ge_p3, &ans);
		return temp;
	}

	CurvePoint CurvePoint::operator-(const CurvePoint& a) const {
		CurvePoint temp;
		ge_cached cache;
		ge_p3_to_cached(&cache, &a.m_ge_p3);

		ge_p1p1 ans;
		ge_sub(&ans, &m_ge_p3, &cache);
		ge_p1p1_to_p3(&temp.m_ge_p3, &ans);
		return temp;
	}

	CurvePoint CurvePoint::operator-() const {
		auto temp = *this;
		auto offset = *this;
		temp -= offset;
		temp -= offset;
		return temp;
	}

	CurvePoint CurvePoint::operator*(const Scalar& a) const {
		CurvePoint ret;
		Scalar zero;
		ge_p2 ans;
		ge_double_scalarmult_vartime(&ans, a.data(), &m_ge_p3, zero.data());
		Scalar temp;
		ge_tobytes(temp.data(), &ans);

		ge_frombytes_negate_vartime(&ret.m_ge_p3, temp.data());
		return -ret;
	}

	CurvePoint operator*(const Scalar& a, const CurvePoint& b) {
		CurvePoint ret;
		Scalar zero;
		ge_p2 ans;
		ge_double_scalarmult_vartime(&ans, a.data(), &b.m_ge_p3, zero.data());
		Scalar temp;
		ge_tobytes(temp.data(), &ans);

		ge_frombytes_negate_vartime(&ret.m_ge_p3, temp.data());
		return -ret;
	}

	CurvePoint& CurvePoint::operator+=(const CurvePoint& a) {
		*this = *this + a;
		return *this;
	}

	CurvePoint& CurvePoint::operator-=(const CurvePoint& a) {
		*this = *this - a;
		return *this;
	}

	CurvePoint& CurvePoint::operator*=(const Scalar& a) {
		*this = *this * a;
		return *this;
	}

	bool CurvePoint::operator==(const CurvePoint& a) const {
		Scalar this_bytes;
		ge_p3_tobytes(this_bytes.data(), &m_ge_p3);
		Scalar a_bytes;
		ge_p3_tobytes(a_bytes.data(), &a.m_ge_p3);

		return this_bytes == a_bytes;
	}

	bool CurvePoint::operator!=(const CurvePoint& a) const {
		Scalar this_bytes;
		ge_p3_tobytes(this_bytes.data(), &m_ge_p3);
		Scalar a_bytes;
		ge_p3_tobytes(a_bytes.data(), &a.m_ge_p3);

		return this_bytes != a_bytes;
	}

	std::array<uint8_t, 32> CurvePoint::toBytes() const {
		std::array<uint8_t, 32> buffer {};
		ge_p3_tobytes(buffer.data(), &m_ge_p3);
		return buffer;
	}

	void CurvePoint::fromBytes(const std::array<uint8_t, 32>& buffer) {
		ge_p3 tmp;
		ge_frombytes_negate_vartime(&tmp, buffer.data());
		CurvePoint tmpPoint;
		tmpPoint.m_ge_p3 = tmp;
		*this = -tmpPoint;
	}

} // namespace catapult::crypto