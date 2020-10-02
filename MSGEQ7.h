#ifndef MSGEQ7__H
#define MSGEQ7__H

#include <Arduino.h>

// Number of bands of output by the hardware IC
#define MAX_BAND 7

class MSGEQ7 {
  public:
    /*
     * Class Constructor
     *
     * Instantiate a new instance of the class. The parameters passed are used to
     * connect the software to the hardware. Multiple instances may co-exist.
     */
    MSGEQ7(uint8_t resetPin, uint8_t strobePin, uint8_t dataPin):
      _resetPin(resetPin), _strobePin(strobePin), _dataPin(dataPin)
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
    uint16_t get(uint8_t band);

  private:
    // Pins to interact with the IC
    uint8_t _resetPin;
    uint8_t _strobePin;
    uint8_t _dataPin;

    // Array of all input values
    uint16_t _data[MAX_BAND];
};

#endif
