#pragma once

#include <string>
#include <vector>

#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace gdo_selftest {

/// Hardware self-test for the GDO blaQ, driven from loop() so nothing blocks.
///
/// The Security+ wire is a single shared line, and the blaQ hears its own
/// transmissions, so with a pull-up on the bus (the test harness) every byte
/// sent on TX must come back on RX. That one loopback covers the TX transistor,
/// the RX divider/inverter, protection parts, both GPIOs and the terminal block.
///
/// The harness may be hand-held against the terminals, so the bus tests wait
/// for contact (the line held up steadily), and a failed attempt waits for
/// contact again and retries, up to max_attempts.
///
/// Results are logged as `KTEST <name> <PASS|FAIL|INFO> <detail>` lines,
/// bracketed by `KTEST BEGIN` / `KTEST END <verdict>`, and the whole block is
/// re-printed every report_interval so a host that attaches late still gets it.
/// Progress is logged as `KTEST STATUS <text>` while the test runs.
class GDOSelfTest : public Component {
 public:
  void set_pins(int tx, int rx) {
    this->tx_pin_ = tx;
    this->rx_pin_ = rx;
  }
  void set_min_rssi(int rssi) { this->min_rssi_ = rssi; }
  void set_report_interval(uint32_t ms) { this->report_interval_ = ms; }
  void set_contact_timeout(uint32_t ms) { this->contact_timeout_ = ms; }
  void set_max_attempts(uint8_t attempts) { this->max_attempts_ = attempts; }

  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::LATE; }

  bool is_done() const { return this->state_ == State::DONE; }
  bool passed() const;

  /// Called with the attempt number (1-based) when the unit starts waiting
  /// for the harness, and again every few seconds while it keeps waiting.
  template<typename F> void add_on_waiting_for_contact_callback(F &&callback) {
    this->waiting_callbacks_.add(std::forward<F>(callback));
  }

 protected:
  enum class State : uint8_t { WAIT_START, WAIT_CONTACT, IDLE_CHECK, ECHO_SEND, ECHO_WAIT, WIFI_WAIT, DONE };
  enum class Verdict : uint8_t { PASS, FAIL, INFO };

  struct Result {
    std::string name;
    Verdict verdict;
    std::string detail;
  };

  bool configure_uart_(uint32_t baud, bool even_parity);
  void enter_wait_contact_(uint32_t now);
  void finish_attempt_(uint32_t now, bool timed_out);
  void finish_idle_check_();
  void finish_echo_();
  void finish_wifi_();
  void add_chip_info_();
  void add_(const std::string &name, Verdict verdict, const std::string &detail);
  void status_(const char *fmt, ...);
  void report_();

  int tx_pin_{-1};
  int rx_pin_{-1};
  int min_rssi_{-80};
  uint32_t report_interval_{3000};
  uint32_t contact_timeout_{45000};
  uint8_t max_attempts_{3};

  State state_{State::WAIT_START};
  uint32_t state_started_{0};
  uint32_t test_started_{0};
  uint32_t last_report_{0};
  bool uart_installed_{false};
  bool uart_ok_{true};

  // Harness contact detection
  uint8_t attempt_{0};
  uint32_t contact_since_{0};  // 0 = no contact yet
  uint32_t last_prompt_{0};

  // Idle check sampling
  uint32_t idle_samples_{0};
  uint32_t idle_busy_{0};

  // Which echo configuration is running (index into ECHO_CONFIGS)
  uint8_t echo_index_{0};

  // Bus results of the attempt in progress; only the final attempt's results
  // make it into results_.
  bool in_attempt_{false};
  std::vector<Result> attempt_results_;
  std::vector<Result> results_;

  CallbackManager<void(uint8_t)> waiting_callbacks_;
};

class WaitingForContactTrigger : public Trigger<uint8_t> {
 public:
  explicit WaitingForContactTrigger(GDOSelfTest *parent) {
    parent->add_on_waiting_for_contact_callback([this](uint8_t attempt) { this->trigger(attempt); });
  }
};

}  // namespace gdo_selftest
}  // namespace esphome
