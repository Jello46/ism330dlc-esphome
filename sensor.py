import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c, sensor
from esphome.const import CONF_ID, CONF_ADDRESS, UNIT_G, ICON_ACCELERATION

DEPENDENCIES = ["i2c"]

ism330dlc_level_ns = cg.esphome_ns.namespace("ism330dlc_level")
ISM330DLCLevelComponent = ism330dlc_level_ns.class_(
    "ISM330DLCLevelComponent", cg.PollingComponent, i2c.I2CDevice
)

CONF_ACCEL_X = "accel_x"
CONF_ACCEL_Y = "accel_y"
CONF_ACCEL_Z = "accel_z"

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(ISM330DLCLevelComponent),
            cv.Optional(CONF_ADDRESS, default=0x6A): cv.i2c_address,

            cv.Optional(CONF_ACCEL_X): sensor.sensor_schema(
                unit_of_measurement=UNIT_G, icon=ICON_ACCELERATION, accuracy_decimals=4
            ),
            cv.Optional(CONF_ACCEL_Y): sensor.sensor_schema(
                unit_of_measurement=UNIT_G, icon=ICON_ACCELERATION, accuracy_decimals=4
            ),
            cv.Optional(CONF_ACCEL_Z): sensor.sensor_schema(
                unit_of_measurement=UNIT_G, icon=ICON_ACCELERATION, accuracy_decimals=4
            ),
        }
    )
    .extend(cv.polling_component_schema("200ms"))
    .extend(i2c.i2c_device_schema(0x6A))
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    if CONF_ACCEL_X in config:
        sens = await sensor.new_sensor(config[CONF_ACCEL_X])
        cg.add(var.set_accel_x(sens))
    if CONF_ACCEL_Y in config:
        sens = await sensor.new_sensor(config[CONF_ACCEL_Y])
        cg.add(var.set_accel_y(sens))
    if CONF_ACCEL_Z in config:
        sens = await sensor.new_sensor(config[CONF_ACCEL_Z])
        cg.add(var.set_accel_z(sens))
