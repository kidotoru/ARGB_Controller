

#include "effect.h"
#ifndef TARGET_H
#define TARGET_H


class Target {
  public:
    int startIndex;
    int lastIndex;
    HSVColor color;
    ALED *aled;
    Effect *effects[10];
    Effect *effect;
    int controlValue;
    byte waitTime;
    unsigned long last_draw_time;

    Target();
    ~Target();
    virtual void changeEffect(int effectIndex);
    virtual void draw();
};

#endif
