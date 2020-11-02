#include <math.h>
#include "EWM.h"
#include "MSGEQ7.h"

#define msg7Strobe 7
#define msg7RESET 8
#define msg7Right A4
#define msg7Left A5

// Potentiometers
#define pBeeCutoff A0
#define pBassCutoff A1
#define pMidCutoff A2
#define pHighCutoff A3

EWM beePot(0.01);
EWM bassPot(0.01);
EWM midPot(0.01);
EWM highPot(0.01);

int bee_cut_save = 0;
int bass_cut_save = 0; // 72
int mid_cut_save = 0; // 74
int high_cut_save = 0; // 75

MSGEQ7 eq7(msg7RESET, msg7Strobe, msg7Left, msg7Right);

const int LEDpins[2][3] = {{3, 5, 6}, {11, 10, 9}};
const int bin_size = 7;
uint16_t bins[2][3];
char buffer[256];

#define MAX_SAMPLES 10
#define AVG_CUT 2
#define NOISE_FLOOR 80

uint16_t volumes[2][MAX_BAND];
uint16_t raw_v[2][MAX_BAND][MAX_SAMPLES];
int sample_count = 0;

// [63hz, 160Hz, 400Hz, 1kHz, 2.5kHz, 6.25kHz, 16kHz]
// Power required in DB (lower number = louder)
// [0, -1.3, -5.4, -8.3, -8.7, -13.9, -9.3]

uint16_t noise_floors[MAX_BAND] = {30, 30, 30, 30, 30, 30, 30};

uint16_t expander_thresholds[MAX_BAND]    = { 50, 100,  50,  40, 120, 190, 250};
double expander_multipliers[MAX_BAND]     = {2.5, 1.5, 3.0, 2.0, 2.0, 2.5, 3.0};

uint16_t compressor_thresholds[MAX_BAND]  = {500, 700, 250, 300, 1000, 900, 800};
double compressor_ratios[MAX_BAND]        = {4.0, 2.0, 3.0, 3.5, 4.0,  3.0, 2.0};
double compressor_gain = 1.4;

void setup() {
  Serial.begin(250000);
  Serial.println("Setup");

  // Initialize Potentiometers
  pinMode(pBeeCutoff, INPUT);
  pinMode(pBassCutoff, INPUT);
  pinMode(pMidCutoff, INPUT);
  pinMode(pHighCutoff, INPUT);

  // Initialize LED Pins and Bins
  for (int i = 0 ; i < 2 ; i++) {
    for (int j = 0 ; j < 3 ; j++) {
      pinMode(LEDpins[i][j], OUTPUT);
      bins[i][j] = 0;
    }
  }

  // Initialize Volumes
  for (int i = 0 ; i < 2 ; i++) {
    for (int j = 0 ; j < MAX_BAND ; j++) {
      volumes[i][j] = 0;
    }
  }

  // Clear Raw Data
  clear_raw();

  // Initialize eq7
  eq7.begin();
}

void clear_raw() {
  for (int i = 0 ; i < 2 ; i++) {
    for (int j = 0 ; j < MAX_BAND ; j++) {
      for (int k = 0 ; k < MAX_SAMPLES ; k++) {
        raw_v[i][j][k] = 0;
      }
    }
  }
}

void read_pot(const char *_name, EWM pot, int pin, int *save, bool _print=false) {
  int raw = pot.filter(analogRead(pin));
  int value = map(raw, 0, 1023, 0, 255);
  if (value != *save && abs(value - *save) >= 5) {
    if (_print) {
      sprintf(buffer, "%s: %d", _name, value);
      Serial.println(buffer);
    }
    *save = value;
  }
}

uint16_t newMap(uint16_t x, uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max) {
  return (x - in_min) * ((out_max - out_min) / (in_max - in_min)) + out_min;
}

void isort(uint16_t a[], int len) {
  for (int i = 0; i < len; i++) {
    uint16_t temp = a[i];
    int j = i - 1;
    while (j >= 0 && a[j] > temp) {
      a[j + 1] = a[j];
      j = j - 1;
    }
    a[j + 1] = temp;
  }
}

uint16_t expander(uint16_t value, uint16_t threshold, double multiplier, uint16_t min_value=0, uint16_t max_value=1023) {
  if (value < threshold) {
    return constrain(uint16_t(value * multiplier), min_value, max_value);
  } else {
    return value;
  }
}

uint16_t compressor(uint16_t value, uint16_t threshold, double ratio, double gain=1.0, uint16_t min_value=0, uint16_t max_value=1023) {
  if (ratio > max_value) {
    ratio = max_value;
  }

  if (value > threshold) {
    uint16_t diff = value - threshold;
    uint16_t cut = uint16_t(diff / ratio);
    uint16_t below = value - diff;
    return constrain((below + 1 + cut) * gain, min_value, max_value);
  } else {
    return value * gain;
  }
}


