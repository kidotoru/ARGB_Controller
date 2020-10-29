//---------------------------------------------------------
// アドレサブルLEDコントローラ
//---------------------------------------------------------

// 初期化要求
// RST
// EEPROM address 0 - 512 を0x00 で埋める

// セットアップ要求
// STG [ALED num] [Target num]   ex: STG 5 2
// STA [aled index:0-4] [ledCount]  ex: STA 0 24
// STT [Target index:0-7] [start index] [last index] [aled index] [color hue] [color sat] [color val]
//   ex: STT 0 0 7 0 0 100 100
//   ex: STT 1 8 23 0 0 100 100
// 先にSTA,STTを行ってからSTGを行う
//
// セットアップコマンド
// STA 0 24
// STT 0 0 7 0 0 100 100
// STT 1 8 23 0 0 100 100
// STG 5 2

// EEPROM address 0 - 511 value 0 - 255
// 0: ALED num   min=1,max=5 セットアップ要求でsave
// 1: Target num min=1,max=8 セットアップ要求でsave
// 2 - 7 RFU
// 8 - 15 ALED  [aled_index + 8]
//    8:ALED_CH1 ledCount セットアップ要求でsave
//    9:ALED_CH2 ledCount セットアップ要求でsave
//   10:ALED_CH3 ledCount セットアップ要求でsave
//   11:ALED_CH4 ledCount セットアップ要求でsave
//   12:ALED_CH5 ledCount セットアップ要求でsave
//   13:RFU
//   14:RFU
//   15:RFU
// 16 - 23 Target01 [16 + Target_index * 8 + 0,1,2,3,4,5,6,7]
//   16:start index セットアップ要求でsave
//   17:last index  セットアップ要求でsave
//   18:aled index  セットアップ要求でsave
//   19:color hue   セットアップ要求/色変更時にsave
//   20:color sat   セットアップ要求/色変更時にsave
//   21:color val   セットアップ要求/色変更時にsave
//   22:effect idx  セットアップ要求時に0セット,エフェクト変更時にsave
//   23:waitTime    セットアップ要求時に0セット,速度変更時,エフェクト変更時にsave
// 24 - 31 Target02
// 32 - 39 Target03
// 40 - 47 Target04
// 48 - 55 Target05
// 56 - 63 Target06
// 64 - 71 Target07
// 72 - 79 Target08

// EEP
// EEPROM の内容をシリアル通信する

#include <EEPROM.h>
#include "target.h"

int ales_num = 0;
ALED **aleds = new ALED*[1];

// 制御するデバイス数
// ファンの数＝デバイス数出ないことに注意。
// 例えばファンとリングを別々に制御する場合、ファンは１つでもデバイス数は2になる
int target_num = 0;
Target **targets;

// エフェクト
// todo 各デバイス共通のセットアップ のfor　の前でnew する?
Static static1 = Static();
Rainbow rainbow1 = Rainbow();
Aurora aurora1 = Aurora();
Loop loop1 = Loop();
Heartbeat heartbeat1 = Heartbeat();
Blink blink1 = Blink();
Random random1 = Random();
DoubleFlash doubleFlash1 = DoubleFlash();

// 起動後、一度だけ呼び出される処理
void setup() {
  // 使うピンを出力に設定
  pinMode(14, OUTPUT);  // チャネル1 Pin14, PC0
  pinMode(15, OUTPUT);  // チャネル2 Pin15, PC1
  pinMode(16, OUTPUT);  // チャネル3 Pin16, PC2
  pinMode(17, OUTPUT);  // チャネル4 Pin17, PC3
  pinMode(18, OUTPUT);  // チャネル5 Pin18, PC4
  pinMode(8, OUTPUT);   // LED

  // シリアル通信初期化
  Serial.begin(9600);
  while (!Serial) {
  };
  Serial.println("\n\n-----------------------");
  Serial.println("Program Start.....");

  // 起動時チェック
  // EEPROM の、address 0,1 がそれぞれ1-5, 1-8 の範囲外の場合処理を中止
  ales_num = EEPROM.read(0);
  target_num = EEPROM.read(1);
  if (!(ales_num >= 1 && ales_num <= 5 && target_num >= 1 && target_num <= 8)) {
    return;
  }

  // ALED のインスタンス生成
  for (int i = 0; i < ales_num; i++) {
    // TODO CH1,CH2 をどうするか？
    aleds[i] = new ALED(CH1 | CH2 | CH3 | CH4 | CH5, EEPROM.read(8 + i));
  }

  // 制御対象デバイスのセットアップ
  setupTarget();
  Serial.println("setup Complete.....");
  // オープニングアニメーション
  for (int i = 0; i < 1; i++) {
    opening();
  }

  for (int i = 0; i < 2; i++) {
    // [16 + Target_index * 8 + 0,1,2,3,4,5,6,7]
    // 初期色
    float h = 0;
    float s = 0;
    float v = 0;
    // 0.5倍で保存されているので、２倍にしてセット
    h = EEPROM.read(16 + i * 8 + 3) * 2;
    s = EEPROM.read(16 + i * 8 + 4) * 2;
    v = EEPROM.read(16 + i * 8 + 5) * 2;
    targets[i]->color = HSVColor(h, s, v);
    // 初期エフェクト
    targets[i]->changeEffect(EEPROM.read(16 + i * 8 + 6));
    // 初期スピード
    targets[i]->waitTime = EEPROM.read(16 + i * 8 + 7);
  }

  Serial.println("Complete.....");
}

