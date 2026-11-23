#include "gdo_selftest.h"

#include <algorithm>
#include <cinttypes>
#include <cstdarg>
#include <cstdio>

#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"

#include "esphome/components/wifi/wifi_component.h"
#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace gdo_selftest {

static const char *const TAG = "ktest";

// Bumped whenever the report format changes, so the host can refuse a mismatch.
static const int REPORT_FORMAT = 2;

static const uart_port_t UART_PORT = UART_NUM_1;

// Let USB enumerate and the host attach before the first (silent) bus test.
static const uint32_t START_DELAY_MS = 1500;
static const uint32_t IDLE_SETTLE_MS = 50;
static const uint32_t IDLE_SAMPLE_MS = 200;
static const uint32_t ECHO_SETTLE_MS = 20;
static const uint32_t WIFI_TIMEOUT_MS = 20000;
// The line must read "pulled up" continuously this long to count as contact,
// so the bus tests don't start while a hand-held harness is still landing.
static const uint32_t CONTACT_STABLE_MS = 300;
// Re-prompt (tone) this often while waiting for the harness.
static const uint32_t PROMPT_REPEAT_MS = 10000;

struct EchoConfig {
  const char *name;
  uint32_t baud;
  bool even_parity;
};

// The two wire formats the blaQ speaks: Security+ 1.0 and Security+ 2.0.
static const EchoConfig ECHO_CONFIGS[] = {
    {"echo_1200_8e1", 1200, true},
    {"echo_9600_8n1", 9600, false},
};
static const size_t ECHO_COUNT = sizeof(ECHO_CONFIGS) / sizeof(ECHO_CONFIGS[0]);

// Solid levels, alternating bits, a walking one and a walking zero: a stuck,
// shorted or slow bit shows up as a mismatch on at least one of these bytes.
static const uint8_t ECHO_PATTERN[] = {
    0x00, 0xFF, 0x55, 0xAA, 0x0F, 0xF0, 0x33, 0xCC,  // levels / alternation
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,  // walking one
    0xFE, 0xFD, 0xFB, 0xF7, 0xEF, 0xDF, 0xBF, 0x7F,  // walking zero
    'K',  'O',  'N',  'N',  'E',  'C',  'T',  'D',
};
static const size_t ECHO_LEN = sizeof(ECHO_PATTERN);

// Time to clock the pattern out at a given baud (11 or 12 bits per byte), with
// generous headroom for the line's RC and driver latency.
static uint32_t echo_timeout_ms(const EchoConfig &cfg) {
  uint32_t bits = ECHO_LEN * (cfg.even_parity ? 12 : 10);
  return (bits * 1000) / cfg.baud * 2 + 200;
}

static const char *verdict_str(bool pass) { return pass ? "PASS" : "FAIL"; }

void GDOSelfTest::setup() {
  this->state_ = State::WAIT_START;
  this->state_started_ = millis();
  this->test_started_ = this->state_started_;
}

void GDOSelfTest::dump_config() {
  ESP_LOGCONFIG(TAG,
                "GDO self-test:\n"
                "  TX pin: GPIO%d\n"
                "  RX pin: GPIO%d\n"
                "  Min RSSI: %d dBm\n"
                "  Contact timeout: %" PRIu32 " ms\n"
                "  Max attempts: %u",
                this->tx_pin_, this->rx_pin_, this->min_rssi_, this->contact_timeout_, this->max_attempts_);
}

bool GDOSelfTest::configure_uart_(uint32_t baud, bool even_parity) {
  uart_config_t cfg = {};
  cfg.baud_rate = (int) baud;
  cfg.data_bits = UART_DATA_8_BITS;
  cfg.parity = even_parity ? UART_PARITY_EVEN : UART_PARITY_DISABLE;
  cfg.stop_bits = UART_STOP_BITS_1;
  cfg.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
  cfg.source_clk = UART_SCLK_DEFAULT;

  if (!this->uart_installed_) {
    esp_err_t err = uart_driver_install(UART_PORT, 256, 0, 0, nullptr, 0);
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "uart_driver_install: %s", esp_err_to_name(err));
      return false;
    }
    this->uart_installed_ = true;
  }

  // Same order as gdolib: inversion must be set before the pins are routed, or
  // TX idles high for a moment and the transistor pulls the bus low.
  if (uart_param_config(UART_PORT, &cfg) != ESP_OK)
    return false;
  if (uart_set_line_inverse(UART_PORT, UART_SIGNAL_RXD_INV | UART_SIGNAL_TXD_INV) != ESP_OK)
    return false;
  if (uart_set_pin(UART_PORT, this->tx_pin_, this->rx_pin_, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE) != ESP_OK)
    return false;
  uart_flush_input(UART_PORT);
  return true;
}

