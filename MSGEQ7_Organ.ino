#define msg7RESET 13
#define msg7Strobe 12
#define msg7DCout 0

// Potentiometers
#define pBassCutoff 1
#define pMidCutoff 2
#define pHighCutoff 3

const int LEDpins[3] = {3, 5, 6};
const int bin_size = 7;
int bins[3];
char buffer[256];
int bass_cut_save = 0; // 72
int mid_cut_save = 0; // 74
int high_cut_save = 0; // 75

void setup() {
  Serial.begin(250000);
  Serial.println("Setup");

  for (int x = 0 ; x < 3 ; x++) {
    pinMode(LEDpins[x], OUTPUT);
    bins[x] = 0;
  }

  pinMode(msg7RESET, OUTPUT);
  pinMode(msg7Strobe, OUTPUT);
}

void loop() {
  // Reset the MSGEQ7
  digitalWrite(msg7RESET, HIGH);
  delay(5);
  digitalWrite(msg7RESET, LOW);

  // Check potentiometers
  int bass_cut_raw = analogRead(pBassCutoff);
  int bass_cut_val = map(bass_cut_raw, 0, 1023, 0, 256);
  if (bass_cut_val != bass_cut_save) {
    sprintf(buffer, "Bass Cut: %d", bass_cut_val);
    Serial.println(buffer);
    bass_cut_save = bass_cut_val;
  }

  int mid_cut_raw = analogRead(pMidCutoff);
  int mid_cut_val = map(mid_cut_raw, 0, 1023, 0, 256);
  if (mid_cut_val != mid_cut_save) {
    sprintf(buffer, "Mid Cut: %d", mid_cut_val);
    Serial.println(buffer);
    mid_cut_save = mid_cut_val;
  }

  int high_cut_raw = analogRead(pHighCutoff);
  int high_cut_val = map(high_cut_raw, 0, 1023, 0, 256);
  if (high_cut_val != high_cut_save) {
    sprintf(buffer, "High Cut: %d", high_cut_val);
    Serial.println(buffer);
    high_cut_save = high_cut_val;
  }

  // Check all 7 Bands
  for (int x = 0 ; x < bin_size ; x++) {
    // Pull Strobe Low
    digitalWrite(msg7Strobe, LOW);
    delayMicroseconds(35);

    // Read Value
    int spectrumRead = analogRead(msg7DCout);

    // We want to split the 7 bands into 3 bins
    // 0 -> 63hz + 160Hz
    // 1 -> 400Hz + 1kHz
    // 2 -> 2.5kHz + 6.25kHz + 16kHz
    if (x == 0 || x == 1) {
      bins[0] += spectrumRead;
    } else if (x == 2 || x == 3) {
      bins[1] += spectrumRead;
    } else {
      bins[2] += spectrumRead;
    }

    // Pull Strobe High
    digitalWrite(msg7Strobe, HIGH);
  }

  for (int x = 0 ; x < 3 ; x++) {
    int val = 0;

    if (x == 0 || x == 1) {
      val = bins[x] / 2;
    } else {
      val = bins[x] / 3;
    }
    bins[x] = 0;

    int PWMValue = map(val, 0, 1023, 0, 255);
    if (
        (x == 0 && PWMValue < bass_cut_save) ||
        (x == 1 && PWMValue < mid_cut_save) ||
        (x == 2 && PWMValue < high_cut_save)
    ) {
      PWMValue = 0;
    }
    int output_val = constrain(PWMValue, 0, 255);
    
    analogWrite(LEDpins[x], output_val);
    
    if(output_val != 0 && x == 0)
      //digitalWrite(LEDpins[x], HIGH);
      Serial.println(output_val);
    
    /*else
      digitalWrite(LEDpins[x], LOW);
    */
  }
}
