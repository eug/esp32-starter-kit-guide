#pragma once

namespace mqttlink {
void begin();
void loop();  // connect, HA discovery, periodic state publish (no-op if unconfigured)
}  // namespace mqttlink
