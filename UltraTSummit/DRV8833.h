/* ====================================================
 *  DRV8833 Library
 *    Author: luisf18 (github)
 *    Ver.: 1.0.1 (corrigido)
 *    last_update: 04/12/2022
 *    fix: writeSD() acessava Speed[] fora dos limites
 *         antes de validar o índice do motor
 * ====================================================
 */

#ifndef DRV8833_H
#define DRV8833_H

#include <Arduino.h>

// ─────────────────────────────────────────────────────────────
//  Struct auxiliar para encadear movimentos com timer
// ─────────────────────────────────────────────────────────────
typedef struct motor_move_t {
  int VL = 0, VR = 0, dt = 0;
  void set(int vl, int vr, int duration) {
    VL = vl;
    VR = vr;
    dt = duration;
  }
} motor_move_t;

// ─────────────────────────────────────────────────────────────
class DRV8833 {

public:

  // ── Construtor ─────────────────────────────────────────────
  DRV8833(uint8_t _in1, uint8_t _in2, uint8_t _in3, uint8_t _in4) {
    p[0] = _in1;
    p[1] = _in2;
    p[2] = _in3;
    p[3] = _in4;
  }

  // ===========================================================
  //  SETUP
  // ===========================================================

  boolean begin() { return begin(PWM_HZ, PWM_RES); }

  boolean begin(uint32_t f_Hz, uint8_t res) {
    pinMode(p[0], OUTPUT);
    pinMode(p[1], OUTPUT);
    pinMode(p[2], OUTPUT);
    pinMode(p[3], OUTPUT);

    Serial.println("[x] DRV8833 Begin");
    Serial.printf(" | - Pins: [ %d, %d, %d, %d ]\n", p[0], p[1], p[2], p[3]);
    Serial.printf(" | - PWM: [ Range: 0 to %d ] [ %d bits / %d Hz ]\n",
                  PWM_MAX, PWM_RES, PWM_HZ);

    init_pwm(f_Hz, res);
    stop();
    return true;
  }

  // ─────────────────────────────────────────────────────────
  uint8_t init_pwm(uint32_t HZ, uint8_t RES) {
    #ifdef ESP32
      if ((RES > 0) && (RES <= 12)) PWM_RES = RES;
      PWM_MAX = (1 << PWM_RES) - 1;
      // ESP32 arduino-esp32 v3+: ledcAttach vincula pino ao canal
      // DIAGNÓSTICO: cada ledcAttach() retorna true/false. Se algum canal
      // LEDC estiver esgotado (o chip tem um número limitado de canais
      // PWM compartilhados com todo o resto do programa), o attach falha
      // e aparece "FALHOU" abaixo no serial monitor — é assim que
      // confirmamos se o motor direito está sem canal PWM válido.
      bool ok0 = ledcAttach(p[0], HZ, PWM_RES);
      PWM_HZ = ok0 ? HZ : PWM_HZ;
      bool ok1 = ledcAttach(p[1], PWM_HZ, PWM_RES);
      bool ok2 = ledcAttach(p[2], PWM_HZ, PWM_RES);
      bool ok3 = ledcAttach(p[3], PWM_HZ, PWM_RES);

      Serial.printf(" | - ledcAttach: [pino %d: %s] [pino %d: %s] [pino %d: %s] [pino %d: %s]\n",
                    p[0], ok0 ? "OK" : "FALHOU",
                    p[1], ok1 ? "OK" : "FALHOU",
                    p[2], ok2 ? "OK" : "FALHOU",
                    p[3], ok3 ? "OK" : "FALHOU");
    #else
      if (RES >= 4 && RES <= 16) PWM_RES = RES;
      analogWriteResolution(PWM_RES);
      PWM_MAX = (1 << PWM_RES) - 1;
      if (HZ >= 100 && HZ <= 60000) PWM_HZ = HZ;
      analogWriteFreq(PWM_HZ);
    #endif

    Serial.printf("PWM: [ Range: 0 to %d ] [ %d bits / %d Hz ]\n",
                  PWM_MAX, PWM_RES, PWM_HZ);

    return (true | (PWM_HZ == HZ) << 1 | (PWM_RES == RES) << 2);
  }

  int     pin(uint8_t n) { return (n > 3 ? 0 : p[n]); }
  int     range()        { return PWM_MAX; }
  int     read(uint8_t n){ return (n > 1 ? 0 : Speed[n]); }

  // ===========================================================
  //  ALTO NÍVEL — ACIONAMENTO DIFERENCIAL
  // ===========================================================

