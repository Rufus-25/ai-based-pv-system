# System Configuration
class Config:
    # WiFi Settings
    WIFI_SSID = "" # Enter your WiFi here
    WIFI_PASSWORD = "" # Enter your WiFi password here
    
    # MQTT Settings (Communication with Python Server)
    MQTT_BROKER = "192.168.1.100"  # Your Python server IP
    MQTT_PORT = 1883
    MQTT_TOPIC_SENSOR = "pv_tracker/sensors"
    MQTT_TOPIC_COMMANDS = "pv_tracker/commands"
    
    # Blynk Settings
    BLYNK_AUTH_TOKEN = "YOUR_BLYNK_TOKEN"
    
    # Hardware Pins
    SERVO_PIN = 13
    DHT_PIN = 4
    SDA_PIN = 21
    SCL_PIN = 22
    
    # Timing Intervals (seconds)
    SENSOR_READ_INTERVAL = 5
    MQTT_PUBLISH_INTERVAL = 10
    BLYNK_UPDATE_INTERVAL = 5
    SERVO_UPDATE_INTERVAL = 60
    
    # Safety Limits
    MAX_TEMPERATURE = 80.0
    MAX_CURRENT = 3.0
    VOLTAGE_THRESHOLD = 5.0