void loop() {
  bool print_pots = true;

  // Check potentiometers
  read_pot("Bee Cut", beePot, pBeeCutoff, &bee_cut_save, print_pots);
  read_pot("Bass Cut", bassPot, pBassCutoff, &bass_cut_save, print_pots);
  read_pot("Mid Cut", midPot, pMidCutoff, &mid_cut_save, print_pots);
  read_pot("High Cut", highPot, pHighCutoff, &high_cut_save, print_pots);

  if (sample_count < MAX_SAMPLES) {
    eq7.read();

    for (int i = 0 ; i < 2 ; i++) {
      for (int j = 0 ; j < MAX_BAND ; j++) {
        // Grab raw value
        uint16_t raw = eq7.get(i, j);

        sprintf(buffer, "RAW [%s : %02d | %04d]", i == LEFT ? " LEFT" : "RIGHT", j, raw);
        //Serial.println(buffer);

        // Get rid of noise floor
        uint16_t value = newMap(constrain(raw, NOISE_FLOOR, 1023), NOISE_FLOOR, 1023, 0, 1023);
        raw_v[i][j][sample_count] = value;
      }
    }
    sample_count++;
  } else {
    for (int i = 0 ; i < 2 ; i++) {
      for (int j = 0 ; j < MAX_BAND ; j++) {
        // Sort each sub array
        isort(raw_v[i][j], MAX_SAMPLES);

        // Average all values but the smallest and biggest
        uint16_t avg = 0;
        for (int k = AVG_CUT ; k < MAX_SAMPLES - AVG_CUT ; k++) {
          avg += raw_v[i][j][k];
        }
        avg = avg / (MAX_SAMPLES - (AVG_CUT * 2));

        uint16_t value = avg;

        // Expand and Compress
        //uint16_t value = expander(avg, expander_thresholds[i], expander_multipliers[i]);
        //value = compressor(value, compressor_thresholds[i], compressor_ratios[i], compressor_gain);

        // Peak compressor (Do we need this?)
        // value = compressor(value, 1000, 1023, 1.0);

        sprintf(buffer, "AVG [%s : %02d | %04d]", i == LEFT ? " LEFT" : "RIGHT", j, value);
        //Serial.println(buffer);
        volumes[i][j] = value;
      }
    }

    for (int i = 0 ; i < 2 ; i++) {
      for (int j = 0 ; j < MAX_BAND ; j++) {
        uint16_t value = volumes[i][j];

        // We want to split the 7 bands into 3 bins
        // 0 -> 63hz + 160Hz
        // 1 -> 400Hz + 1kHz
        // 2 -> 2.5kHz + 6.25kHz + 16kHz
        if (j == 0 || j == 1)
          bins[i][0] += value;
        else if (j == 2 || j == 3)
          bins[i][1] += value;
        else
          bins[i][2] += value;
      }
    }

    // Average out the bins and find the highest bin
    uint16_t highest[2] = {0, 0};
    for (int i = 0 ; i < 2 ; i++) {
      for (int j = 0 ; j < 3 ; j++) {
        if (j == 0 || j == 1)
          bins[i][j] = bins[i][j] / 2;
        else
          bins[i][j] = bins[i][j] / 3;

        // Do we want to normalize over both left and right, or individually?
        if (bins[i][j] > highest[i])
          highest[i] = bins[i][j];
      }
    }

    // Find the normalized multiplier
    double normalizer[2] = {0, 0};
    for (int i = 0 ; i < 2 ; i++)
      normalizer[i] = 1023 / highest[i];


    for (int i = 0 ; i < 2 ; i++) {
      for (int j = 0 ; j < 3 ; j++) {
        uint16_t value = bins[i][j] * normalizer[i];
        bins[i][j] = 0;

        // Turn into volume
        uint16_t vol = uint16_t(20.0 * log10((double)value));
        int PWMValue = map(vol, 0, 60, 0, 255);
        int output_val = constrain(PWMValue, 0, 255);

        sprintf(buffer,"[ %02d | %02d | %02d | %03d ]", i, j, LEDpins[i][j], output_val);
        Serial.println(buffer);

        analogWrite(LEDpins[i][j], output_val);
      }
    }

    clear_raw();
    sample_count = 0;
  }
}

/*
  for (int x = 0 ; x < bin_size ; x++) {
    // Read Value
    int spectrumRead = eq7.get(x);

    // Multiply Values To Reach Equal Level Per Frequency
    //    63Hz = 585 * 1.00
    //   160Hz = 554 * 1.06
    //   400Hz = 573 * 1.02
    //    1kHz = 534 * 1.10
    //  2.5kHz = 483 * 1.21
    // 6.25kHz = 303 * 1.93
    //   16kHz = 317 * 1.85

    switch (x) {
      case 0:
        break;
      case 1:
        spectrumRead *= 1.06;
        break;
      case 2:
        spectrumRead *= 1.02;
        break;
      case 3:
        spectrumRead *= 1.10;
        break;
      case 4:
        spectrumRead *= 1.21;
        break;
      case 5:
        spectrumRead *= 1.93;
        break;
      case 6:
        spectrumRead *= 1.85;
        break;
      default:
        break;
    }
    if (x == 0) {
      spectrumRead *= 1;
    } else if (x == 1) {
      spectrumRead *= 1.06;
    }

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

    //if(output_val != 0 && x == 0)
      //digitalWrite(LEDpins[x], HIGH);
      //Serial.println(output_val);

    else
      digitalWrite(LEDpins[x], LOW);

  }
}
*/
