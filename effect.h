#include "aled.h"
#ifndef EFFECT_H
#define EFFECT_H

class Effect {
  public:
    ALED *aled;
    unsigned long ini_waitTime;
    
    Effect() {};
    virtual void draw(int startIndex, int lastIndex, HSVColor color, ALED *aled, int& controlValue) = 0;
};

class Static: public Effect {
  public:
    // using宣言 コンストラクタは基底クラスのものを使う>>> 必要？
    //using Effect::Effect;
    Static();
    virtual void draw(int startIndex, int lastIndex, HSVColor color, ALED *aled, int& controlValue) override;
};
class Rainbow: public Effect {
  public:
    Rainbow();
    virtual void draw(int startIndex, int lastIndex, HSVColor color, ALED *aled, int& controlValue) override;
};

class Aurora: public Effect {
  public:
    Aurora();
    virtual void draw(int startIndex, int lastIndex, HSVColor color, ALED *aled, int& controlValue) override;
};

class Loop: public Effect {
  public:
    Loop();
    virtual void draw(int startIndex, int lastIndex, HSVColor color, ALED *aled, int& controlValue) override;
};

class Heartbeat: public Effect {
  public:
    Heartbeat();
    virtual void draw(int startIndex, int lastIndex, HSVColor color, ALED *aled, int& controlValue) override;
};

class Blink: public Effect {
  public:
    Blink();
    virtual void draw(int startIndex, int lastIndex, HSVColor color, ALED *aled, int& controlValue) override;
};

class Random: public Effect {
  public:
    Random();
    virtual void draw(int startIndex, int lastIndex, HSVColor color, ALED *aled, int& controlValue) override;
};

class DoubleFlash: public Effect {
  public:
    DoubleFlash();
    virtual void draw(int startIndex, int lastIndex, HSVColor color, ALED *aled, int& controlValue) override;
};
#endif