void GDOSelfTest::loop() {
  const uint32_t now = millis();
  const uint32_t in_state = now - this->state_started_;

  switch (this->state_) {
    case State::WAIT_START:
      if (in_state < START_DELAY_MS)
        return;
      ESP_LOGI(TAG, "Starting self-test");
      // Routing the pins with inversion set releases the bus (TX transistor
      // off), so the RX pin reflects whether the harness pull-up is there.
      this->uart_ok_ = this->configure_uart_(ECHO_CONFIGS[0].baud, ECHO_CONFIGS[0].even_parity);
      this->attempt_ = 1;
      this->enter_wait_contact_(now);
      return;

    case State::WAIT_CONTACT: {
      // Contact = the line pulled up (RX pin low through the inverting stage)
      // without a single busy sample for CONTACT_STABLE_MS.
      bool busy = false;
      for (int i = 0; i < 10; i++) {
        if (gpio_get_level((gpio_num_t) this->rx_pin_))
          busy = true;
        delayMicroseconds(100);
      }
      if (busy) {
        this->contact_since_ = 0;
      } else if (this->contact_since_ == 0) {
        this->contact_since_ = now ? now : 1;
      }

      if (this->contact_since_ != 0 && now - this->contact_since_ >= CONTACT_STABLE_MS) {
        this->status_("harness contact: running bus tests (attempt %u/%u)", this->attempt_, this->max_attempts_);
        this->in_attempt_ = true;
        this->attempt_results_.clear();
        this->idle_samples_ = 0;
        this->idle_busy_ = 0;
        this->state_ = State::IDLE_CHECK;
        this->state_started_ = now;
        return;
      }

      if (in_state >= this->contact_timeout_) {
        if (this->attempt_results_.empty()) {
          // Never made contact at all.
          this->in_attempt_ = true;
          char detail[128];
          snprintf(detail, sizeof(detail),
                   "no harness contact within %" PRIu32 "s (harness not applied, bus shorted, or TX stuck on)",
                   this->contact_timeout_ / 1000);
          this->add_("bus_idle", Verdict::FAIL, detail);
        } else {
          // Not re-applied after a failed attempt: report that attempt.
          this->status_("no harness contact for attempt %u/%u", this->attempt_, this->max_attempts_);
          this->attempt_--;
        }
        this->finish_attempt_(now, true);
        return;
      }

      if (now - this->last_prompt_ >= PROMPT_REPEAT_MS) {
        this->last_prompt_ = now;
        this->waiting_callbacks_.call(this->attempt_);
      }
      return;
    }

    case State::IDLE_CHECK:
      // With TX released, the pulled-up line idles high, which the inverting RX
      // stage presents as a low pin. A high pin here means something is holding
      // the bus down: a stuck TX transistor, a short, or a damaged RX stage.
      if (in_state < IDLE_SETTLE_MS)
        return;
      if (in_state < IDLE_SETTLE_MS + IDLE_SAMPLE_MS) {
        // loop() only runs every ~16ms, so take a 1ms burst each pass.
        for (int i = 0; i < 10; i++) {
          this->idle_samples_++;
          if (gpio_get_level((gpio_num_t) this->rx_pin_))
            this->idle_busy_++;
          delayMicroseconds(100);
        }
        return;
      }
      this->finish_idle_check_();
      this->echo_index_ = 0;
      this->state_ = State::ECHO_SEND;
      this->state_started_ = now;
      return;

    case State::ECHO_SEND: {
      const EchoConfig &cfg = ECHO_CONFIGS[this->echo_index_];
      if (!this->uart_ok_ || !this->configure_uart_(cfg.baud, cfg.even_parity)) {
        this->add_(cfg.name, Verdict::FAIL, "uart setup failed");
        this->uart_ok_ = false;
        this->echo_index_++;
        this->state_started_ = now;
        if (this->echo_index_ >= ECHO_COUNT)
          this->finish_attempt_(now, false);
        return;
      }
      delay(ECHO_SETTLE_MS);
      uart_flush_input(UART_PORT);
      uart_write_bytes(UART_PORT, ECHO_PATTERN, ECHO_LEN);
      this->state_ = State::ECHO_WAIT;
      this->state_started_ = now;
      return;
    }

    case State::ECHO_WAIT: {
      const EchoConfig &cfg = ECHO_CONFIGS[this->echo_index_];
      size_t buffered = 0;
      uart_get_buffered_data_len(UART_PORT, &buffered);
      if (buffered < ECHO_LEN && in_state < echo_timeout_ms(cfg))
        return;
      this->finish_echo_();
      this->echo_index_++;
      this->state_started_ = now;
      if (this->echo_index_ < ECHO_COUNT) {
        this->state_ = State::ECHO_SEND;
      } else {
        this->finish_attempt_(now, false);
      }
      return;
    }

    case State::WIFI_WAIT: {
      auto *wifi = wifi::global_wifi_component;
      bool have_results = wifi != nullptr && !wifi->get_scan_result().empty();
      if (!have_results && now - this->test_started_ < WIFI_TIMEOUT_MS)
        return;
      this->finish_wifi_();
      this->add_chip_info_();
      this->state_ = State::DONE;
      this->state_started_ = now;
      this->last_report_ = 0;
      ESP_LOGI(TAG, "Self-test complete: %s", verdict_str(this->passed()));
      return;
    }

    case State::DONE:
      if (this->last_report_ == 0 || now - this->last_report_ >= this->report_interval_) {
        this->last_report_ = now;
        this->report_();
      }
      return;
  }
}

