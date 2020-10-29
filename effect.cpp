#include "effect.h"

// Static
// ただ点灯するのみ
Static::Static() {
  ini_waitTime = 255;
}

void Static::draw(int startIndex, int lastIndex, HSVColor color, ALED *aled, int& controlValue) {
  (void)controlValue;
  for (int i = startIndex; i <= lastIndex; i++) {
    aled->loadLedData(i, color);
  }
  aled->sendLedData();
}

// レインボー
// すべての色相をデバイスの端から端まで順次表示する
Rainbow::Rainbow() {
  ini_waitTime = 50;
}

void Rainbow::draw(int startIndex, int lastIndex, HSVColor color, ALED *aled, int& _offset) {
  (void) color;
  float tick = 360 / (float)(lastIndex - startIndex + 1);
  float _hue = 0;
  for (int i = startIndex; i <= lastIndex; i++) {
    int pos = i + _offset;
    pos = pos > lastIndex ? pos - lastIndex + startIndex -  1 : pos;

    aled->loadLedData(pos, HSVColor(_hue, color.sat, color.val));
    _hue += tick;
    _hue = _hue < 360 ? _hue : 0;
  }

  _offset++;
  _offset = _offset > lastIndex - startIndex ? 0 : _offset;
  aled->sendLedData();
}

// オーロラ
// 時間経過とともに、表示する色相をゆっくりと変化させる
// デバイス内でも、先頭から末尾に向かって色相を変化させる
Aurora::Aurora() {
  ini_waitTime = 25;
}

void Aurora::draw(int startIndex, int lastIndex, HSVColor color, ALED *aled, int& _hue) {
  (void) color;
  for (int i = startIndex; i <= lastIndex; i++) {
    float tmp = _hue + (i - startIndex) * 8;
    tmp = tmp < 360 ? tmp : tmp - 360;
    aled->loadLedData(i, HSVColor(tmp, color.sat, color.val));
  }
  _hue++;
  if (_hue > 360) {
    _hue = 0;
  }
  aled->sendLedData();
}

// ループ
// ごく短い光の帯を、デバイスの端から端まで移動させる
Loop::Loop() {
  ini_waitTime = 25;
}

void Loop::draw(int startIndex, int lastIndex, HSVColor color, ALED *aled, int& _offset) {
  for (int i = startIndex; i <= lastIndex; i++) {
    int pos = i + _offset;
    pos = pos > lastIndex ? pos - lastIndex + startIndex -  1 : pos;
    aled->loadLedData(pos, HSVColor(color.hue, color.sat, color.val * (float)((i - startIndex) / (lastIndex - startIndex))));
  }

  _offset++;
  _offset = _offset > lastIndex - startIndex ? 0 : _offset;
  aled->sendLedData();
}

// 心臓の鼓動
// 時間経過とともに明るさを変化させる
Heartbeat::Heartbeat() {
  ini_waitTime = 10;
}

void Heartbeat::draw(int startIndex, int lastIndex, HSVColor color, ALED *aled, int& _degree) {
  double radian = _degree * PI / 180.0;
  // TODO sin(radian) の処理が重く、
  // 複数デバイスで同時に実行したときと、１つだけで動かしたときで処理時間に差がでてしまうため、sinテーブル化を検討
  float v = (sin(radian) + 1) * 50;
  for (int i = startIndex; i <= lastIndex; i++) {
    aled->loadLedData(i, HSVColor(color.hue, color.sat, v));
  }
  _degree++;
  _degree = _degree >= 360 ? 0 : _degree;
  aled->sendLedData();
}


Blink::Blink() {
  ini_waitTime = 250;
}

void Blink::draw(int startIndex, int lastIndex, HSVColor color, ALED *aled, int& _state) {
  // _state は、0-7 のいずれか。0 で点灯、4 で消灯　それ以外は何もしない
  // waitTimeに最大255までしか設定できないため

  if (!_state) {
    for (int i = startIndex; i <= lastIndex; i++) {
      aled->loadLedData(i, color);
    }
    _state++;
  } else if (_state == 4) {
    for (int i = startIndex; i <= lastIndex; i++) {
      aled->loadLedData(i, HSVColor(0, 0, 0));
    }
    _state = 0;
  } else {
    _state++;
  }

  aled->sendLedData();
}

Random::Random() {
  ini_waitTime = 128;
}

void Random::draw(int startIndex, int lastIndex, HSVColor color, ALED *aled, int& controlValue) {
  (void)controlValue;
  for (int i = startIndex; i <= lastIndex; i++) {
    aled->loadLedData(i, HSVColor(random(0, 359), color.sat, color.val));
  }
  aled->sendLedData();
}

DoubleFlash::DoubleFlash() {
  ini_waitTime = 128;
}

void DoubleFlash::draw(int startIndex, int lastIndex, HSVColor color, ALED *aled, int& _state) {
  // _state は、0-7 のいずれか。0,2 で点灯、それ以外は消灯
  if (_state == 0 || _state == 2) {
    for (int i = startIndex; i <= lastIndex; i++) {
      aled->loadLedData(i, color);
    }
  } else {
    for (int i = startIndex; i <= lastIndex; i++) {
      aled->loadLedData(i, HSVColor(0, 0, 0));
    }
  }
  _state++;
  _state = _state > 7 ? 0 : _state;
  aled->sendLedData();
}
