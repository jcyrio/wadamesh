// SPDX-License-Identifier: GPL-3.0-or-later
// Run with g++ -std=c++11 -Wall -Wextra -Werror -Isrc and execute the binary.
#include <assert.h>
#include <stdint.h>

#include "helpers/IncomingMessageCache.h"

static const uint8_t ALICE[32] = {1};
static const uint8_t HELLO[8] = {3};

static void test_distinct_messages() {
  IncomingMessageCache cache;
  assert(!cache.seenOrRemember(ALICE, 100, HELLO, 5, 1000));

  // A sender can intentionally say "hello" again immediately. A different
  // send-time is a distinct message, however little receive time has elapsed.
  assert(!cache.seenOrRemember(ALICE, 101, HELLO, 5, 1001));
  assert(cache.seenOrRemember(ALICE, 100, HELLO, 5, 1002));
  assert(cache.seenOrRemember(ALICE, 101, HELLO, 5, 1003));

  // Use the full sender key, not a name or short public-key prefix.
  uint8_t same_prefix[32] = {1};
  same_prefix[31] = 2;
  assert(!cache.seenOrRemember(same_prefix, 100, HELLO, 5, 1004));
  assert(cache.seenOrRemember(same_prefix, 100, HELLO, 5, 1005));

  uint8_t different_text[8] = {3};
  different_text[7] = 4;
  assert(!cache.seenOrRemember(ALICE, 100, different_text, 5, 1006));
  assert(!cache.seenOrRemember(ALICE, 100, HELLO, 6, 1007));
}

static void test_retry_train_and_expiry() {
  IncomingMessageCache cache;
  const uint32_t start = 1000;
  assert(!cache.seenOrRemember(ALICE, 100, HELLO, 5, start));
  // A delayed retry remains the same message; there is no short text-only gate.
  assert(cache.seenOrRemember(ALICE, 100, HELLO, 5, start + 30000));
  assert(cache.seenOrRemember(ALICE, 100, HELLO, 5,
                              start + IncomingMessageCache::LIFETIME_MS - 1));
  // Expiry is measured from first delivery, and retries do not extend it.
  assert(!cache.seenOrRemember(ALICE, 100, HELLO, 5,
                               start + IncomingMessageCache::LIFETIME_MS));
}

static void test_clock_rollover_and_zero() {
  IncomingMessageCache cache;
  const uint32_t start = 0xfffffff0U;
  assert(!cache.seenOrRemember(ALICE, 100, HELLO, 5, start));
  assert(cache.seenOrRemember(ALICE, 100, HELLO, 5, 0x20));
  const uint32_t expiry = start + IncomingMessageCache::LIFETIME_MS;
  assert(cache.seenOrRemember(ALICE, 100, HELLO, 5, expiry - 1));
  assert(!cache.seenOrRemember(ALICE, 100, HELLO, 5, expiry));

  IncomingMessageCache unset_clock;
  assert(!unset_clock.seenOrRemember(ALICE, 0, HELLO, 5, 0));
  assert(unset_clock.seenOrRemember(ALICE, 0, HELLO, 5, 1));
}

static void test_bounded_capacity() {
  IncomingMessageCache cache;
  for (uint32_t ts = 1; ts <= IncomingMessageCache::CAPACITY; ++ts) {
    assert(!cache.seenOrRemember(ALICE, ts, HELLO, 5, ts));
  }
  assert(cache.seenOrRemember(ALICE, 1, HELLO, 5, 100));
  assert(!cache.seenOrRemember(ALICE, 99, HELLO, 5, 101));
  assert(cache.seenOrRemember(ALICE, 2, HELLO, 5, 102));
  assert(!cache.seenOrRemember(ALICE, 1, HELLO, 5, 103));
}

int main() {
  test_distinct_messages();
  test_retry_train_and_expiry();
  test_clock_rollover_and_zero();
  test_bounded_capacity();
}
