import umqtt.simple as mqtt
import json
from machine import Timer
from utils.logger import Logger

class MQTTClient:
    def __init__(self, config):
        self.config = config
        self.logger = Logger()
        self.client = None
        self.is_connected = False
        self.message_queue = []
        
    def connect(self):
        """Connect to MQTT broker"""
        try:
            self.client = mqtt.MQTTClient(
                client_id="esp32_pv_tracker",
                server=self.config.MQTT_BROKER,
                port=self.config.MQTT_PORT
            )
            self.client.set_callback(self.message_received)
            self.client.connect()
            self.client.subscribe(self.config.MQTT_TOPIC_COMMANDS)
            self.is_connected = True
            self.logger.info("MQTT Connected successfully")
            return True
        except Exception as e:
            self.logger.error(f"MQTT connection failed: {e}")
            return False
    
    def message_received(self, topic, message):
        """Handle incoming MQTT messages from Python server"""
        try:
            command = json.loads(message)
            self.logger.info(f"Received command: {command}")
            
            # Handle different command types
            if command['type'] == 'set_angle':
                self.handle_angle_command(command['angle'])
            elif command['type'] == 'set_mode':
                self.handle_mode_command(command['mode'])
            elif command['type'] == 'calibrate':
                self.handle_calibration(command)
                
        except Exception as e:
            self.logger.error(f"Error processing MQTT message: {e}")
    
    def publish_sensor_data(self, sensor_data):
        """Publish sensor data to Python server"""
        if not self.is_connected:
            return False
            
        try:
            message = {
                'timestamp': sensor_data['timestamp'],
                'voltage': sensor_data['voltage'],
                'current': sensor_data['current'],
                'power': sensor_data['power'],
                'temperature': sensor_data['temperature'],
                'humidity': sensor_data['humidity'],
                'servo_angle': sensor_data['servo_angle'],
                'device_id': 'pv_tracker_001'
            }
            
            self.client.publish(
                self.config.MQTT_TOPIC_SENSOR,
                json.dumps(message)
            )
            return True
            
        except Exception as e:
            self.logger.error(f"MQTT publish failed: {e}")
            self.is_connected = False
            return False
    
    def keep_alive(self):
        """Maintain MQTT connection"""
        if self.is_connected:
            try:
                self.client.check_msg()  # Check for incoming messages
            except:
                self.is_connected = False
