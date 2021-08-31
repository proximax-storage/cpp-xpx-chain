/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#include "catapult/crypto/PrivateKey.h"
#include "tests/test/nodeps/Equality.h"

namespace catapult { namespace crypto {

#define TEST_CLASS PrivateKeyTests

	namespace {
		constexpr Key Zero_Key = Key();

		template<typename T>
		constexpr bool IsZeroKey(T memory) {
			return std::equal(Zero_Key.cbegin(), Zero_Key.cend(), memory);
		}

		Key GenerateRandomRawKey() {
			return test::GenerateRandomByteArray<Key>();
		}

		template<typename T>
		T CreatePrivateKey(const Key& rawKey) {
			auto i = 0u;
			return T::Generate([&i, &rawKey]() { return rawKey[i++]; });
		}

		template<typename T>
		T CreateRandomPrivateKey() {
			return T::Generate(test::RandomByte);
		}
	}

	template<typename T>
	void CanCreateDefaultInitializedPrivateKeyHelp() {
		// Act:
		T key;

		// Assert:
		EXPECT_EQ(sizeof(T), key.size());
		// nothing else to validate since std::array is default initialized with garbage
	}

	TEST(TEST_CLASS, CanCreateDefaultInitializedPrivateKey) {
		CanCreateDefaultInitializedPrivateKeyHelp<PrivateKey>();
		CanCreateDefaultInitializedPrivateKeyHelp<BLSPrivateKey>();
	}

	template<typename T>
	void CanCreatePrivateKeyWithRawKeyHelp() {
		// Arrange:
		auto rawKey = GenerateRandomRawKey();

		// Act:
		auto key = CreatePrivateKey<T>(rawKey);

		// Assert:
		EXPECT_EQ(Key_Size, key.size());
		EXPECT_TRUE(std::equal(rawKey.begin(), rawKey.end(), key.data(), key.data() + key.size())); // raw data
		EXPECT_TRUE(std::equal(rawKey.begin(), rawKey.end(), key.begin(), key.end())); // const iterator
	}

	TEST(TEST_CLASS, CanCreatePrivateKeyWithRawKey) {
		CanCreatePrivateKeyWithRawKeyHelp<PrivateKey>();
		// For BLS we are calculating private key base on IKM
	}

	template<typename T>
	void DestructorZerosOutBackingMemoryHelp() {
		// Arrange: call placement new and move constructor
		auto keyData = GenerateRandomRawKey();
		uint8_t keyMemory[sizeof(T)];
		auto pKey = new (keyMemory) T;
		*pKey = CreatePrivateKey<T>(keyData);

		// Sanity: the key's backing memory is non zero
		EXPECT_FALSE(IsZeroKey(keyMemory));

		// Act: destroy the key
		pKey->~T();

		// Assert: the key's backing memory is zero
		EXPECT_TRUE(IsZeroKey(keyMemory));
	}

	TEST(TEST_CLASS, DestructorZerosOutBackingMemory) {
		DestructorZerosOutBackingMemoryHelp<PrivateKey>();
		DestructorZerosOutBackingMemoryHelp<BLSPrivateKey>();
	}

	template<typename T>
	void MoveConstructorZerosOutBackingMemoryOfSourceHelp() {
		// Arrange: call placement new and move constructor
		uint8_t keyMemory[sizeof(T)];
		auto pKey = new (keyMemory) T;
		*pKey = CreatePrivateKey<T>(GenerateRandomRawKey());
		uint8_t keyCopy[sizeof(T)];
		std::copy(pKey->begin(), pKey->end(), std::begin(keyCopy));

		// Sanity: the key's backing memory is non zero
		EXPECT_FALSE(IsZeroKey(keyMemory));

		// Act: trigger the move constructor
		auto movedKey = std::move(*pKey);

		// Assert:
		// - the source's backing memory is zero
		EXPECT_TRUE(IsZeroKey(keyMemory));
		// - the destination's backing memory is equal to the moved key
		EXPECT_TRUE(std::equal(std::begin(keyCopy), std::end(keyCopy), movedKey.data(), movedKey.data() + movedKey.size()));
	}

	TEST(TEST_CLASS, MoveConstructorZerosOutBackingMemoryOfSource) {
		MoveConstructorZerosOutBackingMemoryOfSourceHelp<PrivateKey>();
		MoveConstructorZerosOutBackingMemoryOfSourceHelp<BLSPrivateKey>();
	}

