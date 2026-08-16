// LinkQualityClient
//
// Prints the reception metadata of every packet the radio hears: signal
// strength, signal-to-noise ratio, and how many hops the packet travelled.
//
// The other examples deal in payloads. This one ignores payloads entirely and
// reports on the radio link itself, which is what you want when measuring
// coverage, comparing antennas, or deciding where a node should go.
//
// Wiring: the radio's serial pins to the pins named below, plus a common
// ground. Set the radio's Serial module to PROTO mode at a matching baud rate.

#include <Meshtastic.h>

#define RX_PIN 16
#define TX_PIN 17
#define BAUD   38400

void on_packet_meta(const mt_packet_meta_t * meta) {
  // hop_start is what the sender launched the packet with; hop_limit is what
  // is left. Equal means nothing relayed it, so this reading describes one
  // radio path rather than the last relay's.
  uint8_t hops = meta->hop_start - meta->hop_limit;

  Serial.print("from 0x");
  Serial.print(meta->from, HEX);
  Serial.print("  rssi ");
  Serial.print(meta->rx_rssi);
  Serial.print(" dBm  snr ");
  Serial.print(meta->rx_snr, 2);
  Serial.print(" dB  hops ");
  Serial.print(hops);

  if (hops == 0) Serial.print(" (direct)");
  if (meta->via_mqtt) Serial.print("  [arrived over MQTT, not the air]");
  if (!meta->is_decoded) Serial.print("  [payload not decryptable]");

  Serial.println();
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) delay(10);

  Serial.println("Listening for packets...");

  set_packet_meta_callback(on_packet_meta);
  mt_serial_init(RX_PIN, TX_PIN, BAUD);
}

void loop() {
  mt_loop(millis());
}
