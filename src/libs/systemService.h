#ifndef EMULATOR_LIBS_SYSTEMSERVICE_H_
#define EMULATOR_LIBS_SYSTEMSERVICE_H_

#include <cstddef>

namespace Libs::SystemService {

// SCE_SYSTEM_SERVICE_EVENT_ENTITLEMENT_UPDATE
inline constexpr int EventEntitlementUpdate = 0x10000003;

// Queue an event for sceSystemServiceReceiveEvent. The guest-visible
// SystemServiceEvent struct carries 8192 bytes of event-specific data after the
// event type; games dispatch on both. A truncated payload is reported by EXIT.
void PostEvent(int event_type, const void* data, size_t size);

void NotifyEntitlementUpdate();

} // namespace Libs::SystemService

#endif
