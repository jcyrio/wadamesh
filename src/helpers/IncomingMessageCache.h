// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

// Bounded, volatile cache of private messages already delivered to the app.
// Retries can change their attempt bits (and thus their packet hash) while
// retaining the sender, send-time and plaintext. Suppress only that logical
// message. Identical words with a new timestamp may be an intentional new send.
class IncomingMessageCache {
public:
  static constexpr size_t CAPACITY = 32;
  static constexpr uint32_t LIFETIME_MS = 10UL * 60UL * 1000UL;

  bool seenOrRemember(const uint8_t sender[32], uint32_t timestamp,
                      const uint8_t text_digest[8], size_t text_len,
                      uint32_t now_ms) {
    if (!sender || !text_digest) return false;
    for (Entry& entry : entries_) {
      if (!entry.used || uint32_t(now_ms - entry.first_seen_ms) >= LIFETIME_MS) continue;
      if (entry.timestamp == timestamp && entry.text_len == text_len &&
          memcmp(entry.sender, sender, sizeof(entry.sender)) == 0 &&
          memcmp(entry.text_digest, text_digest, sizeof(entry.text_digest)) == 0) {
        return true;
      }
    }

    Entry& entry = entries_[next_];
    memcpy(entry.sender, sender, sizeof(entry.sender));
    memcpy(entry.text_digest, text_digest, sizeof(entry.text_digest));
    entry.timestamp = timestamp;
    entry.text_len = text_len;
    entry.first_seen_ms = now_ms;
    entry.used = true;
    next_ = (next_ + 1) % CAPACITY;
    return false;
  }

private:
  struct Entry {
    uint8_t sender[32] = {};
    uint8_t text_digest[8] = {};
    uint32_t timestamp = 0;
    size_t text_len = 0;
    uint32_t first_seen_ms = 0;
    bool used = false;
  };

  Entry entries_[CAPACITY] = {};
  size_t next_ = 0;
};