  // Controle diferencial clássico
  // linear/angular em unidades PWM, ou em µs de rádio (radio_range=true)
  void diff_drive(int linear, int angular, boolean invert, boolean radio_range) {
    if (radio_range) {
      linear  = map(linear,  1000, 2000, -PWM_MAX, PWM_MAX);
      angular = map(angular, 1000, 2000, -PWM_MAX, PWM_MAX);
    }
    int vel_l = constrain(linear + angular, -PWM_MAX, PWM_MAX);
    int vel_r = constrain(linear - angular, -PWM_MAX, PWM_MAX);
    if (invert) move(-vel_r, -vel_l);
    else        move( vel_l,  vel_r);
  }

  // ===========================================================
  //  MOVIMENTOS COM TIMER (não bloqueantes)
  //  ATENÇÃO: chame update() no loop() para que funcionem!
  // ===========================================================

  void clear_moving() { Moving = false; Moving_list = 0; }

  // Move indefinidamente
  void move(motor_move_t m) {
    move(m.VL, m.VR);
    Moving_timeout = millis() + m.dt;
    Moving = true;
  }

  // Executa uma lista de movimentos em sequência
  void move(motor_move_t *m, int len) {
    move(*m);
    Move_list   = m + 1;
    Moving_list = len - 1;
  }

  // Move por 'duration' ms e para
  void move_for(int speed_0, int speed_1, uint32_t duration) {
    Moving         = true;
    Moving_timeout = millis() + duration;
    move(speed_0, speed_1);
    Moving_list = 0;
  }

  // Move por 'duration' ms e depois executa um segundo movimento
  void move_for_then(int speed_0, int speed_1, uint32_t duration,
                     int speed_next_0, int speed_next_1, uint32_t duration_next) {
    Moving         = true;
    Moving_timeout = millis() + duration;
    move(speed_0, speed_1);
    Moving_list    = 1;
    Move_next.VL   = speed_next_0;
    Move_next.VR   = speed_next_1;
    Move_next.dt   = duration_next;
    Move_list      = &Move_next;
  }

  // Deve ser chamado no loop() para processar os timers
  int update() {
    if (!Moving) return -1;

    if (millis() >= Moving_timeout) {
      Moving = false;
      if (Moving_list) {
        Moving_list--;
        Moving          = true;
        Moving_timeout += Move_list->dt;
        if (abs(Move_list->VL) <= PWM_MAX) write(Move_list->VL, 0); else stop(0);
        if (abs(Move_list->VR) <= PWM_MAX) write(Move_list->VR, 1); else stop(1);
        Move_list++;
      }
    }

    return (Moving + Moving_list);
  }

  // ===========================================================
  //  MOVIMENTOS DIRETOS
  // ===========================================================

  void move   (int s0, int s1) { write   (s0, 0); write   (s1, 1); }
  void moveSD (int s0, int s1) { writeSD (s0, 0); writeSD (s1, 1); }
  void moveRaw(int s0, int s1) { writeRaw(s0, 0); writeRaw(s1, 1); }

  // Freio ativo (curto-circuita os terminais — para mais rápido)
  void stop() {
    #ifdef ESP32
      ledcWrite(p[0], PWM_MAX); ledcWrite(p[1], PWM_MAX);
      ledcWrite(p[2], PWM_MAX); ledcWrite(p[3], PWM_MAX);
    #else
      analogWrite(p[0], PWM_MAX); analogWrite(p[1], PWM_MAX);
      analogWrite(p[2], PWM_MAX); analogWrite(p[3], PWM_MAX);
    #endif
    Speed[0] = 0; Speed[1] = 0;
  }

  void stop(uint8_t motor) {
    if (motor > 1) return;
    #ifdef ESP32
      ledcWrite(p[0 + 2*motor], PWM_MAX);
      ledcWrite(p[1 + 2*motor], PWM_MAX);
    #else
      analogWrite(p[0 + 2*motor], PWM_MAX);
      analogWrite(p[1 + 2*motor], PWM_MAX);
    #endif
    Speed[motor] = 0;
  }

  // Corte de energia (motor desacelera por inércia)
  void off() {
    #ifdef ESP32
      ledcWrite(p[0], 0); ledcWrite(p[1], 0);
      ledcWrite(p[2], 0); ledcWrite(p[3], 0);
    #else
      analogWrite(p[0], 0); analogWrite(p[1], 0);
      analogWrite(p[2], 0); analogWrite(p[3], 0);
    #endif
    Speed[0] = 0; Speed[1] = 0;
  }

  void off(uint8_t motor) {
    if (motor > 1) return;
    #ifdef ESP32
      ledcWrite(p[0 + 2*motor], 0);
      ledcWrite(p[1 + 2*motor], 0);
    #else
      analogWrite(p[0 + 2*motor], 0);
      analogWrite(p[1 + 2*motor], 0);
    #endif
    Speed[motor] = 0;
  }

  // ===========================================================
  //  BAIXO NÍVEL — ESCRITA PWM
  // ===========================================================

