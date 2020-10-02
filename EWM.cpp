#include "EWM.h"

EWM::EWM(double alpha) {
  this->alpha = alpha;
}

double EWM::filter(double input) {
  if (hasInitial) {
    output = (alpha * input) + ((1 - alpha) * output);
  } else {
    output = input;
    hasInitial = true;
  }
  return output;
}
