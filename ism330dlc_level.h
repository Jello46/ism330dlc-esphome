#pragma once

#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome {
namespace ism330dlc_level {

static const char *const TAG = "ism330dlc_level";

// Registers
static const uint8_t REG_WHO_AM_I   = 0x0F;  // expected 0x6A
static const uint8_t REG_CTRL1_XL   = 0x10;
static const uint8_t REG_CTRL3_C    = 0x12;
static const uint8_t REG_OUTX_L_XL  = 0x28;

static const uint8_t CTRL3_C_BDU_IFINC = 0x44;
static const uint8_t CTRL1_XL_104HZ_2G = 0x40;

class ISM330DLCLevelComponent : public PollingComponent, public i2c::I2CDevice {
 public:
  void set_accel_x(sensor::Sensor *s) { ax_ = s; }
  void set_accel_y(sensor::Sensor *s) { ay_ = s; }
  void set_accel_z(sensor::Sensor *s) { az_ = s; }

  void dump_config() override {
    ESP_LOGCONFIG(TAG, "ISM330DLC Level (external component)");
    ESP_LOGCONFIG(TAG, "  Address: 0x%02X", this->address_);
    ESP_LOGCONFIG(TAG, "  Initialized: %s", inited_ ? "YES" : "NO");
    LOG_I2C_DEVICE(this);
    LOG_SENSOR("  ", "Accel X", this->ax_);
    LOG_SENSOR("  ", "Accel Y", this->ay_);
    LOG_SENSOR("  ", "Accel Z", this->az_);
  }

  void setup() override {
    ESP_LOGI(TAG, "setup() (addr 0x%02X)", this->address_);
    (void) this->try_init_();  // do not hard-fail; update() will retry
  }

  void update() override {
    // Keep attempting init until it works
    if (!inited_) {
      if (!this->try_init_()) return;
    }

    uint8_t data[6];
    auto err = this->read_register(REG_OUTX_L_XL, data, 6);

    // One quick retry on transient NACK
    if (err != i2c::ERROR_OK) {
      delay(2);
      err = this->read_register(REG_OUTX_L_XL, data, 6);
    }
    if (err != i2c::ERROR_OK) {
      ESP_LOGW(TAG, "Accel read failed (addr 0x%02X)", this->address_);
      return;
    }

    int16_t x = (int16_t)((data[1] << 8) | data[0]);
    int16_t y = (int16_t)((data[3] << 8) | data[2]);
    int16_t z = (int16_t)((data[5] << 8) | data[4]);

    constexpr float G_PER_LSB = 0.000061f;

    if (ax_) ax_->publish_state(x * G_PER_LSB);
    if (ay_) ay_->publish_state(y * G_PER_LSB);
    if (az_) az_->publish_state(z * G_PER_LSB);
  }

 protected:
  sensor::Sensor *ax_{nullptr};
  sensor::Sensor *ay_{nullptr};
  sensor::Sensor *az_{nullptr};

  bool inited_{false};

  bool try_init_() {
    // Read WHO_AM_I using read_register (most compatible)
    uint8_t who = 0;
    auto err = this->read_register(REG_WHO_AM_I, &who, 1);
    if (err != i2c::ERROR_OK) {
      // Log at most every 5s to avoid spam
      static uint32_t last_log = 0;
      uint32_t now = millis();
      if (now - last_log > 5000) {
        ESP_LOGW(TAG, "WHO_AM_I read failed (addr 0x%02X)", this->address_);
        last_log = now;
      }
      return false;
    }

    // WHO_AM_I for ISM330DLC is typically 0x6A; log what we see
    ESP_LOGI(TAG, "WHO_AM_I = 0x%02X", who);

    // write_byte returns bool in ESPHome 2025.12.x
    if (!this->write_byte(REG_CTRL3_C, CTRL3_C_BDU_IFINC)) {
      ESP_LOGW(TAG, "Write CTRL3_C failed");
      return false;
    }

    if (!this->write_byte(REG_CTRL1_XL, CTRL1_XL_104HZ_2G)) {
      ESP_LOGW(TAG, "Write CTRL1_XL failed");
      return false;
    }

    inited_ = true;
    ESP_LOGI(TAG, "Init OK (±2g, 104Hz)");
    return true;
  }
};

}  // namespace ism330dlc_level
}  // namespace esphome
