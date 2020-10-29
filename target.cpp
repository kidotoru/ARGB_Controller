#include "target.h"

Target::Target() {
  controlValue = 0;
  last_draw_time = 0;
}

Target::~Target() {

}

void Target::changeEffect(int effectIndex) {
  effect = effects[effectIndex];
  waitTime = effect->ini_waitTime;
  controlValue = 0;
}

void Target::draw() {
  if (effect != NULL) {
    if (last_draw_time + waitTime > millis()) {
      return;
    }
    last_draw_time = millis();
    effect->draw(startIndex, lastIndex, color, aled, controlValue);
  }
}
