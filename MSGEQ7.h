#ifndef MSGEQ7__H
#define MSGEQ7__H

#include <Arduino.h>

// Number of bands of output by the hardware IC
#define MAX_BAND 7
#define LEFT 0
#define RIGHT 1

class MSGEQ7 {
  public:
    /*
     * Class Constructor
     *
     * Instantiate a new instance of the class. The parameters passed are used to
     * connect the software to the hardware.
     */
    MSGEQ7(uint8_t resetPin, uint8_t strobePin, uint8_t dataLeftPin, uint8_t dataRightPin):
      _resetPin(resetPin), _strobePin(strobePin), _dataLeftPin(dataLeftPin), _dataRightPin(dataRightPin)
      {};

    /*
     * Class Destructor
     *
     * Does the necessary to clean up once the object is no longer required
     */
    ~MSGEQ7(void) {};

    /*
     * Initialize the object
     */
    void begin(void);

    /*
     * Reset the IC before initiating a read
     */
    void reset(void);

    /*
     * Read the next set of data from the IC
     */
    void read(bool bReset = true);

    /*
     * Get a specific value from the data read
     */
    uint16_t get(uint8_t side, uint8_t band);

  private:
    // Pins to interact with the IC
    uint8_t _resetPin;
    uint8_t _strobePin;
    uint8_t _dataLeftPin;
    uint8_t _dataRightPin;

    // Array of all input values
    uint16_t _dataLeft[MAX_BAND];
    uint16_t _dataRight[MAX_BAND];
};

#endif