	template<typename T>
	void MoveAssignmentZerosOutBackingMemoryOfSourceHelp() {
		// Arrange: call placement new and move constructor
		uint8_t keyMemory[sizeof(T)];
		auto pKey = new (keyMemory) T;
		*pKey = CreatePrivateKey<T>(GenerateRandomRawKey());
		uint8_t keyCopy[sizeof(T)];
		std::copy(pKey->begin(), pKey->end(), std::begin(keyCopy));

		// Sanity: the key's backing memory is non zero
		EXPECT_FALSE(IsZeroKey(keyMemory));

		// Act: trigger the move assignment operator
		T movedKey;
		const auto& assignResult = (movedKey = std::move(*pKey));

		// Assert:
		// - the source's backing memory is zero
		EXPECT_TRUE(IsZeroKey(keyMemory));
		// - the destination's backing memory is equal to the moved key
		EXPECT_TRUE(std::equal(std::begin(keyCopy), std::end(keyCopy), movedKey.data(), movedKey.data() + movedKey.size()));
		// - the assignment operator returned the correct reference
		EXPECT_EQ(&movedKey, &assignResult);
	}

	TEST(TEST_CLASS, MoveAssignmentZerosOutBackingMemoryOfSource) {
		MoveAssignmentZerosOutBackingMemoryOfSourceHelp<PrivateKey>();
		MoveAssignmentZerosOutBackingMemoryOfSourceHelp<BLSPrivateKey>();
	}

	namespace {
		template<typename T>
		void AssertCannotCreatePrivateKeyFromStringWithSize(size_t size, char keyFirstChar) {
			// Arrange:
			auto rawKeyString = test::GenerateRandomHexString(size);
			rawKeyString[0] = keyFirstChar;

			// Act + Assert: key creation should fail but string should not be cleared
			EXPECT_THROW(T::FromString(rawKeyString), catapult_invalid_argument) << "string size: " << size;
			EXPECT_EQ(keyFirstChar, rawKeyString[0]);
		}

		template<typename T>
		void AssertCannotCreatePrivateKeyFromStringWithInvalidCharacter(char invalidChar) {
			// Arrange:
			auto rawKeyString = test::GenerateRandomHexString(Key_Size * 2);
			rawKeyString[Key_Size] = invalidChar;

			// Act + Assert: key creation should fail but string should not be cleared
			EXPECT_THROW(T::FromString(rawKeyString), catapult_invalid_argument) << "invalid char: " << invalidChar;
			EXPECT_EQ(invalidChar, rawKeyString[Key_Size]);
		}

		template<typename T>
		void AssertCannotCreatePrivateKeyFromSecureStringWithSize(size_t size) {
			// Arrange:
			std::string zeroString(size, '\0');
			auto rawKeyString = test::GenerateRandomHexString(size);

			// Act + Assert: key creation should fail but string should still be cleared
			EXPECT_THROW(
					T::FromStringSecure(&rawKeyString[0], rawKeyString.size()),
					catapult_invalid_argument) << "string size: " << size;
			EXPECT_EQ(zeroString, rawKeyString);
		}

		template<typename T>
		void AssertCannotCreatePrivateKeyFromSecureStringWithInvalidCharacter(char invalidChar) {
			// Arrange:
			std::string zeroString(Key_Size * 2, '\0');
			auto rawKeyString = test::GenerateRandomHexString(Key_Size * 2);
			rawKeyString[Key_Size] = invalidChar;

			// Act + Assert: key creation should fail but string should still be cleared
			EXPECT_THROW(
					T::FromStringSecure(&rawKeyString[0], rawKeyString.size()),
					catapult_invalid_argument) << "invalid char: " << invalidChar;
			EXPECT_EQ(zeroString, rawKeyString);
		}
	}

	TEST(TEST_CLASS, CannotCreatePrivateKeyFromIncorrectlySizedString) {
		// Assert:
		AssertCannotCreatePrivateKeyFromStringWithSize<PrivateKey>(0, '\0');
		AssertCannotCreatePrivateKeyFromStringWithSize<PrivateKey>(50, 'c');
		AssertCannotCreatePrivateKeyFromStringWithSize<PrivateKey>(120, 'd');
		AssertCannotCreatePrivateKeyFromStringWithSize<BLSPrivateKey>(0, '\0');
		AssertCannotCreatePrivateKeyFromStringWithSize<BLSPrivateKey>(50, 'c');
		AssertCannotCreatePrivateKeyFromStringWithSize<BLSPrivateKey>(120, 'd');
	}

	TEST(TEST_CLASS, CannotCreatePrivateKeyFromIncorrectlySizedSecureString) {
		// Assert:
		AssertCannotCreatePrivateKeyFromSecureStringWithSize<PrivateKey>(0);
		AssertCannotCreatePrivateKeyFromSecureStringWithSize<PrivateKey>(50);
		AssertCannotCreatePrivateKeyFromSecureStringWithSize<PrivateKey>(120);
		AssertCannotCreatePrivateKeyFromSecureStringWithSize<BLSPrivateKey>(0);
		AssertCannotCreatePrivateKeyFromSecureStringWithSize<BLSPrivateKey>(50);
		AssertCannotCreatePrivateKeyFromSecureStringWithSize<BLSPrivateKey>(120);
	}

