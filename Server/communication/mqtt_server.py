import paho.mqtt.client as mqtt
import json
from loguru import logger
from ai_engine.power_predictor import PowerPredictor
from ai_engine.fault_detector import FaultDetector

class MQTTServer:
    def __init__(self, config):
        self.config = config
        self.client = mqtt.Client()
        self.power_predictor = PowerPredictor(config)
        self.fault_detector = FaultDetector(config)
        
        # Set up MQTT callbacks
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message
        
    def on_connect(self, client, userdata, flags, rc):
        """MQTT connection callback"""
        if rc == 0:
            logger.info("MQTT Server connected successfully")
            client.subscribe("pv_tracker/sensors")
        else:
            logger.error(f"MQTT connection failed with code {rc}")
    
    def on_message(self, client, userdata, msg):
        """Handle incoming sensor data from ESP32"""
        try:
            sensor_data = json.loads(msg.payload.decode())
            logger.info(f"Received sensor data: {sensor_data}")
            
            # Process data through AI pipeline
            self.process_sensor_data(sensor_data)
            
        except Exception as e:
            logger.error(f"Error processing MQTT message: {e}")
    
    def process_sensor_data(self, sensor_data):
        """Process sensor data through AI models"""
        # 1. Fault detection
        faults = self.fault_detector.analyze(sensor_data)
        
        if faults:
            self.send_fault_alerts(faults, sensor_data['device_id'])
        
        # 2. Power prediction (every hour)
        if self.should_predict_power(sensor_data):
            prediction = self.power_predictor.predict_next_day_power(
                sensor_data, 
                self.get_weather_forecast()
            )
            
            if prediction:
                self.send_prediction_to_esp32(prediction, sensor_data['device_id'])
    
    def send_prediction_to_esp32(self, prediction, device_id):
        """Send AI predictions back to ESP32"""
        command = {
            'type': 'power_prediction',
            'prediction': prediction,
            'timestamp': pd.Timestamp.now().isoformat()
        }
        
        self.client.publish(
            f"pv_tracker/commands/{device_id}",
            json.dumps(command)
        )
        logger.info("Sent power prediction to ESP32")
    
    def start_server(self):
        """Start the MQTT server"""
        try:
            self.client.connect(self.config.MQTT_BROKER, self.config.MQTT_PORT, 60)
            self.client.loop_start()
            logger.info("MQTT server started")
        except Exception as e:
            logger.error(f"Failed to start MQTT server: {e}")
