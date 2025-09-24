import numpy as np
import pandas as pd
from sklearn.ensemble import RandomForestRegressor
from sklearn.preprocessing import StandardScaler
import joblib
from loguru import logger
import sqlalchemy as db

class PowerPredictor:
    def __init__(self, config):
        self.config = config
        self.model = None
        self.scaler = StandardScaler()
        self.is_trained = False
        
    def load_model(self, model_path):
        """Load pre-trained model"""
        try:
            self.model = joblib.load(model_path)
            self.is_trained = True
            logger.info("AI model loaded successfully")
        except Exception as e:
            logger.error(f"Failed to load model: {e}")
            self.train_model()  # Train new model if loading fails
    
    def train_model(self, training_data):
        """Train the power prediction model"""
        try:
            # Feature engineering
            features = self.extract_features(training_data)
            targets = training_data['power_output']
            
            # Scale features
            features_scaled = self.scaler.fit_transform(features)
            
            # Train Random Forest model
            self.model = RandomForestRegressor(
                n_estimators=100,
                max_depth=10,
                random_state=42
            )
            
            self.model.fit(features_scaled, targets)
            self.is_trained = True
            
            # Save model
            joblib.dump(self.model, 'models/power_model.pkl')
            logger.info("Power prediction model trained and saved")
            
        except Exception as e:
            logger.error(f"Model training failed: {e}")
    
    def extract_features(self, data):
        """Extract features for AI model"""
        features = pd.DataFrame({
            'hour': data['timestamp'].dt.hour,
            'day_of_year': data['timestamp'].dt.dayofyear,
            'temperature': data['temperature'],
            'humidity': data['humidity'],
            'historical_irradiance': data['irradiance'],
            'season_sin': np.sin(2 * np.pi * data['timestamp'].dt.dayofyear / 365),
            'season_cos': np.cos(2 * np.pi * data['timestamp'].dt.dayofyear / 365)
        })
        return features
    
    def predict_next_day_power(self, current_conditions, weather_forecast):
        """Predict power output for next 24 hours"""
        if not self.is_trained:
            return None
            
        try:
            # Prepare prediction data
            prediction_data = self.prepare_prediction_data(
                current_conditions, 
                weather_forecast
            )
            
            # Make predictions
            predictions = self.model.predict(prediction_data)
            
            # Format results
            result = {
                'timestamp': pd.date_range(start=pd.Timestamp.now(), periods=24, freq='H'),
                'predicted_power': predictions,
                'confidence': self.calculate_confidence(predictions)
            }
            
            logger.info("Next-day power prediction completed")
            return result
            
        except Exception as e:
            logger.error(f"Prediction failed: {e}")
            return None
    
    def calculate_confidence(self, predictions):
        """Calculate prediction confidence based on variance"""
        return max(0, 1 - (np.std(predictions) / np.mean(predictions)))
