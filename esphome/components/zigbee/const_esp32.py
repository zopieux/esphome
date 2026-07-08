import esphome.codegen as cg

DEVICE_TYPE = "device_type"
ROLE = "role"
CONF_MAX_EP_NUMBER = 239
CONF_NUM = "num"
CONF_CLUSTERS = "clusters"
CONF_ATTRIBUTES = "attributes"
CONF_ENDPOINT = "endpoint"
CONF_CLUSTER = "cluster"
SCALE = "scale"
CONF_ATTRIBUTE_ID = "attribute_id"
KEY_BS_EP = "binary_sensor_ep"
KEY_SENSOR_EP = "sensor_ep"
KEY_SWITCH_EP = "switch_ep"
KEY_LIGHT_EP = "light_ep"
KEY_BUTTON_EP = "button_ep"

ha_standard_devices = cg.esphome_ns.enum("zb_ha_standard_devs_e")
DEVICE_ID = {
    "RANGE_EXTENDER": ha_standard_devices.ZB_HA_RANGE_EXTENDER_DEVICE_ID,
    "SIMPLE_SENSOR": ha_standard_devices.ZB_HA_SIMPLE_SENSOR_DEVICE_ID,
    "CUSTOM_ATTR": ha_standard_devices.ZB_HA_CUSTOM_ATTR_DEVICE_ID,
    "ON_OFF_OUTPUT": ha_standard_devices.ZB_HA_ON_OFF_OUTPUT_DEVICE_ID,
    "COLOR_DIMMABLE_LIGHT": ha_standard_devices.ZB_HA_COLOR_DIMMABLE_LIGHT_DEVICE_ID,
}
cluster_id = cg.esphome_ns.enum("esp_zb_zcl_cluster_id_t")
CLUSTER_ID = {
    "BASIC": cluster_id.ESP_ZB_ZCL_CLUSTER_ID_BASIC,
    "BINARY_INPUT": cluster_id.ESP_ZB_ZCL_CLUSTER_ID_BINARY_INPUT,
    "ANALOG_INPUT": cluster_id.ESP_ZB_ZCL_CLUSTER_ID_ANALOG_INPUT,
    "ON_OFF": cluster_id.ESP_ZB_ZCL_CLUSTER_ID_ON_OFF,
    "LEVEL_CONTROL": cluster_id.ESP_ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL,
    "COLOR_CONTROL": cluster_id.ESP_ZB_ZCL_CLUSTER_ID_COLOR_CONTROL,
}
cluster_role = cg.esphome_ns.enum("esp_zb_zcl_cluster_role_t")
CLUSTER_ROLE = {
    "SERVER": cluster_role.ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
}
attr_type = cg.esphome_ns.enum("esp_zb_zcl_attr_type_t")
ATTR_TYPE = {
    "BOOL": attr_type.ESP_ZB_ZCL_ATTR_TYPE_BOOL,
    "8BITMAP": attr_type.ESP_ZB_ZCL_ATTR_TYPE_8BITMAP,
    "CHAR_STRING": attr_type.ESP_ZB_ZCL_ATTR_TYPE_CHAR_STRING,
    "SINGLE": attr_type.ESP_ZB_ZCL_ATTR_TYPE_SINGLE,
    "DOUBLE": attr_type.ESP_ZB_ZCL_ATTR_TYPE_DOUBLE,
    "U8": attr_type.ESP_ZB_ZCL_ATTR_TYPE_U8,
    "U16": attr_type.ESP_ZB_ZCL_ATTR_TYPE_U16,
    "U32": attr_type.ESP_ZB_ZCL_ATTR_TYPE_U32,
    "8BIT_ENUM": attr_type.ESP_ZB_ZCL_ATTR_TYPE_8BIT_ENUM,
    "16BIT_ENUM": attr_type.ESP_ZB_ZCL_ATTR_TYPE_16BIT_ENUM,
}
