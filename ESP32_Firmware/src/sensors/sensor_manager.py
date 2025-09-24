import time
from machine import I2C, Pin
from .power_sensor import PowerSensor
from .environment_sensor import EnvironmentSensor
from utils.logger import Logger

class SensorManager:
    def __init__(self, config):
        self.config = config
        self.logger = Logger()
        
        # Initialize I2C for power sensor
        i2c = I2C(0, sda=Pin(config.SDA_PIN), scl=Pin(config.SCL_PIN))
        
        # Initialize sensors
        self.power_sensor = PowerSensor(i2c)
        self.env_sensor = EnvironmentSensor(config.DHT_PIN)
        
        self.last_read_time = 0
        
    def read_all_sensors(self):
        """Read all sensors and return formatted data"""
        current_time = time.time()
        
        # Only read sensors at specified interval
        if current_time - self.last_read_time < self.config.SENSOR_READ_INTERVAL:
            return None
            
        try:
            sensor_data = {
                'timestamp': current_time,
                'voltage': self.power_sensor.read_voltage(),
                'current': self.power_sensor.read_current(),
                'temperature': self.env_sensor.read_temperature(),
                'humidity': self.env_sensor.read_humidity()
            }
            
            # Calculate derived values
            sensor_data['power'] = sensor_data['voltage'] * sensor_data['current']
            
            # Validate data
            if self.validate_sensor_data(sensor_data):
                self.last_read_time = current_time
                return sensor_data
            else:
                self.logger.warning("Invalid sensor data detected")
                return None
                
        except Exception as e:
            self.logger.error(f"Sensor read error: {e}")
            return None
    
    def validate_sensor_data(self, data):
        """Validate sensor readings for plausibility"""
        if (data['voltage'] < 0 or data['voltage'] > 25 or
            data['current'] < 0 or data['current'] > 5 or
            data['temperature'] < -10 or data['temperature'] > 100):
            return False
        return True
