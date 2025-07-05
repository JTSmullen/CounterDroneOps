# --- Import Statements ---
import paho.mqtt.client as mqtt
import json
from datetime import datetime
from colorama import Fore, Style, init

# --- MQTT CONFIG (Must match your ESP32 and broker) ---
# IMPORTANT: Replace with the IP address of your Raspberry Pi!
# You can find it by running 'hostname -I' on the Pi.
MQTT_SERVER = "192.168.68.64" # IP address of your PI
MQTT_PORT = 1883
MQTT_TOPIC = "sensors/radar/status" # The topic to subscribe to

# NEW: Define a list of colors to cycle through for each node ID.
# You can add or change any colors from the colorama library.
NODE_COLORS = [
    Fore.GREEN,
    Fore.YELLOW,
    Fore.BLUE,
    Fore.MAGENTA,
    Fore.CYAN,
    Fore.LIGHTRED_EX,
    Fore.LIGHTBLUE_EX,
    Fore.LIGHTYELLOW_EX,
]

# Initialize colorama. `autoreset=True` ensures the color of each print
# statement doesn't affect the next one.
init(autoreset=True)

# --- MQTT Callback Functions ---

# The callback for when the client connects to the MQTT broker.
def on_connect(client, userdata, flags, rc, properties=None):
    """
    This function is called when the script successfully connects to the MQTT broker.
    """
    if rc == 0:
        print(f"{Fore.CYAN}---> Successfully connected to MQTT Broker at {MQTT_SERVER}")
        # Subscribe to the topic once connected
        client.subscribe(MQTT_TOPIC)
        print(f"{Fore.CYAN}---> Subscribed to topic '{MQTT_TOPIC}'. Waiting for messages...")
    else:
        print(f"{Fore.RED}---> Failed to connect, return code {rc}")
        if rc == 5:
            print(f"{Fore.RED}---> Connection refused: Check Mosquitto config for 'allow_anonymous true'.")

# MODIFIED: The on_message function now colors output based on node ID.
def on_message(client, userdata, msg):
    """
    This function is called for every message received.
    It decodes the message, formats it, and prints it to the console.
    The color of the output is now determined by the sensor's node ID.
    """
    try:
        # Get the current time and format it nicely
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

        # Decode the payload from bytes to a string and then parse the JSON
        data = json.loads(msg.payload.decode("utf-8"))
        
        # Extract the node ID and status
        node_id = data.get("nodeId")
        status = data.get("status")

        if node_id is None or status is None:
            # Handle messages that don't have the expected keys
            print(f"{Fore.YELLOW}[WARNING] Malformed message received: {msg.payload.decode()}")
            return

        # --- MODIFIED: Color selection logic ---
        # We use the modulo operator (%) to cycle through our list of colors.
        # This ensures that even if we have more nodes than colors, we'll just loop back to the beginning.
        # We use (node_id - 1) because lists are 0-indexed but your node IDs likely start at 1.
        color = NODE_COLORS[(node_id - 1) % len(NODE_COLORS)]

        # Determine the text based on the status
        if status == "motion_detected":
            status_text = "Presence Detected"
            
            # Extract detailed motion data if it exists
            target_num = data.get("targetNumber")
            range_cm = data.get("range_cm")
            speed_ms = data.get("speed_m_s")
            
            # Build a detailed output string with the new data
            if range_cm is not None and speed_ms is not None:
                # NOTE: The bug in your previous code 'range_cm*100' is corrected. 
                # The ESP32 already sends the value in cm.
                details = f"| Target: {target_num} | Range: {range_cm:.1f} cm | Speed: {speed_ms:.2f} m/s"
                print(f"{color}NODE - {node_id}: {status_text} {details} | Time: {timestamp}")
            else:
                # Fallback for a simple motion message
                print(f"{color}NODE - {node_id}: {status_text} | Time: {timestamp}")

        elif status == "no_motion":
            status_text = "No Presence"
            # We add padding to align the timestamp with the longer motion_detected line
            print(f"{color}NODE - {node_id}: {status_text}                                           | Time: {timestamp}")
        else:
            # Handle any other status messages
            status_text = f"Unknown Status ({status})"
            print(f"{color}NODE - {node_id}: {status_text} | Time: {timestamp}")

    except json.JSONDecodeError:
        print(f"{Fore.YELLOW}[WARNING] Could not decode JSON from payload: {msg.payload.decode()}")
    except Exception as e:
        print(f"{Fore.RED}[ERROR] An error occurred in on_message: {e}")

# --- Main Script Execution ---
if __name__ == "__main__":
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1)
    client.on_connect = on_connect
    client.on_message = on_message

    print("--- MQTT Logger Initializing ---")
    if not MQTT_SERVER:
        print(f"{Fore.RED}---> CRITICAL: MQTT_SERVER IP address is not set. Please edit the script and add it.")
        exit()

    try:
        client.connect(MQTT_SERVER, MQTT_PORT, 60)
    except Exception as e:
        print(f"{Fore.RED}---> CRITICAL: Could not connect to {MQTT_SERVER}. Please check the IP address and that Mosquitto is running.")
        print(f"{Fore.RED}---> Error details: {e}")
        exit()

    client.loop_forever()