void GDOSelfTest::enter_wait_contact_(uint32_t now) {
  this->state_ = State::WAIT_CONTACT;
  this->state_started_ = now;
  this->contact_since_ = 0;
  this->last_prompt_ = now;
  this->status_("waiting for harness (attempt %u/%u)", this->attempt_, this->max_attempts_);
  this->waiting_callbacks_.call(this->attempt_);
}

// Ends a bus attempt: retries on failure while attempts remain (a slipping
// harness), otherwise commits the attempt's results and moves on.
void GDOSelfTest::finish_attempt_(uint32_t now, bool timed_out) {
  this->in_attempt_ = false;
  const Result *failed = nullptr;
  for (const auto &r : this->attempt_results_) {
    if (r.verdict == Verdict::FAIL) {
      failed = &r;
      break;
    }
  }

  if (failed != nullptr && !timed_out && this->attempt_ < this->max_attempts_) {
    this->status_("attempt %u/%u failed (%s): re-apply harness", this->attempt_, this->max_attempts_,
                  failed->name.c_str());
    this->attempt_++;
    this->enter_wait_contact_(now);
    return;
  }

  this->results_.insert(this->results_.end(), this->attempt_results_.begin(), this->attempt_results_.end());
  char detail[32];
  snprintf(detail, sizeof(detail), "%u/%u", this->attempt_, this->max_attempts_);
  this->add_("bus_attempts", Verdict::INFO, detail);
  this->state_ = State::WIFI_WAIT;
  this->state_started_ = now;
}

void GDOSelfTest::status_(const char *fmt, ...) {
  char buf[128];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  ESP_LOGI(TAG, "KTEST STATUS %s", buf);
}

void GDOSelfTest::finish_idle_check_() {
  char detail[128];
  if (!this->uart_ok_) {
    this->add_("bus_idle", Verdict::FAIL, "uart setup failed");
    return;
  }
  // Without a pull-up the line sits at 0V, which is indistinguishable from a
  // bus held low -- so a solid low says "check the fixture first".
  const char *why = "";
  if (this->idle_busy_ == this->idle_samples_ && this->idle_samples_ > 0) {
    why = " (line at 0V: fixture pull-up missing, bus shorted, or TX stuck on)";
  } else if (this->idle_busy_ > 0) {
    why = " (line flapping: noise, bad contact, or RX fault)";
  }
  snprintf(detail, sizeof(detail), "busy=%" PRIu32 "/%" PRIu32 "%s", this->idle_busy_, this->idle_samples_, why);
  this->add_("bus_idle", this->idle_busy_ == 0 ? Verdict::PASS : Verdict::FAIL, detail);
}

void GDOSelfTest::finish_echo_() {
  const EchoConfig &cfg = ECHO_CONFIGS[this->echo_index_];
  uint8_t rx[ECHO_LEN * 2];
  int got = uart_read_bytes(UART_PORT, rx, sizeof(rx), 0);
  if (got < 0)
    got = 0;

  char detail[96];
  if (got == 0) {
    snprintf(detail, sizeof(detail), "rx=0/%u (no echo: check fixture pull-up, TX or RX path)", (unsigned) ECHO_LEN);
    this->add_(cfg.name, Verdict::FAIL, detail);
    return;
  }

  size_t compare = std::min((size_t) got, ECHO_LEN);
  for (size_t i = 0; i < compare; i++) {
    if (rx[i] != ECHO_PATTERN[i]) {
      snprintf(detail, sizeof(detail), "rx=%d/%u mismatch@%u sent=0x%02X got=0x%02X", got, (unsigned) ECHO_LEN,
               (unsigned) i, ECHO_PATTERN[i], rx[i]);
      this->add_(cfg.name, Verdict::FAIL, detail);
      return;
    }
  }
  if ((size_t) got != ECHO_LEN) {
    snprintf(detail, sizeof(detail), "rx=%d/%u (%s)", got, (unsigned) ECHO_LEN,
             (size_t) got < ECHO_LEN ? "bytes lost" : "extra bytes on bus");
    this->add_(cfg.name, Verdict::FAIL, detail);
    return;
  }
  snprintf(detail, sizeof(detail), "rx=%d/%u", got, (unsigned) ECHO_LEN);
  this->add_(cfg.name, Verdict::PASS, detail);
}

