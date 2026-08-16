// Host-side check of the packet-metadata callback.
//
// This runs on a development machine with a stub Arduino layer. It cannot
// prove anything about a real radio, but it does prove the callback fires
// when it should, carries the right values, and does not fire when unset —
// none of which should wait for hardware to find out.

#include <cassert>
#include <cstdio>
#include <cstring>

#include "Meshtastic.h"

SerialStub Serial;

// Defined in mt_protocol.cpp; not exposed in a header.
bool handle_mesh_packet(meshtastic_MeshPacket * meshPacket);

// --- link stubs for the transports we are not exercising -------------------
bool mt_wifi_loop(uint32_t) { return false; }
bool mt_serial_loop() { return false; }
size_t mt_wifi_check_radio(char *, size_t) { return 0; }
size_t mt_serial_check_radio(char *, size_t) { return 0; }
bool mt_wifi_send_radio(const char *, size_t) { return false; }
bool mt_serial_send_radio(const char *, size_t) { return false; }
void mt_wifi_reset_idle_timeout(uint32_t) {}

// --- capture ---------------------------------------------------------------
static mt_packet_meta_t seen;
static int seen_count = 0;

static void capture(const mt_packet_meta_t * meta) {
  seen = *meta;
  seen_count++;
}

static meshtastic_MeshPacket base_packet() {
  meshtastic_MeshPacket p = meshtastic_MeshPacket_init_default;
  p.from       = 0x11223344;
  p.to         = 0xFFFFFFFF;
  p.id         = 987654321;
  p.rx_time    = 1786000000;
  p.channel    = 0;
  p.rx_snr     = 6.25f;
  p.rx_rssi    = -84;
  p.hop_start  = 3;
  p.hop_limit  = 1;
  p.next_hop   = 0x44;
  p.relay_node = 0x22;
  p.via_mqtt   = false;
  return p;
}

static int failures = 0;

static void check(bool ok, const char * what) {
  if (!ok) { std::printf("FAIL  %s\n", what); failures++; }
  else     { std::printf("ok    %s\n", what); }
}

int main() {
  // 1. Nothing registered: must not crash, must not count.
  {
    seen_count = 0;
    meshtastic_MeshPacket p = base_packet();
    p.which_payload_variant = meshtastic_MeshPacket_decoded_tag;
    p.decoded.portnum = meshtastic_PortNum_TEXT_MESSAGE_APP;
    handle_mesh_packet(&p);
    check(seen_count == 0, "no callback registered -> nothing reported, no crash");
  }

  set_packet_meta_callback(capture);

  // 2. A decoded packet reports every field the radio knew.
  {
    seen_count = 0;
    meshtastic_MeshPacket p = base_packet();
    p.which_payload_variant = meshtastic_MeshPacket_decoded_tag;
    p.decoded.portnum = meshtastic_PortNum_TEXT_MESSAGE_APP;
    p.decoded.payload.size = 12;
    handle_mesh_packet(&p);

    check(seen_count == 1,                 "decoded packet reported exactly once");
    check(seen.from == 0x11223344,         "from carried through");
    check(seen.to == 0xFFFFFFFF,           "to carried through");
    check(seen.id == 987654321,            "packet id carried through");
    check(seen.rx_time == 1786000000,      "rx_time carried through");
    check(seen.rx_rssi == -84,             "rssi carried through");
    check(seen.rx_snr > 6.24f && seen.rx_snr < 6.26f, "snr carried through");
    check(seen.hop_start == 3,             "hop_start carried through");
    check(seen.hop_limit == 1,             "hop_limit carried through");
    check(seen.hop_start - seen.hop_limit == 2, "hops travelled is derivable");
    check(seen.next_hop == 0x44,           "next_hop carried through");
    check(seen.relay_node == 0x22,         "relay_node carried through");
    check(seen.via_mqtt == false,          "via_mqtt carried through");
    check(seen.is_decoded == true,         "decoded packet marked decoded");
    check(seen.portnum == (uint32_t)meshtastic_PortNum_TEXT_MESSAGE_APP, "portnum carried through");
    check(seen.payload_size == 12,         "payload size carried through");
  }

  // 3. A direct reception is distinguishable from a relayed one.
  {
    seen_count = 0;
    meshtastic_MeshPacket p = base_packet();
    p.hop_limit = 3;  // equal to hop_start
    p.which_payload_variant = meshtastic_MeshPacket_decoded_tag;
    p.decoded.portnum = meshtastic_PortNum_TEXT_MESSAGE_APP;
    handle_mesh_packet(&p);
    check(seen.hop_start - seen.hop_limit == 0, "direct reception reports zero hops");
  }

  // 4. An encrypted packet still reports, with portnum honestly unknown.
  {
    seen_count = 0;
    meshtastic_MeshPacket p = base_packet();
    p.which_payload_variant = meshtastic_MeshPacket_encrypted_tag;
    p.encrypted.size = 31;
    handle_mesh_packet(&p);

    check(seen_count == 1,          "encrypted packet reported");
    check(seen.is_decoded == false, "encrypted packet marked not decoded");
    check(seen.portnum == 0,        "portnum is zero when it could not be read");
    check(seen.payload_size == 31,  "encrypted payload size still reported");
    check(seen.rx_rssi == -84,      "signal strength survives an unreadable payload");
  }

  // 5. The case upstream drops on the floor: a portnum the switch does not
  //    list. Without this the logger would silently under-count receptions.
  {
    seen_count = 0;
    meshtastic_MeshPacket p = base_packet();
    p.which_payload_variant = meshtastic_MeshPacket_decoded_tag;
    p.decoded.portnum = (meshtastic_PortNum)250;
    p.decoded.payload.size = 7;
    bool handled = handle_mesh_packet(&p);

    check(seen_count == 1,        "unrecognised portnum still reported");
    check(seen.portnum == 250,    "unrecognised portnum value carried through");
    check(handled == false,       "existing return value for unknown portnum unchanged");
  }

  // 6. A packet that arrived over MQTT is flagged, not hidden.
  {
    seen_count = 0;
    meshtastic_MeshPacket p = base_packet();
    p.via_mqtt = true;
    p.which_payload_variant = meshtastic_MeshPacket_decoded_tag;
    p.decoded.portnum = meshtastic_PortNum_TEXT_MESSAGE_APP;
    handle_mesh_packet(&p);
    check(seen.via_mqtt == true, "mqtt-delivered packet is flagged");
  }

  // 7. Unregistering stops the reports.
  {
    set_packet_meta_callback(NULL);
    seen_count = 0;
    meshtastic_MeshPacket p = base_packet();
    p.which_payload_variant = meshtastic_MeshPacket_decoded_tag;
    p.decoded.portnum = meshtastic_PortNum_TEXT_MESSAGE_APP;
    handle_mesh_packet(&p);
    check(seen_count == 0, "callback can be unregistered");
  }

  std::printf("\n%s\n", failures ? "FAILURES PRESENT" : "all checks passed");
  return failures ? 1 : 0;
}