	TEST(TEST_CLASS, CannotCreatePrivateKeyFromStringWithInvalidCharacter) {
		// Assert:
		AssertCannotCreatePrivateKeyFromStringWithInvalidCharacter<PrivateKey>('g');
		AssertCannotCreatePrivateKeyFromStringWithInvalidCharacter<PrivateKey>('-');
		AssertCannotCreatePrivateKeyFromStringWithInvalidCharacter<BLSPrivateKey>('g');
		AssertCannotCreatePrivateKeyFromStringWithInvalidCharacter<BLSPrivateKey>('-');
	}

	TEST(TEST_CLASS, CannotCreatePrivateKeyFromSecureStringWithInvalidCharacter) {
		// Assert:
		AssertCannotCreatePrivateKeyFromSecureStringWithInvalidCharacter<PrivateKey>('g');
		AssertCannotCreatePrivateKeyFromSecureStringWithInvalidCharacter<PrivateKey>('-');
		AssertCannotCreatePrivateKeyFromSecureStringWithInvalidCharacter<BLSPrivateKey>('g');
		AssertCannotCreatePrivateKeyFromSecureStringWithInvalidCharacter<BLSPrivateKey>('-');
	}

	TEST(TEST_CLASS, CanCreatePrivateKeyFromString) {
		// Arrange:
		auto rawKeyString = std::string("3485d98efd7eb07adafcfd1a157d89de2796a95e780813c0258af3f5f84ed8cb");

		// Act:
		auto key = PrivateKey::FromString(rawKeyString);

		// Assert:
		Key rawKey{ {
			0x34, 0x85, 0xD9, 0x8E, 0xFD, 0x7E, 0xB0, 0x7A, 0xDA, 0xFC, 0xFD, 0x1A, 0x15, 0x7D, 0x89, 0xDE,
			0x27, 0x96, 0xA9, 0x5E, 0x78, 0x08, 0x13, 0xC0, 0x25, 0x8A, 0xF3, 0xF5, 0xF8, 0x4E, 0xD8, 0xCB
		} };
		EXPECT_TRUE(std::equal(rawKey.cbegin(), rawKey.cend(), key.data(), key.data() + key.size()));

		// - note that the factory function should not have zeroed out the input string
		EXPECT_EQ('3', rawKeyString[0]);
	}

	TEST(TEST_CLASS, CanCreatePrivateKeyFromStringSecure) {
		// Arrange:
		std::string zeroString(Key_Size * 2, '\0');
		auto rawKeyString = std::string("3485d98efd7eb07adafcfd1a157d89de2796a95e780813c0258af3f5f84ed8cb");

		// Act:
		auto key = PrivateKey::FromStringSecure(&rawKeyString[0], rawKeyString.size());

		// Assert:
		Key rawKey{ {
			0x34, 0x85, 0xD9, 0x8E, 0xFD, 0x7E, 0xB0, 0x7A, 0xDA, 0xFC, 0xFD, 0x1A, 0x15, 0x7D, 0x89, 0xDE,
			0x27, 0x96, 0xA9, 0x5E, 0x78, 0x08, 0x13, 0xC0, 0x25, 0x8A, 0xF3, 0xF5, 0xF8, 0x4E, 0xD8, 0xCB
		} };
		EXPECT_TRUE(std::equal(rawKey.cbegin(), rawKey.cend(), key.data(), key.data() + key.size()));

		// - note that the factory function should have zeroed out the input string
		EXPECT_EQ(zeroString, rawKeyString);
	}

	TEST(TEST_CLASS, CanCreatePrivateKeyFromGenerator) {
		// Act:
		auto seed = 0;
		auto key = PrivateKey::Generate([&seed]() {
			auto val = static_cast<uint8_t>(seed / 8 + seed % 8);
			++seed;
			return val;
		});

		// Assert:
		Key rawKey{ {
			0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
			0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A
		} };
		EXPECT_TRUE(std::equal(rawKey.cbegin(), rawKey.cend(), key.data(), key.data() + key.size()));
	}

	namespace {
		const char* Default_Key = "default";

		std::unordered_map<std::string, PrivateKey> GenerateEqualityInstanceMap() {
			auto rawKey = GenerateRandomRawKey();
			std::unordered_map<std::string, PrivateKey> map;
			map.emplace(Default_Key, CreatePrivateKey<PrivateKey>(rawKey));
			map.emplace("same-raw", CreatePrivateKey<PrivateKey>(rawKey));
			map.emplace("diff-raw", CreateRandomPrivateKey<PrivateKey>());
			return map;
		}

		std::unordered_set<std::string> GetEqualTags() {
			return { Default_Key, "same-raw" };
		}
	}

	TEST(TEST_CLASS, OperatorEqualReturnsTrueForEqualObjects) {
		// Assert:
		test::AssertOperatorEqualReturnsTrueForEqualObjects(Default_Key, GenerateEqualityInstanceMap(), GetEqualTags());
	}

	TEST(TEST_CLASS, OperatorNotEqualReturnsTrueForUnequalObjects) {
		// Assert:
		test::AssertOperatorNotEqualReturnsTrueForUnequalObjects(Default_Key, GenerateEqualityInstanceMap(), GetEqualTags());
	}
}}
