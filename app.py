import requests
from datetime import datetime
import socket
import json
from threading import Thread
import logging

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s',
    handlers=[
        logging.StreamHandler(),
        logging.FileHandler('esp32_server.log')
    ]
)

# Google Sheets configuration
GOOGLE_SCRIPT_URL = "https://script.google.com/macros/s/AKfycbxRFlDfxYKL0MHKnOBI_AMLXL73sp8cfzRPc871Es-q8vmZEkzpyQ1UsbRa94ZNIbU/exec"

# Network configuration
SERVER_IP = '0.0.0.0'  # Listen on all available network interfaces
SERVER_PORT = 12345     # Choose a port number

class ClientMonitor:
    def __init__(self):
        self.clients = {}
        
    def add_client(self, address):
        self.clients[address] = {
            'first_seen': datetime.now(),
            'last_seen': datetime.now(),
            'message_count': 0,
            'last_message': None
        }
        
    def update_client(self, address, message):
        if address not in self.clients:
            self.add_client(address)
            
        self.clients[address]['last_seen'] = datetime.now()
        self.clients[address]['message_count'] += 1
        self.clients[address]['last_message'] = message
        
    def get_status(self):
        status = {
            'total_clients': len(self.clients),
            'active_clients': sum(1 for c in self.clients.values() 
            if (datetime.now() - c['last_seen']).seconds < 60),
            'total_messages': sum(c['message_count'] for c in self.clients.values()),
            'clients': self.clients
        }
        return status

monitor = ClientMonitor()

def send_to_google_sheets(payload):
    """Send data to Google Sheets"""
    try:
        logging.info(f"Sending to Google Sheets: {payload}")
        response = requests.post(GOOGLE_SCRIPT_URL, json=payload, timeout=10)
        logging.info(f"Google Sheets Response: {response.status_code}, {response.text}")
        return response.status_code == 200
    except Exception as e:
        logging.error(f"Error sending to Google Sheets: {e}")
        return False

def handle_client(client_socket, address):
    """Handle incoming client connections"""
    logging.info(f"Connection established from {address}")
    
    try:
        # Receive data
        data = client_socket.recv(1024).decode('utf-8')
        if not data:
            return
            
        logging.info(f"Received data from {address}: {data}")
        monitor.update_client(address, data)
        
        # Parse JSON
        try:
            payload = json.loads(data)
            
            # Add timestamp if not provided
            if 'timestamp' not in payload:
                payload['timestamp'] = datetime.now().isoformat()
                
            # Send to Google Sheets
            if send_to_google_sheets(payload):
                response = "SUCCESS: Data sent to Google Sheets"
            else:
                response = "ERROR: Failed to send data to Google Sheets"
                
        except json.JSONDecodeError as e:
            response = f"ERROR: Invalid JSON format - {str(e)}"
            logging.error(response)
            
        # Send response back to ESP32
        client_socket.send(response.encode('utf-8'))
        
    except Exception as e:
        logging.error(f"Error handling client {address}: {e}")
    finally:
        client_socket.close()
        logging.info(f"Connection with {address} closed")

def start_server():
    """Start the TCP server"""
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.bind((SERVER_IP, SERVER_PORT))
    server.listen(5)
    logging.info(f"Server listening on {SERVER_IP}:{SERVER_PORT}")
    
    try:
        while True:
            client_sock, addr = server.accept()
            client_handler = Thread(
                target=handle_client,
                args=(client_sock, addr)
            )
            client_handler.start()
    except KeyboardInterrupt:
        logging.info("Server shutting down...")
        # Print final status
        logging.info("\nFinal Client Status:")
        logging.info(json.dumps(monitor.get_status(), indent=2, default=str))
    finally:
        server.close()

if __name__ == "__main__":
    start_server()