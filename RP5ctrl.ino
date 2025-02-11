/*
  RP5ctrl : AV controller for Raspberry Pi 5 power switch and related devices
    by pado3@mstdn.jp
  r1.1 2025/02/11 clarify through command (0x0), RPi PW from SW will W action(single click)
  r1.0 2025/02/06 initial release

  pin assignment:
    D3 : IR LED w/buffer Tr + 47ohm
    D4 : indicator green LED w/10k
    D5 : RPi5 power switch, pullup
    D6 : REGZA TV power switch, pullup
    D7 : REGZA TV input selector, pullup
    D8 : Panasonic soundbar input selector, pullup
    D9 : RPi5 control output w/Tr switch
    D10: SPI /CS, pull down w/100k (can't deactivate from master in exchange to control wire)
    D11: SPI MOSI
    D12: SPI MISO (NC)
    D13: SPI SCK
*/

#include <Arduino.h>
#include <SPI.h>
#include "PinDefinitionsAndMore.h"  // IRremoteのピン定義等。同一フォルダに置く
#include <IRremote.hpp>

#define APP_VERSION "1.1 (2025/02/11 10:38)"  // アプリバージョン
#define DELAY_AFTER_SW 300                    // チャタリング予防 in msec
#define DELAY_AFTER_LOOP 100                  // CPUウェイト in msec

bool RPI_TR_STATE = false;  // SPIを使うとdelay()が効かなくなるのでRPi PWはトグルにする

const int LED = 4;  // 出力時に点けるインジケータLED
const int IN5 = 5;  // RPi5をON/OFFするスイッチ
const int IN6 = 6;  // REGZA TVをON/OFFするスイッチ
const int IN7 = 7;  // REGZA TVの入力切換スイッチ
const int IN8 = 8;  // Panasonicサウンドバーの入力切換スイッチ
const int RPW = 9;  // RPi5の外部入力コントロール

// 機種とコマンドのリスト
// 0:RPi PWR, 1:TV PWR, 2:TV Selector, 3:SP Selector
const char* Machine[] = { "RPi", "REGZA", "REGZA", "SP" };  // SWはD5, 6, 7, 8
const uint16_t Address[] = { 0xFF, 0x40, 0x40, 0xA };       // IRアドレス、RPIはダミー
const uint8_t Command[] = { 0xFF, 0x12, 0xF, 0x86 };        // ダミー、ON/OFF, 入力切換, 入力切換
const uint8_t Repeats[] = { 0x0, 0x1, 0x1, 0x1 };           // ダミー、IRはリピート2回

void setup() {
  // インジケータLEDとRPi5コントロールは出力
  pinMode(LED, OUTPUT);
  pinMode(RPW, OUTPUT);
  // スイッチを付ける端子は全てプルアップ
  pinMode(IN5, INPUT_PULLUP);
  pinMode(IN6, INPUT_PULLUP);
  pinMode(IN7, INPUT_PULLUP);
  pinMode(IN8, INPUT_PULLUP);

  // 起動メッセージ
  Serial.begin(115200);  // SPIに合わせる必要はない
  delay(200);
  Serial.println();
  Serial.println(F("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-"));
  Serial.println(F("START " __FILE__ " from " __DATE__ "\r"));
  Serial.println(F("APP version " APP_VERSION " w/IRremote " VERSION_IRREMOTE));
  IrSender.begin();  // Start with IR_SEND_PIN in PinDefinitionsAndMore.h
  Serial.println(F("IR sender pin is " STR(IR_SEND_PIN)));
  Serial.println(F("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-"));

  // SPIをスレーブモードで初期設定し割り込み開始
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE1);
  SPCR |= _BV(SPE);
  pinMode(SS, INPUT);
  SPI.attachInterrupt();

  // RPW用Tr, B端子の初期値はfalse(= LOW = Hi-Z = OFF)
  digitalWrite(RPW, RPI_TR_STATE);
}

void loop() {
  byte sw = 0;

  // 同時押しされていても最後のキーだけを有効にする
  if (!digitalRead(IN5)) sw = 1;
  if (!digitalRead(IN6)) sw = 2;
  if (!digitalRead(IN7)) sw = 3;
  if (!digitalRead(IN8)) sw = 4;

  if (0 < sw && sw < 5) {
    // Serial.println();
    Serial.print(F("get SW data="));
    Serial.print(sw);
    digitalWrite(LED, HIGH);
    // スイッチ入力からの外部制御
    ext_ctrl(sw);
    delay(DELAY_AFTER_SW);  // チャタリング予防（SPI非アクティブであればdelayが効く）
    if (sw == 1) {          // loop内はdelayが効くのでRPi電源SWもシングルクリックでいい
      Serial.print(F("put SW data="));
      Serial.print(sw);
      ext_ctrl(sw);         // toggle
      delay(DELAY_AFTER_SW);
    }
  } else {
    digitalWrite(LED, LOW);  // SPI割り込みが入ってもループに帰ってくるのでLED消灯できる
  }
  delay(DELAY_AFTER_LOOP);  // CPUウェイト
}

// SPI割り込み
ISR(SPI_STC_vect) {
  byte cc = SPDR;  // SPDRレジスタにデータが送られてくる
  if (0 < cc) {    // if 0x0, through the data 1.1
    // Serial.println();
    Serial.print(F("receive SPI data="));
    Serial.print(cc);
    digitalWrite(LED, HIGH);  // LEDを消す方はloop()に任せる
    // SPIからの外部制御
    ext_ctrl(cc);
  }
}

void ext_ctrl(byte CMD) {
  // 機種・アドレス・コマンドの変数定義
  char* sMachine;
  uint16_t sAddress;
  uint8_t sCommand;
  uint8_t sRepeats;
  // 制御内容の決定
  if (CMD <= 4) {
    sMachine = Machine[CMD - 1];
    sAddress = Address[CMD - 1];
    sCommand = Command[CMD - 1];
    sRepeats = Repeats[CMD - 1];
  } else {  // REGZA制御
    sMachine = "REGZA";
    sAddress = 0x40;
    sCommand = CMD;
    sRepeats = 0x1;
  }

  // 実動作
  if (sMachine == "RPi") {
    Serial.print(F(", "));
    Serial.print(sMachine);
    Serial.print(F(" Tr state: "));
    RPI_TR_STATE = !RPI_TR_STATE;  // delay()によるON/OFFがSPIで入れると通らないのでトグル
    Serial.println(RPI_TR_STATE);
    Serial.flush();
    digitalWrite(RPW, RPI_TR_STATE);
  } else if (sMachine == "REGZA") {
    Serial.print(F(", IR send to "));
    Serial.print(sMachine);
    Serial.print(F(", Code: 0x"));
    Serial.println(sCommand, HEX);
    Serial.flush();
    IrSender.sendNEC(sAddress & 0xFF, sCommand, sRepeats);
  } else if (sMachine == "SP") {
    Serial.print(F(", IR send to "));
    Serial.print(sMachine);
    Serial.print(F(", Code: 0x"));
    Serial.println(sCommand, HEX);
    Serial.flush();
    IrSender.sendPanasonic(sAddress & 0xFFF, sCommand, sRepeats);
  }
}
