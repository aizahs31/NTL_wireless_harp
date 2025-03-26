#define NUM_SENSORS 11
const int trigPins[NUM_SENSORS] = {22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42};
const int echoPins[NUM_SENSORS] = {23, 25, 27, 29, 31, 33, 35, 37, 39, 41, 43};

// Corrected note order
const char* notes[NUM_SENSORS] = {"C4", "C#4", "D4", "D#4", "E4", "F4", "F#4", "G4", "A4", "A#4", "B4"};

void setup() {
    Serial.begin(250000);  // Super fast baud rate
    for (int i = 0; i < NUM_SENSORS; i++) {
        pinMode(trigPins[i], OUTPUT);
        pinMode(echoPins[i], INPUT);
    }
}

void loop() {
    bool noteTriggered = false;

    for (int i = 0; i < NUM_SENSORS; i++) {
        digitalWrite(trigPins[i], LOW);
        delayMicroseconds(2);
        digitalWrite(trigPins[i], HIGH);
        delayMicroseconds(10);
        digitalWrite(trigPins[i], LOW);

        long duration = pulseIn(echoPins[i], HIGH, 25000); // Timeout to prevent lockup
        int distance = duration * 0.034 / 2;  // Convert time to cm

        if (distance > 0 && distance < 50) {  // Object within 50cm
            if (noteTriggered) Serial.print(",");  // Separate multiple notes
            Serial.print(notes[i]);
            noteTriggered = true;
        }
    }

    if (noteTriggered) Serial.println();  // Print only if notes are detected
}
