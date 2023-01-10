/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

extern "C" {
#include <ref10/ge.h>
}

#include "Scalar.h"
#include "KeyPair.h"

namespace catapult::crypto {

class CurvePoint {

    ge_p3 m_ge_p3;

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
};

}