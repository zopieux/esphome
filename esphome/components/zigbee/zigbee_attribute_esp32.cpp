#include "zigbee_attribute_esp32.h"
#include "esphome/core/log.h"
#include "esphome/core/defines.h"
#ifdef USE_ESP32
#ifdef USE_ZIGBEE

namespace esphome::zigbee {

static const char *const TAG = "zigbee.attribute";

void ZigbeeAttribute::set_attr_() {
  if (!this->zb_->is_connected()) {
    return;
  }
  if (esp_zb_lock_acquire(10 / portTICK_PERIOD_MS)) {
    esp_zb_zcl_status_t state = esp_zb_zcl_set_attribute_val(this->endpoint_id_, this->cluster_id_, this->role_,
                                                             this->attr_id_, this->value_p_, false);
    if (this->force_report_) {
      this->report_(true);
    }
    this->set_attr_requested_ = false;
    // Check for error
    if (state != ESP_ZB_ZCL_STATUS_SUCCESS) {
      ESP_LOGE(TAG, "Setting attribute failed, ZCL status: %u", static_cast<unsigned>(state));
    }
    esp_zb_lock_release();
  }
}

void ZigbeeAttribute::report_(bool has_lock) {
  if (!this->zb_->is_connected()) {
    return;
  }
  if (has_lock or esp_zb_lock_acquire(10 / portTICK_PERIOD_MS)) {
    esp_zb_zcl_report_attr_cmd_t cmd = {};
    cmd.address_mode = ESP_ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
    cmd.direction = ESP_ZB_ZCL_CMD_DIRECTION_TO_CLI;
    cmd.zcl_basic_cmd.dst_addr_u.addr_short = 0x0000;
    cmd.zcl_basic_cmd.dst_endpoint = 1;
    cmd.zcl_basic_cmd.src_endpoint = this->endpoint_id_;
    cmd.clusterID = this->cluster_id_;
    cmd.attributeID = this->attr_id_;

    esp_zb_zcl_report_attr_cmd_req(&cmd);
    if (!has_lock) {
      esp_zb_lock_release();
    }
  }
}

esp_zb_zcl_reporting_info_t ZigbeeAttribute::get_reporting_info() {
  esp_zb_zcl_reporting_info_t reporting_info = {};
  reporting_info.direction = ESP_ZB_ZCL_CMD_DIRECTION_TO_SRV;
  reporting_info.ep = this->endpoint_id_;
  reporting_info.cluster_id = this->cluster_id_;
  reporting_info.cluster_role = this->role_;
  reporting_info.attr_id = this->attr_id_;
  reporting_info.manuf_code = ESP_ZB_ZCL_ATTR_NON_MANUFACTURER_SPECIFIC;
  reporting_info.dst.profile_id = ESP_ZB_AF_HA_PROFILE_ID;
  reporting_info.u.send_info.min_interval = 10;     /*!< Actual minimum reporting interval */
  reporting_info.u.send_info.max_interval = 0;      /*!< Actual maximum reporting interval */
  reporting_info.u.send_info.def_min_interval = 10; /*!< Default minimum reporting interval */
  reporting_info.u.send_info.def_max_interval = 0;  /*!< Default maximum reporting interval */
  reporting_info.u.send_info.delta.s16 = 0;         /*!< Actual reportable change */

  return reporting_info;
}

void ZigbeeAttribute::set_report(bool force) {
  this->report_enabled = true;
  this->force_report_ = force;
}

void ZigbeeAttribute::loop() {
  if (this->set_attr_requested_) {
    this->set_attr_();
  }

  if (!this->set_attr_requested_) {
    this->disable_loop();
  }
}

#ifdef USE_LIGHT
static float s_last_x[240] = {0.3127f};
static float s_last_y[240] = {0.3290f};
static bool s_last_initialized[240] = {false};

static void init_last_xy(uint8_t ep) {
  if (!s_last_initialized[ep]) {
    s_last_x[ep] = 0.3127f;
    s_last_y[ep] = 0.3290f;
    s_last_initialized[ep] = true;
  }
}

static void rgb_to_xy(float r, float g, float b, float &x, float &y) {
  r = (r > 0.04045f) ? powf((r + 0.055f) / 1.055f, 2.4f) : (r / 12.92f);
  g = (g > 0.04045f) ? powf((g + 0.055f) / 1.055f, 2.4f) : (g / 12.92f);
  b = (b > 0.04045f) ? powf((b + 0.055f) / 1.055f, 2.4f) : (b / 12.92f);

  float X = r * 0.4124f + g * 0.3576f + b * 0.1805f;
  float Y = r * 0.2126f + g * 0.7152f + b * 0.0722f;
  float Z = r * 0.0193f + g * 0.1192f + b * 0.9505f;

  float sum = X + Y + Z;
  if (sum == 0.0f) {
    x = 0.3127f;
    y = 0.3290f;
  } else {
    x = X / sum;
    y = Y / sum;
  }
}

static void xy_to_rgb(float x, float y, float &r, float &g, float &b) {
  if (y == 0.0f) {
    r = g = b = 0.0f;
    return;
  }
  float X = x / y;
  float Y = 1.0f;
  float Z = (1.0f - x - y) / y;

  r = X * 3.2406f + Y * -1.5372f + Z * -0.4986f;
  g = X * -0.9689f + Y * 1.8758f + Z * 0.0415f;
  b = X * 0.0557f + Y * -0.2040f + Z * 1.0570f;

  r = std::max(0.0f, std::min(1.0f, r));
  g = std::max(0.0f, std::min(1.0f, g));
  b = std::max(0.0f, std::min(1.0f, b));

  r = (r > 0.0031308f) ? (1.055f * powf(r, 1.0f / 2.4f) - 0.055f) : (r * 12.92f);
  g = (g > 0.0031308f) ? (1.055f * powf(g, 1.0f / 2.4f) - 0.055f) : (g * 12.92f);
  b = (b > 0.0031308f) ? (1.055f * powf(b, 1.0f / 2.4f) - 0.055f) : (b * 12.92f);
}

void ZigbeeAttribute::publish_state_to_zigbee() {
  if (this->light_ == nullptr)
    return;

  if (this->cluster_id_ == ESP_ZB_ZCL_CLUSTER_ID_ON_OFF && this->attr_id_ == 0x0000) {
    this->set_attr((bool) this->light_->remote_values.is_on());
  } else if (this->cluster_id_ == ESP_ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL && this->attr_id_ == 0x0000) {
    this->set_attr((uint8_t) (this->light_->remote_values.get_brightness() * 254.0f));
  } else if (this->cluster_id_ == ESP_ZB_ZCL_CLUSTER_ID_COLOR_CONTROL) {
    if (this->attr_id_ == 0x0008) {
      this->set_attr((uint8_t) 1);  // Color mode: XY coordinates
    } else if (this->attr_id_ == 0x0003 || this->attr_id_ == 0x0004) {
      float r = this->light_->remote_values.get_red();
      float g = this->light_->remote_values.get_green();
      float b = this->light_->remote_values.get_blue();
      float x, y;
      rgb_to_xy(r, g, b, x, y);
      init_last_xy(this->endpoint_id_);
      s_last_x[this->endpoint_id_] = x;
      s_last_y[this->endpoint_id_] = y;
      if (this->attr_id_ == 0x0003) {
        this->set_attr((uint16_t) (x * 65536.0f));
      } else {
        this->set_attr((uint16_t) (y * 65536.0f));
      }
    }
  }
}
#endif

void ZigbeeAttribute::on_value_received(uint8_t type, const void *value_p) {
#ifdef USE_BUTTON
  if (this->button_ != nullptr) {
    if (type == ESP_ZB_ZCL_ATTR_TYPE_BOOL && value_p != nullptr) {
      bool value = *(const bool *) value_p;
      if (value) {
        this->defer([this]() {
          this->button_->press();
          this->set_attr(false);
        });
      }
    }
  }
#endif
#ifdef USE_SWITCH
  if (this->switch_ != nullptr) {
    if (type == ESP_ZB_ZCL_ATTR_TYPE_BOOL && value_p != nullptr) {
      bool value = *(const bool *) value_p;
      this->defer([this, value]() { this->switch_->control(value); });
    }
  }
#endif
#ifdef USE_LIGHT
  if (this->light_ != nullptr) {
    if (this->cluster_id_ == ESP_ZB_ZCL_CLUSTER_ID_ON_OFF && this->attr_id_ == 0x0000) {
      if (type == ESP_ZB_ZCL_ATTR_TYPE_BOOL && value_p != nullptr) {
        bool value = *(const bool *) value_p;
        this->defer([this, value]() {
          auto call = this->light_->make_call();
          call.set_state(value);
          call.perform();
        });
      }
    } else if (this->cluster_id_ == ESP_ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL && this->attr_id_ == 0x0000) {
      if (type == ESP_ZB_ZCL_ATTR_TYPE_U8 && value_p != nullptr) {
        uint8_t value = *(const uint8_t *) value_p;
        this->defer([this, value]() {
          auto call = this->light_->make_call();
          call.set_brightness((float) value / 254.0f);
          call.perform();
        });
      }
    } else if (this->cluster_id_ == ESP_ZB_ZCL_CLUSTER_ID_COLOR_CONTROL) {
      if (this->attr_id_ == 0x0003 && type == ESP_ZB_ZCL_ATTR_TYPE_U16 && value_p != nullptr) {
        uint16_t raw_x = *(const uint16_t *) value_p;
        float x = (float) raw_x / 65536.0f;
        this->defer([this, x]() {
          init_last_xy(this->endpoint_id_);
          s_last_x[this->endpoint_id_] = x;
          float r, g, b;
          xy_to_rgb(s_last_x[this->endpoint_id_], s_last_y[this->endpoint_id_], r, g, b);
          auto call = this->light_->make_call();
          call.set_rgb(r, g, b);
          call.perform();
        });
      } else if (this->attr_id_ == 0x0004 && type == ESP_ZB_ZCL_ATTR_TYPE_U16 && value_p != nullptr) {
        uint16_t raw_y = *(const uint16_t *) value_p;
        float y = (float) raw_y / 65536.0f;
        this->defer([this, y]() {
          init_last_xy(this->endpoint_id_);
          s_last_y[this->endpoint_id_] = y;
          float r, g, b;
          xy_to_rgb(s_last_x[this->endpoint_id_], s_last_y[this->endpoint_id_], r, g, b);
          auto call = this->light_->make_call();
          call.set_rgb(r, g, b);
          call.perform();
        });
      }
    }
  }
#endif
}

}  // namespace esphome::zigbee

#endif
#endif