void loop() {
  String cmd;
  if (Serial.available()) {
    // シリアル通信に受信があった場合、通信内容に応じた処理を行う
    cmd = Serial.readStringUntil('\n');
    Serial.println(cmd);
    cmd.toUpperCase();
    executeCommond(cmd);
  }
  if (ales_num >= 1 && ales_num <= 5 && target_num >= 1 && target_num <= 8) {
    // 各デバイスのLED点灯処理
    for (int i = 0; i < target_num; i++) {
      targets[i]->draw();
    }
  }
}

// 各デバイスのセットアップ。
void setupTarget() {
  targets = new Target*[target_num];
  for (int i = 0; i < target_num; i++) {
    targets[i] = new Target();
    targets[i]->startIndex = EEPROM.read(16 + i * 8 + 0);
    targets[i]->lastIndex  = EEPROM.read(16 + i * 8 + 1);
    targets[i]->aled = aleds[EEPROM.read(16 + i * 8 + 2)];
    targets[i]->color = HSVColor(240, 100, 100);
    targets[i]->effects[0] = &static1;
    targets[i]->effects[1] = &rainbow1;
    targets[i]->effects[2] = &aurora1;
    targets[i]->effects[3] = &loop1;
    targets[i]->effects[4] = &heartbeat1;
    targets[i]->effects[5] = &blink1;
    targets[i]->effects[6] = &random1;
    targets[i]->effects[7] = &doubleFlash1;
    targets[i]->effect = &static1;
  }
}

// オープニングアニメーション
// R,G,B 各色を表示後、リングを３回明滅させる
void opening() {
  HSVColor color1;
  HSVColor color2;
  for (int i = 0; i < 3; i++) {
    color1 = HSVColor(i * 120, 100, 100);
    color2 = HSVColor((i + 1) * 120, 100, 100);
    for (int j = 0; j < 16; j++) {
      for (int k = 0; k < 16; k++) {
        if (j < k) {
          aleds[0]->loadLedData(k + 8, color1);
          aleds[0]->loadLedData(k / 2, color1);
        } else {
          aleds[0]->loadLedData(k + 8, color2);
          aleds[0]->loadLedData(k / 2, color2);
        }
      }
      aleds[0]->sendLedData();
      delay(50);
    }
  }

  // リング３回明滅
  // ファン側は消灯する
  targets[0]->color = HSVColor(0, 0, 0);
  targets[0]->draw();
  // １回目
  targets[1]->color = HSVColor(240, 100, 100);
  targets[1]->draw();
  delay(500);
  targets[1]->color = HSVColor(0, 0, 0);
  targets[1]->draw();
  delay(500);
  // ２回目
  targets[1]->color = HSVColor(120, 100, 100);
  targets[1]->draw();
  delay(500);
  targets[1]->color = HSVColor(0, 0, 0);
  targets[1]->draw();
  delay(500);
  // ３回目
  targets[1]->color = HSVColor(0, 100, 100);
  targets[1]->draw();
  delay(1000);
  // ファン側を点灯
  targets[0]->color = HSVColor(0, 100, 100);
  targets[0]->draw();
}