void GDOSelfTest::finish_wifi_() {
  auto *wifi = wifi::global_wifi_component;
  if (wifi == nullptr) {
    this->add_("wifi_scan", Verdict::FAIL, "wifi component missing");
    return;
  }

  struct Ap {
    std::string ssid;
    int rssi;
  };
  std::vector<Ap> aps;
  for (const auto &res : wifi->get_scan_result()) {
    StringRef ref = res.get_ssid();
    std::string ssid(ref.c_str(), ref.size());
    // Keep the report line parseable: no quotes or control characters.
    for (auto &c : ssid) {
      if (c == '"' || c < 0x20 || c > 0x7E)
        c = '?';
    }
    aps.push_back({ssid, res.get_rssi()});
  }

  if (aps.empty()) {
    this->add_("wifi_scan", Verdict::FAIL, "no access points found (antenna / RF fault)");
    return;
  }

  std::sort(aps.begin(), aps.end(), [](const Ap &a, const Ap &b) { return a.rssi > b.rssi; });
  char detail[64];
  snprintf(detail, sizeof(detail), "aps=%u best=%d min=%d", (unsigned) aps.size(), aps[0].rssi, this->min_rssi_);
  this->add_("wifi_scan", aps[0].rssi >= this->min_rssi_ ? Verdict::PASS : Verdict::FAIL, detail);

  // Report the strongest few so the host can apply a site-specific check
  // (e.g. "office SSID at >= -60 dBm") without reflashing.
  size_t shown = std::min(aps.size(), (size_t) 8);
  for (size_t i = 0; i < shown; i++) {
    char line[64];
    snprintf(line, sizeof(line), "%d \"%s\"", aps[i].rssi, aps[i].ssid.c_str());
    this->add_("wifi_ap", Verdict::INFO, line);
  }
}

void GDOSelfTest::add_chip_info_() {
  uint32_t flash_size = 0;
  esp_flash_get_size(nullptr, &flash_size);
  esp_chip_info_t chip;
  esp_chip_info(&chip);
  char detail[96];
  snprintf(detail, sizeof(detail), "rev=%u cores=%u flash=%" PRIu32 "KB free_heap=%" PRIu32, chip.revision,
           chip.cores, flash_size / 1024, esp_get_free_heap_size());
  this->add_("chip", Verdict::INFO, detail);
}

void GDOSelfTest::add_(const std::string &name, Verdict verdict, const std::string &detail) {
  (this->in_attempt_ ? this->attempt_results_ : this->results_).push_back({name, verdict, detail});
  const char *v = verdict == Verdict::PASS ? "PASS" : verdict == Verdict::FAIL ? "FAIL" : "INFO";
  ESP_LOGI(TAG, "%s: %s %s", name.c_str(), v, detail.c_str());
}

bool GDOSelfTest::passed() const {
  if (this->state_ != State::DONE)
    return false;
  for (const auto &r : this->results_) {
    if (r.verdict == Verdict::FAIL)
      return false;
  }
  return true;
}

void GDOSelfTest::report_() {
  // get_mac_address() (std::string) moved to alloc_helpers.h in ESPHome
  // 2026.11; the buffer form is in helpers.h on both sides of that change.
  char mac[MAC_ADDRESS_BUFFER_SIZE];
  get_mac_address_into_buffer(mac);
  ESP_LOGI(TAG, "KTEST BEGIN format=%d fw=%s mac=%s", REPORT_FORMAT, ESPHOME_PROJECT_VERSION, mac);
  for (const auto &r : this->results_) {
    const char *v = r.verdict == Verdict::PASS ? "PASS" : r.verdict == Verdict::FAIL ? "FAIL" : "INFO";
    ESP_LOGI(TAG, "KTEST %s %s %s", r.name.c_str(), v, r.detail.c_str());
  }
  ESP_LOGI(TAG, "KTEST END %s", verdict_str(this->passed()));
}

}  // namespace gdo_selftest
}  // namespace esphome
