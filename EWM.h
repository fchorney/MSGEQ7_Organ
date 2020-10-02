#ifndef EWM_H_
#define EWM_H_

class EWM {
  public:
    double output = 0;
    double alpha = 0;

    EWM(double alpha);

    double filter(double input);
  private:
    bool hasInitial = false;
};

#endif
