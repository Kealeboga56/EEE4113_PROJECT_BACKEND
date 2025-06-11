# EEE4113_PROJECT_BACKEND

# 🐧 Penguin Monitoring System

This project is a complete UX and data pipeline solution for monitoring penguin behavior using an ESP32 microcontroller. It collects real-time data, stores it, updates the database, and visualizes key insights via a Power BI dashboard.

## 📁 Project Structure

- **`Penguin UX design.pbix`**  
  Power BI file for visualizing penguin behavioral data, including metrics like movement, temperature, and interaction.

- **`Code.js`**  
  JavaScript script to update `penguin_db.csv` by processing incoming data or performing CRUD operations.

- **`app.py`**  
  Python Flask (or other framework) application that runs a background server to receive live data from the ESP32 and log it appropriately.

- **`penguin_db.csv`**  
  The local CSV-based database that stores historical penguin behavior records for visualization and analysis.

- **ESP32 Code**  
  Microcontroller code (not included here) sends behavioral/environmental data over HTTP or MQTT to `app.py`.

---

## 🚀 Getting Started

### 1. Clone this repository
```bash
git clone https://github.com/Kealeboga56/EEE4113_PROJECT_BACKEND.git
cd EEE4113_PROJECT_BACKEND
