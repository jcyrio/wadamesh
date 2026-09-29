// SPDX-License-Identifier: GPL-3.0-or-later
#include <assert.h>
#include <stdint.h>

#include "helpers/IncomingMessageCache.h"

int main() {
  IncomingMessageCache cache;
  uint8_t alice[32] = {1};
  uint8_t bob[32] = {2};
  uint8_t hello[8] = {3};
  uint8_t goodbye[8] = {4};

  assert(!cache.seenOrRemember(alice, 100, hello, 5, 1000));
  assert(cache.seenOrRemember(alice, 100, hello, 5, 1001));
  assert(!cache.seenOrRemember(bob, 100, hello, 5, 1002));
  assert(cache.seenOrRemember(alice, 101, hello, 5, 1003));
  assert(!cache.seenOrRemember(alice, 100, goodbye, 5, 1004));
  assert(!cache.seenOrRemember(alice, 100, hello, 6, 1005));
  assert(cache.seenOrRemember(alice, 100, hello, 5, 1006));
  // A new timestamp with the same text is accepted after the short retry gap.
  assert(!cache.seenOrRemember(alice, 102, hello, 5, 9007));
  assert(cache.seenOrRemember(alice, 103, hello, 5, 10000));

  // A stale cache entry does not suppress a later delivery, even over millis wrap.
  IncomingMessageCache rollover;
  assert(!rollover.seenOrRemember(alice, 100, hello, 5, 0xfffffff0UL));
  assert(rollover.seenOrRemember(alice, 100, hello, 5, 0x20));
  assert(!rollover.seenOrRemember(alice, 100, hello, 5,
      uint32_t(0x20UL + IncomingMessageCache::LIFETIME_MS)));

  IncomingMessageCache exact;
  assert(!exact.seenOrRemember(alice, 100, hello, 5, 1000));
  assert(exact.seenOrRemember(alice, 100, hello, 5, 20000));
  assert(!exact.seenOrRemember(alice, 101, hello, 5, 28001));

  // The bounded cache evicts the oldest message after 32 other arrivals.
  IncomingMessageCache crowded;
  assert(!crowded.seenOrRemember(alice, 1, hello, 5, 1));
  for (uint32_t ts = 2; ts <= IncomingMessageCache::CAPACITY + 1; ++ts) {
    uint8_t sender[32] = {0};
    sender[0] = static_cast<uint8_t>(ts);
    assert(!crowded.seenOrRemember(sender, ts, hello, 5, ts));
  }
  assert(!crowded.seenOrRemember(alice, 1, hello, 5, 100));
  return 0;
}
