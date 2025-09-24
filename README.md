# AI-Based Photovoltaic Tracking and Monitoring System 🔋☀️

A hybrid IoT system that combines ESP32-based real-time control with Python-powered AI analytics for intelligent solar energy optimization.

## 🌟 Overview

This project implements a smart single-axis solar tracking system that uses Artificial Intelligence to maximize energy harvesting from photovoltaic panels. The system features real-time monitoring, predictive power forecasting, and AI-driven fault detection through a hybrid architecture combining MicroPython on ESP32 with a Python AI server.

<!-- ![System Architecture](docs/images/system_architecture.png) -->
[System Architecture diagram]

## ✨ Key Features

- **🤖 AI-Powered Optimization**: Machine learning models for optimal panel orientation and power forecasting
- **📊 Real-time Monitoring**: Live data visualization through Blynk IoT dashboard
- **⚠️ Predictive Maintenance**: Early fault detection for motors, sensors, and panel degradation
- **🌐 Hybrid Architecture**: ESP32 handles real-time control while Python server processes complex AI tasks
- **📱 Remote Control**: Web and mobile dashboard for system management
- **🔧 Fault Resilience**: Self-healing capabilities and graceful degradation

## 🏗️ System Architecture

### Hybrid Design Pattern

```
┌─────────────────┐    MQTT/WiFi    ┌──────────────────┐
│   ESP32 Device  │ ◄─────────────► │  Python AI Server│
│ (MicroPython)   │                 │ (RPi/Cloud/PC)   │
├─────────────────┤                 ├──────────────────┤
│ • Sensor Reading│                 │ • AI Predictions │
│ • Motor Control │                 │ • Fault Analysis │
│ • Basic Logic   │                 │ • Data Storage   │
│ • Blynk Comm    │                 │ • API Integration│
└─────────────────┘                 └──────────────────┘
```

### Data Flow

1. **ESP32** collects sensor data (voltage, current, temperature, humidity)
2. **MQTT** transmits data to Python server for AI processing
3. **AI Server** returns optimal angles and power predictions
4. **ESP32** adjusts panel position and updates Blynk dashboard
5. **Users** monitor system through web/mobile interface

## 📁 Project Structure

```
AI-PV-Tracker-Hybrid/
├── ESP32_Firmware/          # MicroPython code for ESP32
├── Server/                  # AI processing and analytics powered by Python
├── Shared_Protocols/        # Communication standards
├── Documentation/           # Setup and API docs
├── Deployment/              # Docker and deployment scripts
└── Hardware/               # Schematics and BOM
```

## 🚀 Quick Start

### Prerequisites

- **Hardware**:
  - ESP32 development board
  - 10W Solar Panel (18V)
  - MG996R Servo Motor
  - INA219 Voltage/Current Sensor
  - DHT11 Temperature/Humidity Sensor
  - Jumper wires and breadboard
  - 12V Power supply

- **Software**:
  - Python 3.8+
  - MicroPython firmware
  - Mosquitto MQTT broker
  - Blynk IoT account

### Installation

#### 1. ESP32 Setup

```bash
# Flash MicroPython to ESP32
cd Deployment/
./esp32_flash_script.sh

# Upload firmware
cd ../ESP32_Firmware/
ampy --port /dev/ttyUSB0 put src/
ampy --port /dev/ttyUSB0 put config.py
ampy --port /dev/ttyUSB0 put main.py
```

#### 2. Python Server Setup

```bash
cd Server/

# Install dependencies
pip install -r requirements.txt

# Configure environment
cp config/sample_settings.py config/settings.py
# Edit settings with your credentials

# Start the server
python server.py
```

#### 3. MQTT Broker Setup

```bash
# Install Mosquitto
sudo apt-get install mosquitto mosquitto-clients

# Start broker
sudo systemctl start mosquitto
```

#### 4. Blynk Dashboard Setup

