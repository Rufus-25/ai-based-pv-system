# train_angle_model.py
import math
import numpy as np
import pandas as pd
from sklearn.linear_model import LinearRegression
from sklearn.ensemble import RandomForestRegressor
from sklearn.model_selection import cross_val_score, train_test_split
from sklearn.metrics import mean_absolute_error, mean_squared_error
import joblib

# ---------- USER SETTINGS ----------
MEASURED_CSV = "measured.csv"      # your experimental data
LOOKUP_CSV   = "lookup_table.csv"  # output
ARDUINO_OUT  = "lookup_table_arduino.txt"
MODEL_OUT    = "best_model.pkl"    # saved sklearn model

# ---------- 24 formulae -> produce y (then map to servo) ----------
def formula_y(month, hour):
    # hour is numeric (0-23, can be float)
    x = float(hour)
    m = int(month)
    # morning (x < 12) and afternoon (x > 12) coefficients (y = a*x + b)
    month_coeffs = {
        1:  (-14.8770, 180.7787,  14.8770, -176.2698),
        2:  (-14.8944, 180.3061,  14.8944, -177.1592),
        3:  (-14.9048, 179.5944,  14.9048, -178.1219),
        4:  (-14.8998, 178.7357,  14.8998, -178.8592),
        5:  (-14.8826, 177.9410,  14.8826, -179.2425),
        6:  (-14.8700, 177.5208,  14.8700, -179.3582),
        7:  (-14.8763, 177.7205,  14.8763, -179.3098),
        8:  (-14.8943, 178.4277,  14.8943, -179.0359),
        9:  (-14.9050, 179.2921,  14.9050, -178.4278),
        10: (-14.8993, 180.0845,  14.8993, -177.4992),
        11: (-14.8820, 180.6661,  14.8820, -176.5028),
        12: (-14.8699, 180.9198,  14.8699, -175.9577),
    }
    a_m, b_m, a_a, b_a = month_coeffs[m]
    if x < 12:
        y = a_m * x + b_m
    elif x == 12:
        y = 0.0
    else:
        y = a_a * x + b_a
    return y

def formula_servo_angle(month, hour):
    y = formula_y(month, hour)
    if hour < 12:
        angle = 90 - y
        if angle < 0:
            return 90.0
        return angle
    elif hour == 12:
        return 90.0
    else:
        angle = 90 + y
        if angle > 180:
            return 90.0
        return angle

# ---------- Build full formula table (month 1..12, hour 0..23) ----------
def build_formula_table(hour_step=1.0):
    rows = []
    for m in range(1,13):
        h = 0.0
        while h < 24.0:
            fa = formula_servo_angle(m, h)
            rows.append({'month': m, 'hour': h, 'formula_angle': fa})
            h += hour_step
    return pd.DataFrame(rows)

# ---------- Load measured data ----------
def load_measured(path):
    df = pd.read_csv(path)
    # expect columns: month, hour, measured_angle (voltage optional)
    required = ['month','hour','measured_angle']
    for r in required:
        if r not in df.columns:
            raise ValueError(f"measured.csv must contain column '{r}'")
    return df

# ---------- Feature engineering: add cyclical features ----------
def add_features(df):
    # month 1..12 -> 0..11
    df['month0'] = df['month'].astype(int) - 1
    df['hourf'] = df['hour'].astype(float)
    # cyclical transforms
    df['hour_sin'] = np.sin(2*np.pi*df['hourf']/24.0)
    df['hour_cos'] = np.cos(2*np.pi*df['hourf']/24.0)
    df['month_sin'] = np.sin(2*np.pi*df['month0']/12.0)
    df['month_cos'] = np.cos(2*np.pi*df['month0']/12.0)
    return df

