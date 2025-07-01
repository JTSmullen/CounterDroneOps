# --- Import Statements ---
import paho.mqtt.client as mqtt
import json
from datetime import datetime
from colorama import Fore, Style, init

# --- MQTT CONFIG (Must match your ESP32 and broker) ---
# IMPORTANT: Replace with the IP address of your Raspberry Pi!
# You can find it by running 'hostname -I' on the Pi.
MQTT_SERVER = "" # IP address of your PI
MQTT_PORT = 1883
MQTT_TOPIC = "sensors/radar/status" # The topic to subscribe to

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

# MODIFIED: The on_message function is now updated to handle the new, detailed JSON format.
def on_message(client, userdata, msg):
    """
    This function is called for every message received.
    It decodes the message, formats it, and prints it to the console.
    It now handles both simple ('no_motion') and detailed ('motion_detected') payloads.
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

        # Determine the color and text based on the status
        if status == "motion_detected":
            color = Fore.GREEN
            status_text = "Presence Detected"
            
            # NEW: Extract detailed motion data if it exists in the payload
            target_num = data.get("targetNumber")
            range_cm = data.get("range_cm")
            speed_ms = data.get("speed_m_s")
            
            # NEW: Build a detailed output string with the new data
            # The "is not None" check makes this backward compatible in case an old
            # sensor sends a simple "motion_detected" message.
            if range_cm is not None and speed_ms is not None:
                details = f"| Target: {target_num} | Range: {range_cm:.1f} cm | Speed: {speed_ms:.2f} m/s"
                print(f"{color}NODE - {node_id}: {status_text} {details} | Time: {timestamp}")
            else:
                # Fallback for the old, simple motion message
                print(f"{color}NODE - {node_id}: {status_text} | Time: {timestamp}")

        elif status == "no_motion":
            color = Fore.RED
            status_text = "No Presence"
            # We add padding to align the timestamp with the longer motion_detected line
            print(f"{color}NODE - {node_id}: {status_text}                                           | Time: {timestamp}")
        else:
            # Handle any other status messages
            color = Fore.YELLOW
            status_text = f"Unknown Status ({status})"
            print(f"{color}NODE - {node_id}: {status_text} | Time: {timestamp}")

    except json.JSONDecodeError:
        print(f"{Fore.YELLOW}[WARNING] Could not decode JSON from payload: {msg.payload.decode()}")
    except Exception as e:
        print(f"{Fore.RED}[ERROR] An error occurred in on_message: {e}")

# --- Main Script Execution ---

if __name__ == "__main__":
    # Use the newer callback API to avoid the DeprecationWarning
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1)

    # Assign the callback functions
    client.on_connect = on_connect
    client.on_message = on_message

    print("--- MQTT Logger Initializing ---")
    if not MQTT_SERVER:
        print(f"{Fore.RED}---> CRITICAL: MQTT_SERVER IP address is not set. Please edit the script and add it.")
        exit()

    try:
        # Connect to the broker
        client.connect(MQTT_SERVER, MQTT_PORT, 60)
    except Exception as e:
        print(f"{Fore.RED}---> CRITICAL: Could not connect to {MQTT_SERVER}. Please check the IP address and that Mosquitto is running.")
        print(f"{Fore.RED}---> Error details: {e}")
        exit()

    # loop_forever() is a blocking call that processes network traffic,
    # dispatches callbacks, and handles reconnecting automatically.
    # It's perfect for a script that just listens.
    client.loop_forever()
