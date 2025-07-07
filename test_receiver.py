# --- Import Statements ---
import paho.mqtt.client as mqtt
import json
from datetime import datetime
from colorama import Fore, Style, init

# --- MQTT CONFIG ---
MQTT_SERVER = "" # IP address of your PI
MQTT_PORT = 1883
MQTT_TOPIC = "sensors/radar/status"

# --- COLOR CONFIG ---
# Colors for the Node Array (the ESP32 board itself)
NODE_COLORS = [Fore.GREEN, Fore.YELLOW, Fore.BLUE, Fore.MAGENTA, Fore.CYAN]

# Initialize colorama
init(autoreset=True)

# --- MQTT Callback Functions ---
def on_connect(client, userdata, flags, rc, properties=None):
    if rc == 0:
        print(f"{Fore.CYAN}---> Successfully connected to MQTT Broker at {MQTT_SERVER}")
        client.subscribe(MQTT_TOPIC)
        print(f"{Fore.CYAN}---> Subscribed to topic '{MQTT_TOPIC}'. Waiting for messages...")
    else:
        print(f"{Fore.RED}---> Failed to connect, return code {rc}")

# MODIFIED: on_message now handles the 'sensorType' field
def on_message(client, userdata, msg):
    try:
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        data = json.loads(msg.payload.decode("utf-8"))
        
        node_id = data.get("nodeId")
        status = data.get("status")
        sensor_type = data.get("sensorType", "Unknown") # Default to "Unknown" if not present

        if node_id is None or status is None:
            print(f"{Fore.YELLOW}[WARNING] Malformed message received: {msg.payload.decode()}")
            return

        # --- Color and formatting based on Node ID and Sensor Type ---
        color = NODE_COLORS[(node_id - 1) % len(NODE_COLORS)]
        
        # Build the base output string
        base_output = f"{color}NODE ARRAY ID: {node_id} | SENSOR: {sensor_type:<9} |"

        # Add details based on status
        if status == "motion_detected":
            status_text = "Presence Detected"
            details = ""
            # Add specific telemetry for the C4001 sensor
            if sensor_type == "C4001":
                range_cm = data.get("range_cm")
                speed_ms = data.get("speed_m_s")
                if range_cm is not None and speed_ms is not None:
                    details = f"| Range: {range_cm:.1f} cm | Speed: {speed_ms:.2f} m/s"

            print(f"{base_output} {Style.BRIGHT}{status_text:<17}{Style.NORMAL}{details} | Time: {timestamp}")

        elif status == "no_motion":
            status_text = "No Presence"
            print(f"{base_output} {status_text:<17} | Time: {timestamp}")

    except json.JSONDecodeError:
        print(f"{Fore.YELLOW}[WARNING] Could not decode JSON from payload: {msg.payload.decode()}")
    except Exception as e:
        print(f"{Fore.RED}[ERROR] An error occurred in on_message: {e}")

# --- Main Script Execution ---
if __name__ == "__main__":
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1)
    client.on_connect = on_connect
    client.on_message = on_message

    print("--- Multi-Sensor MQTT Logger Initializing ---")
    try:
        client.connect(MQTT_SERVER, MQTT_PORT, 60)
    except Exception as e:
        print(f"{Fore.RED}---> CRITICAL: Could not connect to {MQTT_SERVER}. Error: {e}")
        exit()

    client.loop_forever()