void executeCommond(String cmd) {
  if (cmd.startsWith("CEF")) {
    // エフェクト変更
    // CEF 対象No エフェクトNo
    // ex: CEF 1 2
    changeEffect(cmd);
  } else if (cmd.startsWith("CCL")) {
    // 色変更
    // CCL 対象No メイン／サブ(M or S)HSV値（3桁*3）
    // ex: CCL 1 M 0 100 50
    changeColor(cmd);
  } else if (cmd.startsWith("CSP")) {
    // 速度変更
    // CSP 対象No 速度
    // 現在表示されているエフェクトの速度が変更される
    // ex: CSP 1 200
    changeSpeed(cmd);
  } else if (cmd.startsWith("RST")) {
    EEPROM_reset();
  } else if (cmd.startsWith("STG")) {
    EEPROM_setup_global(cmd);
  } else if (cmd.startsWith("STA")) {
    EEPROM_setup_aled(cmd);
  } else if (cmd.startsWith("STT")) {
    EEPROM_setup_target(cmd);
  } else if (cmd.startsWith("EEP")) {
    EEPROM_query();
  } else if (cmd.startsWith("HLP")) {
    Serial.println("CEF target_no effect_no");
    Serial.println("CCL target_no H S V");
    Serial.println("CSP target_no speed");
    Serial.println("-----------------------");
    Serial.println("effect 0:static 1:rainbow 2:aurora 3:loop 4:heartbeat 5:blink 6:random 7:double-flash");
  } else {
    // 対象外
  }
}

void changeEffect(String cmd) {
  char *c;
  char buf[32];
  cmd.toCharArray(buf, 32);
  // 1つめはコマンド CEF
  c = strtok(buf, " ");
  int index = 0;
  char target_ = 'X';
  int targetIndex = -1;
  int effectIndex = -1;
  while (c != NULL) {
    c = strtok(NULL, " ");
    if (c != NULL) {
      if (index == 0) {
        target_ = *c;
        targetIndex = atoi(c);
      } else if (index == 1) {
        effectIndex = atoi(c);
        break;
      }
      index++;
    }
  }

  if (effectIndex >= 0 && targetIndex < target_num) {
    int firstIndex = target_ == 'A' ? 0 : targetIndex;
    int lastIndex = target_ == 'A' ? target_num - 1 : targetIndex;
    for (int i = firstIndex; i <= lastIndex; i++) {
      targets[i]->changeEffect(effectIndex);
      // [16 + Target_index * 8 + 0,1,2,3,4,5,6,7]
      index = 16 + i * 8 + 6;
      EEPROM.write(index++, effectIndex);
      EEPROM.write(index, targets[i]->waitTime);
    }

    Serial.println("Change Effect.");
  }
}

void changeColor(String cmd) {
  char *c;
  char buf[32];
  cmd.toCharArray(buf, 32);
  c = strtok(buf, " ");
  int index = 0;
  char target_ = 'X';
  int targetIndex = -1;
  float h = -1;
  float s = -1;
  float v = -1;
  // CCL 対象No HSV値（3桁*3）
  while (c != NULL) {
    c = strtok(NULL, " ");
    if (c != NULL) {
      if (index == 0) {
        target_ = *c;
        targetIndex = atoi(c);
      } else if (index == 1) {
        h = atof(c);
      } else if (index == 2) {
        s = atof(c);
      } else if (index == 3) {
        v = atof(c);
        break;
      }
      index++;
    }
  }

  if (v >= 0 && targetIndex < target_num && h <= 360 && s <= 100 && v <= 100) {
    int firstIndex = target_ == 'A' ? 0 : targetIndex;
    int lastIndex = target_ == 'A' ? target_num - 1 : targetIndex;
    for (int i = firstIndex; i <= lastIndex; i++) {
      targets[i]->color = HSVColor(h, s, v);
      // [16 + Target_index * 8 + 0,1,2,3,4,5,6,7]
      index = 16 + i * 8 + 3;
      // 255 までしか保存できないので、0.5倍にして保存する
      EEPROM.write(index++, (byte)(h * 0.5));
      EEPROM.write(index++, (byte)(s * 0.5));
      EEPROM.write(index++, (byte)(v * 0.5));
    }
    Serial.println("Change Color.");
  }
}

