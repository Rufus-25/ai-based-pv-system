import pandas as pd

# Define formulas for each month (before noon, after noon)
formulas = {
    "January":   {"before": lambda x: -14.8770*x + 180.7787, "after": lambda x: 14.8770*x - 176.2698},
    "February":  {"before": lambda x: -14.8944*x + 180.3061, "after": lambda x: 14.8944*x - 177.1592},
    "March":     {"before": lambda x: -14.9048*x + 179.5944, "after": lambda x: 14.9048*x - 178.1219},
    "April":     {"before": lambda x: -14.8998*x + 178.7357, "after": lambda x: 14.8998*x - 178.8592},
    "May":       {"before": lambda x: -14.8826*x + 177.9410, "after": lambda x: 14.8826*x - 179.2425},
    "June":      {"before": lambda x: -14.8700*x + 177.5208, "after": lambda x: 14.8700*x - 179.3582},
    "July":      {"before": lambda x: -14.8763*x + 177.7205, "after": lambda x: 14.8763*x - 179.3098},
    "August":    {"before": lambda x: -14.8943*x + 178.4277, "after": lambda x: 14.8943*x - 179.0359},
    "September": {"before": lambda x: -14.9050*x + 179.2921, "after": lambda x: 14.9050*x - 178.4278},
    "October":   {"before": lambda x: -14.8993*x + 180.0845, "after": lambda x: 14.8993*x - 177.4992},
    "November":  {"before": lambda x: -14.8820*x + 180.6661, "after": lambda x: 14.8820*x - 176.5028},
    "December":  {"before": lambda x: -14.8699*x + 180.9198, "after": lambda x: 14.8699*x - 175.9577},
}

rows = []

# Generate data for all 24 hours
for month, funcs in formulas.items():
    for hour in range(1, 25):  # 1 to 24 inclusive
        if hour < 12:   # Before noon
            y = funcs["before"](hour)
            servo_angle = 90 - y
            if servo_angle < 0:
                servo_angle = 90
        elif hour == 12:  # Noon
            y = 0
            servo_angle = 90
        else:  # After noon
            y = funcs["after"](hour)
            servo_angle = 90 + y
            if servo_angle > 180:
                servo_angle = 90
        
        rows.append([month, hour, y, servo_angle])

# Create DataFrame
df = pd.DataFrame(rows, columns=["Month", "Hour", "Y_value", "Servo_Angle"])

# Save to Excel
df.to_excel("formula_angles_full.xlsx", index=False)
