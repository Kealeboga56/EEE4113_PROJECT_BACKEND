# EEE4113_PROJECT_BACKEND

# 🐧 Penguin Monitoring System

This project is a complete UX and data pipeline solution for monitoring penguin behavior using an ESP32 microcontroller. It collects real-time data, stores it, updates the database, and visualizes key insights via a Power BI dashboard.

## 📁 Project Structure

- **`Penguin UX design.pbix`**  
  Power BI file for visualizing penguin behavioral data, including metrics like weight trend, current weight,average weight and deviation for the average weight.

  this must be uploaded to the steakholders business microsoft account and must connect to a google sheet created them.

- **`Code.gs`**  
  Google script to update `penguin_db.csv` by processing incoming data or performing CRUD operations.

  this is what serves as the link to the google script as it accepts uploads from the app.py file

- **`app.py`**  
  Python Flask (or other framework) application that runs a background server to receive live data from the ESP32 and log it appropriately.

- **`penguin_db.csv`**  
  The local CSV-based database that stores historical penguin behavior records for visualization and analysis.

- **ESP32 Code**  
  Microcontroller code in `Manual_ENcorporation.ino` sends behavioral/environmental data over HTTP  to `app.py`.

---

## 🚀 Getting Started

### 1. Clone this repository
```bash
git clone https://github.com/Kealeboga56/EEE4113_PROJECT_BACKEND.git
cd EEE4113_PROJECT_BACKEND
