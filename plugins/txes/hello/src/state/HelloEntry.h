/**
*** FOR TRAINING PURPOSES ONLY
**/

#pragma once
#include "catapult/types.h"
#include <vector>

namespace catapult { namespace state {

        // Catapult upgrade entry.
        class HelloEntry {
        public:
            // Creates a catapult upgrade entry around \a height and \a blockChainVersion.
            HelloEntry(uint16_t messageCount = 1)
                    : m_messageCount(messageCount)
            {}

            HelloEntry(const Key& key)
                    : m_key(key)
            {}

            HelloEntry(const Key& key, uint16_t messageCount)
                    : m_key(key), m_messageCount(messageCount)
            {}


        public:

            const uint16_t& messageCount() const {
                return m_messageCount;
            }

            /// Sets the \a height to force upgrade at.
            void setMessageCount(const uint16_t& count) {
                m_messageCount = count;
            }

            // Gets the account public key.
            const Key& key() const {
                return m_key;
            }

        private:
            Key         m_key;
            uint16_t    m_messageCount;

            // TODO: what will be our key type?
        };
    }}