1. Create account at [Blynk Cloud](https://blynk.cloud/)
2. Import dashboard template from `blynk_dashboard/`
3. Update auth token in `ESP32_Firmware/secrets.py`

### Configuration

Edit `ESP32_Firmware/config.py`:

```python
# WiFi Settings
WIFI_SSID = "your_wifi_ssid"
WIFI_PASSWORD = "your_wifi_password"

# MQTT Settings
MQTT_BROKER = "192.168.1.100"  # Your server IP
MQTT_PORT = 1883

# Blynk Settings
BLYNK_AUTH_TOKEN = "your_blynk_token"
```

## 🎯 Usage

### Starting the System

1. **Power up the ESP32** - It will automatically connect to WiFi and MQTT
2. **Start Python AI Server**:

   ```bash
   cd Python_Server/
   python server.py
   ```

3. **Access Blynk Dashboard** - Monitor real-time data and predictions
4. **View AI Analytics** - Access server dashboard at `http://localhost:5000`

### Dashboard Features

- **Real-time Monitoring**: Voltage, current, power, temperature, humidity
- **AI Predictions**: Next-day power forecast with confidence scores
- **Fault Alerts**: Early warnings for system issues
- **Manual Control**: Override AI for manual panel positioning
- **Historical Data**: Energy production trends and analysis

## 🤖 AI Capabilities

### Power Forecasting

- **24-hour predictions** using historical data and weather forecasts
- **Random Forest regression** model trained on local conditions
- **Confidence scoring** for prediction reliability

### Fault Detection

- **Anomaly detection** for sensor malfunctions
- **Motor health monitoring** through current analysis
- **Panel degradation** tracking over time

### Optimization Algorithms

- **Reinforcement learning** for optimal tracking angles
- **Weather adaptation** for cloudy/rainy conditions
- **Energy efficiency** maximization

## 📊 Performance Metrics

| Metric | Target | Actual |
|--------|--------|--------|
| Energy Gain vs Fixed Panel | 15-25% | [Results]% |
| Prediction Accuracy (R²) | >0.85 | [Results] |
| Fault Detection Precision | >0.90 | [Results] |
| System Uptime | >99% | [Results]% |

## 🔧 Troubleshooting

### Common Issues

1. **ESP32 not connecting to WiFi**
   - Check WiFi credentials in `secrets.py`
   - Verify router compatibility (2.4GHz only)

2. **MQTT connection failures**
   - Confirm Mosquitto broker is running
   - Check firewall settings for port 1883

3. **Sensor reading errors**
   - Verify I2C connections for INA219
   - Check DHT11 wiring and pull-up resistors

4. **Servo motor not moving**
   - Ensure adequate power supply (5-6V)
   - Check PWM signal on correct pin

### Debug Mode

Enable detailed logging by setting `DEBUG = True` in `config.py`:

```python
# ESP32_Firmware/config.py
DEBUG = True
LOG_LEVEL = "DEBUG"
```

## 📈 Extending the Project

### Adding New Sensors

1. Implement sensor class in `ESP32_Firmware/src/sensors/`
2. Add to `SensorManager` integration
3. Update MQTT message format in `Shared_Protocols/`

### Custom AI Models

1. Place model files in `Server/models/`
2. Update `ai_engine/model_manager.py`
3. Retrain with your dataset using `ai_training/` scripts

### Dashboard Customization

1. Modify Blynk widget layout
2. Add new data streams to MQTT topics
3. Create custom visualizations in web dashboard

## 🤝 Contributing

We welcome contributions! Please see our [Contributing Guidelines](CONTRIBUTING.md) for details.

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- **Supervisor**: Prof. S.A. Oyetunji, Federal University of Technology, Akure
- **Blynk IoT** for the excellent IoT platform
- **MicroPython** community for ESP32 support
- **NASA POWER** API for solar irradiance data

## 📞 Support

- **Issues**: [GitHub Issues](https://github.com/yourusername/ai-pv-tracker/issues)
- **Email**: <your.email@futa.edu.ng>
- **Documentation**: [Full Documentation](Documentation/)

## 🎓 Academic Reference

This project was developed as a final year undergraduate thesis in the Department of Electrical and Electronics Engineering, Federal University of Technology, Akure.

**Student**: Taiwo Rufus Covenant (EEE/19/1397)  
**Supervisor**: Prof. S.A. Oyetunji  
**Institution**: Federal University of Technology, Akure, Nigeria

<!-- --- -->

<!-- <div align="center">

*"Harnessing AI for a Sustainable Energy Future"* ☀️🤖

**Built with ❤️ for renewable energy innovation**

</div> -->