  // Escrita com sinal de velocidade salvo em Speed[]
  void write(int speed, int motor) {
    if (motor > 1) return;
    Speed[motor] = speed;
    #ifdef ESP32
      if (speed >= 0) { ledcWrite(p[0+2*motor],  speed); ledcWrite(p[1+2*motor], 0); }
      else            { ledcWrite(p[1+2*motor], -speed); ledcWrite(p[0+2*motor], 0); }
    #else
      int m = 2 * motor;
      if (speed >= 0) { analogWrite(p[0+m],  speed); analogWrite(p[1+m], 0); }
      else            { analogWrite(p[1+m], -speed); analogWrite(p[0+m], 0); }
    #endif
  }

  // Escrita raw (sem salvar em Speed[])
  void writeRaw(int speed, int motor) {
    if (motor > 1) return;
    #ifdef ESP32
      if (speed >= 0) { ledcWrite(p[0+2*motor],  speed); ledcWrite(p[1+2*motor], 0); }
      else            { ledcWrite(p[1+2*motor], -speed); ledcWrite(p[0+2*motor], 0); }
    #else
      int m = 2 * motor;
      if (speed >= 0) { analogWrite(p[0+m],  speed); analogWrite(p[1+m], 0); }
      else            { analogWrite(p[1+m], -speed); analogWrite(p[0+m], 0); }
    #endif
  }

  // Modo slow-decay (frenagem mais suave)
  // CORREÇÃO: guarda movida para ANTES de acessar Speed[]
  void writeSD(int speed, int motor) {
    if (motor > 1) return;          // ← CORREÇÃO: validação antes de Speed[]
    Speed[motor] = speed;
    #ifdef ESP32
      if (speed < 0) { ledcWrite(p[0+2*motor], PWM_MAX+speed); ledcWrite(p[1+2*motor], PWM_MAX); }
      else           { ledcWrite(p[1+2*motor], PWM_MAX-speed); ledcWrite(p[0+2*motor], PWM_MAX); }
    #else
      int m = 2 * motor;
      if (speed < 0) { analogWrite(p[0+m], PWM_MAX+speed); analogWrite(p[1+m], PWM_MAX); }
      else           { analogWrite(p[1+m], PWM_MAX-speed); analogWrite(p[0+m], PWM_MAX); }
    #endif
  }

  // ===========================================================
  //  SOM — uso criativo dos motores como buzzer
  // ===========================================================

  void     sound_vol(uint8_t vol) { if (vol <= 25) SOUND_VOL = vol; }
  uint16_t sound_duty()           { return SOUND_VOL * (PWM_MAX / 100); }

  // Emite n bipes em ambos os motores
  void bip(uint8_t n, uint16_t dt, uint32_t tone) {
    sound_tone(tone);
    uint16_t duty = sound_duty();
    for (int i = 0; i < n; i++) {
      moveRaw(duty, duty); delay(dt);
      moveRaw(0,    0);    delay(dt);
    }
    sound_stop();
  }

  // Emite n bipes em um motor específico
  void bip(uint8_t n, uint16_t dt, uint32_t tone, uint8_t motor) {
    if (motor > 1) return;
    sound_tone(tone, motor);
    uint16_t duty = sound_duty();
    for (int i = 0; i < n; i++) {
      writeRaw(duty, motor); delay(dt);
      writeRaw(0,    motor); delay(dt);
    }
    sound_stop(motor);
  }

  void sound_tone(uint32_t tone) { sound_tone(tone, 0); sound_tone(tone, 1); }

  void sound_tone(uint32_t tone, uint8_t motor) {
    if (motor > 1) return;
    #ifdef ESP32
      ledcAttach(p[0 + 2*motor], tone, PWM_RES);
      ledcAttach(p[1 + 2*motor], tone, PWM_RES);
    #else
      analogWriteFreq(tone);
    #endif
    writeRaw(sound_duty(), motor);
  }

  void sound_stop()              { sound_stop(0); sound_stop(1); }

  void sound_stop(uint8_t motor) {
    if (motor > 1) return;
    #ifdef ESP32
      ledcAttach(p[0 + 2*motor], PWM_HZ, PWM_RES);
      ledcAttach(p[1 + 2*motor], PWM_HZ, PWM_RES);
    #else
      analogWriteFreq(PWM_HZ);
    #endif
    write(Speed[motor], motor);
  }

// ─────────────────────────────────────────────────────────────
private:

  int      p[4];
  uint32_t PWM_HZ  = 25000;
  uint8_t  PWM_RES = 10;
  uint16_t PWM_MAX = 1023;

  uint16_t SOUND_VOL = 15;

  boolean      Moving         = false;
  uint32_t     Moving_timeout = 0;
  int          Moving_list    = 0;
  motor_move_t Move_next;
  motor_move_t *Move_list;

  int Speed[2] = {0, 0};
};

#endif // DRV8833_H
