import pygame
import serial
import time

# Initialize pygame mixer
pygame.mixer.init()
pygame.mixer.set_num_channels(11)  # 11 channels for 11 sensors

# Load harp sounds (Removed G# at index 8)
notes = {
    0: pygame.mixer.Sound('gpt_sounds/C4_harp_realistic.wav'),
    1: pygame.mixer.Sound('gpt_sounds/C#4_harp_realistic.wav'),
    2: pygame.mixer.Sound('gpt_sounds/D4_harp_realistic.wav'),
    3: pygame.mixer.Sound('gpt_sounds/D#4_harp_realistic.wav'),
    4: pygame.mixer.Sound('gpt_sounds/E4_harp_realistic.wav'),
    5: pygame.mixer.Sound('gpt_sounds/F4_harp_realistic.wav'),
    6: pygame.mixer.Sound('gpt_sounds/F#4_harp_realistic.wav'),
    7: pygame.mixer.Sound('gpt_sounds/G4_harp_realistic.wav'),
    8: pygame.mixer.Sound('gpt_sounds/A4_harp_realistic.wav'),
    9: pygame.mixer.Sound('gpt_sounds/A#4_harp_realistic.wav'),
    10: pygame.mixer.Sound('gpt_sounds/B4_harp_realistic.wav')
}

# Setup Serial connection (change port if needed)
ser = serial.Serial('COM10', 9600, timeout=0.05)
time.sleep(2)
ser.flushInput()

# Setup audio channels
channels = [pygame.mixer.Channel(i) for i in range(11)]
current_channel = 0

def play_note(note):
    """Plays a musical note using pygame mixer."""
    global current_channel
    if note in notes:
        print(f"🎵 Playing note {note}...")
        channels[current_channel].play(notes[note])  # Removed maxtime to allow full note play
        current_channel = (current_channel + 1) % len(channels)


def map_distance_to_note(distance):
    """Maps distance (5-40 cm) to a musical note index (0-10)."""
    note_ranges = [5, 8, 11, 14, 17, 20, 23, 26, 30, 34, 40]  # Adjusted for 11 notes
    for i, boundary in enumerate(note_ranges):
        if distance <= boundary:
            return i
    return -1


def read_distance():
    """Reads the closest valid distance from all sensors."""
    closest_distance = 300  # Start with a large value
    if ser.in_waiting > 0:
        try:
            line = ser.readline().decode('utf-8').strip()
            print(f"📡 Raw Serial Data: {line}")  # Debugging print
            
            parts = line.split()  # Split by space to separate sensor ID and distance
            if len(parts) == 2 and parts[1].isdigit():
                distance = int(parts[1])
                if 5 <= distance <= 40:
                    closest_distance = min(closest_distance, distance)  # Find the closest hand
                    print(f"✅ Distance detected: {distance} cm")
        except Exception as e:
            print(f"⚠️ Error reading data: {e}")
            ser.flushInput()
    return closest_distance if closest_distance != 300 else -1  # Return -1 if no valid data


def main():
    print("🎶 Starting Wireless Harp... 🎶")
    while True:
        distance = read_distance()
        if distance != -1:
            note = map_distance_to_note(distance)
            if note != -1:
                play_note(note)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n🚫 Stopping Harp...")
        ser.close()
