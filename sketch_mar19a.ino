import pygame
import serial
import time
import threading
import os
import sys
os.environ['PYGAME_HIDE_SUPPORT_PROMPT'] = "hide"

# Give this process maximum priority
try:
    import psutil
    p = psutil.Process(os.getpid())
    p.nice(psutil.HIGH_PRIORITY_CLASS)
except:
    print("Could not set high priority - continuing anyway")

# Initialize pygame mixer with extreme low latency settings
pygame.mixer.pre_init(44100, -16, 2, 64)  # Ultra small buffer (64) for absolute minimum delay
pygame.init()
pygame.mixer.init()
pygame.mixer.set_num_channels(22)  # Double the channels for overlapping notes

print("Loading sounds...")
# Load harp sounds
notes = {}
sound_files = [
    'C4_harp_realistic.wav', 'C#4_harp_realistic.wav', 
    'D4_harp_realistic.wav', 'D#4_harp_realistic.wav',
    'E4_harp_realistic.wav', 'F4_harp_realistic.wav', 
    'F#4_harp_realistic.wav', 'G4_harp_realistic.wav',
    'A4_harp_realistic.wav', 'A#4_harp_realistic.wav', 
    'B4_harp_realistic.wav'
]

# Load sounds with error handling
for i, filename in enumerate(sound_files):
    try:
        filepath = f'gpt_sounds/{filename}'
        notes[i] = pygame.mixer.Sound(filepath)
        notes[i].set_volume(1.0)  # Maximum volume
    except Exception as e:
        print(f"Error loading {filepath}: {e}")
        sys.exit(1)

# Preload all sounds for instant response
print("Preloading sounds...")
for sound in notes.values():
    sound.play()
    sound.stop()
print("✓ Sounds ready for instant playback")

# Track serial data for debugging
last_lines = []
MAX_LINES = 50

# Setup Serial connection with maximum speed
ser = None
for port in ["COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9", "COM10"]:
    try:
        ser = serial.Serial(port, 115200, timeout=0.0)  # Zero timeout for non-blocking
        print(f"✓ Connected to {port}")
        break
    except:
        pass

if not ser:
    print("❌ Could not connect to any COM port. Please check connections.")
    print("Available ports:")
    try:
        import serial.tools.list_ports
        ports = serial.tools.list_ports.comports()
        for p in ports:
            print(f"  - {p}")
    except:
        print("  Could not list available ports")
    sys.exit(1)

# Flush serial buffer
ser.reset_input_buffer()
ser.reset_output_buffer()

# Increase serial buffer size
try:
    ser.set_buffer_size(rx_size=4096, tx_size=4096)
except:
    pass  # Not all platforms support this

# Track active sensors
active_sensors = {}
sensor_lock = threading.Lock()

# Setup audio channels with maximum volume
channels = [pygame.mixer.Channel(i) for i in range(22)]
for channel in channels:
    channel.set_volume(1.0)

# Track channel usage
channel_allocation = [-1] * 22  # -1 means available
channel_lock = threading.Lock()

# Direct mapping of sensor to primary channel
def get_channel_for_sensor(sensor_id):
    primary = sensor_id % 11
    backup = primary + 11
    return primary, backup

def play_note(sensor_id, note):
    """Plays a note with absolute minimum delay."""
    primary, backup = get_channel_for_sensor(sensor_id)
    
    # Try primary first, then backup
    channel_to_use = primary if not channels[primary].get_busy() else backup
    
    # Play immediately with no delay
    channels[channel_to_use].play(notes[note])
    
    # Debug - extreme minimal logging
    if sensor_id >= 7:  # Only debug the problematic sensors
        print(f"▶ S{sensor_id}→N{note}")

def map_distance_to_note(distance):
    """Optimized mapping function."""
    if distance < 5 or distance > 50:
        return -1
    
    # Use lookup table for even faster mapping
    ranges = [5, 9, 14, 19, 23, 28, 32, 37, 41, 46, 50]
    for i, max_range in enumerate(ranges):
        if distance <= max_range:
            return 10 - i  # Reverse order: closer = higher note
    return 0

def read_sensors():
    """Ultra optimized sensor reading thread."""
    global active_sensors, last_lines
    
    # Sensor data processing
    while True:
        # Read all available data immediately
        if ser.in_waiting > 0:
            try:
                # Non-blocking read
                line = ser.readline().decode('utf-8').strip()
                if line:
                    # Keep recent data for debugging
                    last_lines.append(line)
                    if len(last_lines) > MAX_LINES:
                        last_lines.pop(0)
                    
                    # Fast split and parse
                    parts = line.split()
                    if len(parts) == 2 and parts[0].isdigit() and parts[1].isdigit():
                        sensor_id = int(parts[0])
                        distance = int(parts[1])
                        
                        # Only log problematic sensors
                        if sensor_id >= 7:
                            print(f"📡 S{sensor_id}:{distance}cm")
                        
                        # Process distance
                        if 0 < distance <= 50:
                            # Immediate mapping
                            note = map_distance_to_note(distance)
                            
                            # Skip further processing if no valid note
                            if note == -1:
                                continue
                            
                            # Fast lock and update
                            with sensor_lock:
                                is_new = (sensor_id not in active_sensors or 
                                         active_sensors[sensor_id]['note'] != note)
                                active_sensors[sensor_id] = {
                                    'distance': distance,
                                    'note': note,
                                    'is_new': is_new
                                }
                        elif sensor_id in active_sensors:
                            with sensor_lock:
                                del active_sensors[sensor_id]
            except Exception as e:
                # Fast error recovery - just continue
                pass

def play_notes():
    """Ultra optimized note playing thread."""
    global active_sensors
    
    while True:
        to_play = {}
        
        # Minimal lock time
        with sensor_lock:
            for sid, data in active_sensors.items():
                if data.get('is_new', True):
                    to_play[sid] = data['note']
                    data['is_new'] = False
        
        # Play each note without any delay
        for sensor_id, note in to_play.items():
            play_note(sensor_id, note)

def debug_thread():
    """Show sensor status periodically."""
    while True:
        time.sleep(5)  # Update every 5 seconds
        
        with sensor_lock:
            active = list(active_sensors.keys())
        
        print(f"\nActive sensors: {active}")
        print(f"Serial buffer: {ser.in_waiting} bytes")
        print("Recent data:")
        for line in last_lines[-5:]:
            print(f"  {line}")
        print()

def main():
    """Main function with improved startup."""
    print("\n🎶 ULTRA LOW LATENCY WIRELESS HARP 🎶")
    print("-----------------------------------")
    
    # Start threads
    threads = []
    
    # Sensor thread at highest priority
    sensor_thread = threading.Thread(target=read_sensors, daemon=True)
    sensor_thread.start()
    threads.append(sensor_thread)
    
    # Notes thread second priority
    notes_thread = threading.Thread(target=play_notes, daemon=True)
    notes_thread.start()
    threads.append(notes_thread)
    
    # Debug thread lowest priority
    debug_thread_obj = threading.Thread(target=debug_thread, daemon=True)
    debug_thread_obj.start()
    threads.append(debug_thread_obj)
    
    print("✓ All systems running!")
    print("Press Ctrl+C to stop")
    
    try:
        # Keep main thread alive but responsive
        while all(t.is_alive() for t in threads):
            time.sleep(0.1)
    except KeyboardInterrupt:
        print("\n🚫 Stopping Harp...")
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n🚫 Stopping Harp...")
    finally:
        if ser:
            ser.close()
        pygame.mixer.quit()
        