# ---------- Main pipeline ----------
def main():
    print("Building formula table (hourly)...")
    formula_df = build_formula_table(hour_step=1.0)  # hourly; change to 0.5 for half-hour resolution
    # Load measured
    print("Loading measured data from", MEASURED_CSV)
    measured = load_measured(MEASURED_CSV)
    # Merge formula angle into measured dataset (on month,hour). If measured hour is float, we do close merge.
    # To be robust, round hours in measured to nearest integer to match formula hourly table
    measured['hour_round'] = measured['hour'].round(0).astype(int)
    merged = measured.merge(formula_df.rename(columns={'hour':'hour_round'}), on=['month','hour_round'], how='left')
    # If formula not found (e.g., measured had fractional hours), compute directly
    missing = merged['formula_angle'].isnull()
    if missing.any():
        print(f"Computing formula_angle for {missing.sum()} rows with fractional hours")
        for idx in merged[missing].index:
            m = int(merged.at[idx,'month'])
            h = float(merged.at[idx,'hour'])
            merged.at[idx,'formula_angle'] = formula_servo_angle(m,h)
    # rename hour round column back
    merged['hour'] = merged['hour']  # keep original
    merged = merged.rename(columns={'measured_angle':'measured_angle', 'formula_angle':'formula_angle'})

    # Build features
    df = merged[['month','hour','formula_angle','measured_angle']].copy()
    df = add_features(df)
    # Features to use
    feature_cols = ['month','hour','formula_angle','hour_sin','hour_cos','month_sin','month_cos']
    X = df[feature_cols].values
    y = df['measured_angle'].values

    # Train/test split
    X_train, X_test, y_train, y_test = train_test_split(X,y,test_size=0.2, random_state=42)

    # Linear regression
    print("Training LinearRegression...")
    lr = LinearRegression()
    lr.fit(X_train, y_train)
    y_pred_lr = lr.predict(X_test)
    rmse_lr = math.sqrt(mean_squared_error(y_test, y_pred_lr))
    mae_lr = mean_absolute_error(y_test, y_pred_lr)
    print(f"LinearRegression RMSE={rmse_lr:.3f}, MAE={mae_lr:.3f}")

    # Random forest
    print("Training RandomForestRegressor (this may take a moment)...")
    rf = RandomForestRegressor(n_estimators=200, random_state=42, n_jobs=-1)
    rf.fit(X_train, y_train)
    y_pred_rf = rf.predict(X_test)
    rmse_rf = math.sqrt(mean_squared_error(y_test, y_pred_rf))
    mae_rf = mean_absolute_error(y_test, y_pred_rf)
    print(f"RandomForest RMSE={rmse_rf:.3f}, MAE={mae_rf:.3f}")

    # Choose model logic: if RF significantly better (say >10% improvement) use RF, otherwise LR
    use_rf = (rmse_rf < 0.9 * rmse_lr)
    if use_rf:
        print("RandomForest performs better. Selecting RandomForest.")
        model = rf
    else:
        print("LinearRegression adequate. Selecting LinearRegression.")
        model = lr

    # Save model
    joblib.dump(model, MODEL_OUT)
    print("Saved model to", MODEL_OUT)

    # Create full lookup predictions for month=1..12 hour=0..23
    print("Generating lookup table for month/hour...")
    rows = []
    for m in range(1,13):
        for h in range(0,24):
            fa = formula_servo_angle(m, h)
            # construct features (must match training feature order)
            hour_sin = math.sin(2*math.pi*h/24.0)
            hour_cos = math.cos(2*math.pi*h/24.0)
            month0 = m-1
            month_sin = math.sin(2*math.pi*month0/12.0)
            month_cos = math.cos(2*math.pi*month0/12.0)
            feat = np.array([[m, float(h), fa, hour_sin, hour_cos, month_sin, month_cos]])
            pred = model.predict(feat)[0]
            # clamp and round to integer servo angle
            pred_clamped = int(round(max(0, min(180, pred))))
            rows.append({'month':m, 'hour':h, 'pred_angle':pred_clamped, 'formula_angle':fa})
    lookup_df = pd.DataFrame(rows)
    lookup_df.to_csv(LOOKUP_CSV, index=False)
    print("Lookup CSV written to", LOOKUP_CSV)

    # Write Arduino 2D array
    print("Writing Arduino array to", ARDUINO_OUT)
    with open(ARDUINO_OUT, 'w') as f:
        f.write("// Auto-generated lookup table: lookup[month-1][hour]\n")
        f.write("const uint8_t lookup[12][24] = {\n")
        for m in range(1,13):
            arr = lookup_df[lookup_df['month']==m]['pred_angle'].tolist()
            arr_str = ", ".join(str(a) for a in arr)
            f.write("  {" + arr_str + "},\n")
        f.write("};\n")
    print("Arduino array written.")

    # Print linear regression coefficients (helpful if you prefer a formula on ESP)
    if not use_rf:
        print("\nLinear regression coefficients (feature order):", feature_cols)
        print("Intercept:", lr.intercept_)
        print("Coefs:", lr.coef_)
        print("\nYou can implement: angle = intercept + sum(coef_i * feature_i) on the ESP32.")
    else:
        print("\nUsing RandomForest. Recommended: use the lookup array on the ESP32 (fast & tiny).")

    print("\nDone. Next steps:")
    print(" - Open", LOOKUP_CSV)
    print(" - Copy contents of", ARDUINO_OUT, "into your ESP32 sketch and use month/hour to index into lookup.")
    print(" - To improve model: append new measured rows to measured.csv and re-run this script.")

if __name__ == "__main__":
    main()
