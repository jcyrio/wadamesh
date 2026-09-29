// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

// Bounded, volatile cache of private messages already delivered to the app.
// MeshCore can still ACK a retransmission with a different packet nonce/hash.
// The sender's sendMessage() generates a new timestamp for every send, so a
// short same-sender/text window also catches re-created application retries.
class IncomingMessageCache {
public:
  static constexpr size_t CAPACITY = 32;
  static constexpr uint32_t LIFETIME_MS = 10UL * 60UL * 1000UL;
  static constexpr uint32_t RETRY_WINDOW_MS = 8UL * 1000UL;

  bool seenOrRemember(const uint8_t sender[32], uint32_t timestamp,
                      const uint8_t text_digest[8], size_t text_len,
                      uint32_t now_ms) {
    if (!sender || !text_digest) return false;
    for (Entry& entry : entries_) {
      if (!entry.used) continue;
      const uint32_t elapsed = uint32_t(now_ms - entry.seen_ms);
      if (entry.text_len == text_len &&
          memcmp(entry.sender, sender, sizeof(entry.sender)) == 0 &&
          memcmp(entry.text_digest, text_digest, sizeof(entry.text_digest)) == 0 &&
          ((entry.timestamp == timestamp && elapsed < LIFETIME_MS) ||
           elapsed < RETRY_WINDOW_MS)) {
        // Keep the short retry window open for a train of re-created sends.
        // Preserve the original timestamp for exact matches after the train.
        if (elapsed < RETRY_WINDOW_MS) entry.seen_ms = now_ms;
        return true;
      }
    }

    Entry& entry = entries_[next_];
    memcpy(entry.sender, sender, sizeof(entry.sender));
    memcpy(entry.text_digest, text_digest, sizeof(entry.text_digest));
    entry.timestamp = timestamp;
    entry.text_len = text_len;
    entry.seen_ms = now_ms;
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
    uint32_t seen_ms = 0;
    bool used = false;
  };

  Entry entries_[CAPACITY] = {};
  size_t next_ = 0;
};