void changeSpeed(String cmd) {
  char *c;
  char buf[32];
  cmd.toCharArray(buf, 32);
  // 1つめはコマンド CSP
  c = strtok(buf, " ");
  int index = 0;
  char target_ = 'X';

  int targetIndex = -1;
  int _speed = -1;
  // CSP 対象No 速度
  while (c != NULL) {
    c = strtok(NULL, " ");
    if (c != NULL) {
      if (index == 0) {
        target_ = *c;
        targetIndex = atoi(c);
      } else if (index == 1) {
        _speed = atoi(c);
        break;
      }
      index++;
    }
  }

  if (_speed >= 0 && _speed <= 255 && targetIndex < target_num) {
    int firstIndex = target_ == 'A' ? 0 : targetIndex;
    int lastIndex = target_ == 'A' ? target_num - 1 : targetIndex;
    for (int i = firstIndex; i <= lastIndex; i++) {
      targets[i]->waitTime = _speed;
      // [16 + Target_index * 8 + 0,1,2,3,4,5,6,7]
      index = 16 + i * 8 + 7;
      EEPROM.write(index, _speed);
    }
    Serial.println("Change Speed.");
  }
}

void EEPROM_reset() {
  for (int i = 0; i < 512; i++) {
    EEPROM.write(i, 0x00);
  }
  Serial.println("EEPROM reset.");
}

// STG [ALED num] [TArget num]   ex: STG 24 2
void EEPROM_setup_global(String cmd) {
  char *c;
  char buf[32];
  cmd.toCharArray(buf, 32);
  c = strtok(buf, " ");
  int index = 0;
  int aledNum = -1;
  int targetNum = -1;
  while (c != NULL) {
    c = strtok(NULL, " ");
    if (c != NULL) {
      if (index == 0) {
        aledNum = atoi(c);
      } else if (index == 1) {
        targetNum = atoi(c);
        break;
      }
      index++;
    }
  }

  if (targetNum >= 0) {
    EEPROM.write(0, aledNum);
    EEPROM.write(1, targetNum);
    Serial.println("EEPROM_setup_global.");
  }
}
// STA [aled index:0-4] [ledCount]  ex: STA 0 24
void EEPROM_setup_aled(String cmd) {
  char *c;
  char buf[32];
  cmd.toCharArray(buf, 32);
  c = strtok(buf, " ");
  int index = 0;
  int aledIndex = -1;
  int ledCount = -1;
  while (c != NULL) {
    c = strtok(NULL, " ");
    if (c != NULL) {
      if (index == 0) {
        aledIndex = atoi(c);
      } else if (index == 1) {
        ledCount = atoi(c);
        break;
      }
      index++;
    }
  }

  if (ledCount >= 0) {
    //  [8 + aled_index]
    index = 8 + aledIndex;
    EEPROM.write(index, ledCount);
    Serial.println("EEPROM_setup_aled.");
  }
}
// STT [Target index:0-7] [start index] [last index] [aled index] [color hue] [color sat] [color val]
//   ex: STT 1 8 23 0 0 100 100
void EEPROM_setup_target(String cmd) {
  char *c;
  char buf[32];
  cmd.toCharArray(buf, 32);
  c = strtok(buf, " ");
  int index = 0;
  int targetIndex = -1;
  int startIndex = -1;
  int lastIndex = -1;
  int aledIndex = -1;
  int colorHue = -1;
  int colorSat = -1;
  int colorVal = -1;
  while (c != NULL) {
    c = strtok(NULL, " ");
    if (c != NULL) {
      if (index == 0) {
        targetIndex = atoi(c);
      } else if (index == 1) {
        startIndex = atoi(c);
      } else if (index == 2) {
        lastIndex = atoi(c);
      } else if (index == 3) {
        aledIndex = atoi(c);
      } else if (index == 4) {
        colorHue = atoi(c);
      } else if (index == 5) {
        colorSat = atoi(c);
      } else if (index == 6) {
        colorVal = atoi(c);
        break;
      }
      index++;
    }
  }

  if (colorVal >= 0) {
    // [16 + Target_index * 8 + 0,1,2,3,4,5,6,7]
    index = 16 + targetIndex * 8;
    EEPROM.write(index++, startIndex);
    EEPROM.write(index++, lastIndex);
    EEPROM.write(index++, aledIndex);
    EEPROM.write(index++, colorHue);
    EEPROM.write(index++, colorSat);
    EEPROM.write(index++, colorVal);
    EEPROM.write(index++, 0x00);
    EEPROM.write(index++, 0x00);
    Serial.println("EEPROM_setup_target.");
  }
}

void EEPROM_query() {
  byte eVal;
  for (int i = 0; i < 80; i++) {
    eVal = EEPROM.read(i);
    Serial.print(i);
    Serial.print("\t");
    Serial.print(eVal);
    Serial.println();
  }
}
