#ifndef Estrategias_H
#define Estrategias_H

#include "PID.h"

#define VEL_SEEK 600
#define VEL_CONTORNO_MIN 600
#define VEL_CONTORNO_MAX 800

void paraTras() {
  // Usa timers não-bloqueantes em vez de delay()
  motor.move_for_then(1023, 1023, 500,
                     -1023, 1023, 350);
  iSeeYou();
}

// ============================================================
//  SeekAndDestroy — PID de aproximação com desvio de linha
// ============================================================
//  Cada variante lê um subconjunto de 4 sensores (3 "de oponente" +
//  1 "de linha", do próprio lado):
//    SeekAndDestroy_L (estratégia 4): frontal-esq, frontal-dir, lateral-DIR, linha-ESQ
//    SeekAndDestroy_R (estratégia 5): lateral-ESQ, frontal-esq, frontal-dir, linha-DIR
//
//  O sensor de linha entra como mais um peso na média do erro (igual
//  aos de oponente), não como um "if" separado — o desvio sai suave,
//  proporcional, dentro do próprio PID, igual aos outros sensores.
//
//  IMPORTANTE: esse PID usa um ganho PRÓPRIO (KP_SEEK), bem menor que
//  o Kp do iSeeYou (450). Lá o erro vira o diferencial inteiro das
//  rodas; aqui ele é SOMADO a vel_base — usar o mesmo Kp=450 fazia até
//  um único sensor fraco (peso 2) virar uma correção de 900, maior que
//  a própria vel_base (550), e o robô parecia começar girando em vez
//  de curvar suave. CALIBRE KP_SEEK e PESO_LINHA abaixo.
// ============================================================

#define KP_SEEK     100.0  // ganho deste PID — CALIBRE AQUI (bem menor que o Kp=450 do iSeeYou, de propósito)
#define PESO_LINHA    6.0  // peso do sensor de linha no erro — CALIBRE AQUI (quanto maior, mais forte o desvio da borda)

float calculoErroSeekL() { // pesos de calculoErroSensor() + peso do sensor de linha
  float soma = 0; int ativos = 0;
  if (leitura[1]) { soma += -2;          ativos++; } // frontal esquerda
  if (leitura[2]) { soma +=  2;          ativos++; } // frontal direita
  if (leitura[3]) { soma +=  4;          ativos++; } // lateral direita
  if (leitura[4]) { soma +=  PESO_LINHA; ativos++; } // linha esquerda -> empurra pra DIREITA (lado oposto)
  return (ativos > 0) ? (soma / ativos) : 0;
}

float calculoErroSeekR() { // pesos de calculoErroSensor() + peso do sensor de linha
  float soma = 0; int ativos = 0;
  if (leitura[0]) { soma += -4;          ativos++; } // lateral esquerda
  if (leitura[1]) { soma += -2;          ativos++; } // frontal esquerda
  if (leitura[2]) { soma +=  2;          ativos++; } // frontal direita
  if (leitura[5]) { soma += -PESO_LINHA; ativos++; } // linha direita -> empurra pra ESQUERDA (lado oposto)
  return (ativos > 0) ? (soma / ativos) : 0;
}

bool _SND_L_travado = false; // uma vez true, SeekAndDestroy_L só chama iSeeYou() até o fim do round
bool _SND_R_travado = false; // idem pro SeekAndDestroy_R

// Chame no início de cada round/combate pra destravar as duas estratégias de novo
void resetSeekAndDestroy() {
  _SND_L_travado = false;
  _SND_R_travado = false;
}

void SeekAndDestroy_L(){
  leituraSensoresSDLeft();

  if (!_SND_L_travado && leitura[1] && leitura[2]) {
    _SND_L_travado = true; // trava: só destrava de novo no próximo round (resetSeekAndDestroy)
  }

  if (_SND_L_travado) {
    iSeeYou();
    return;
  }

  float pid_local = KP_SEEK * calculoErroSeekL();
  int velocidade_esq = constrain((int)(vel_base + pid_local), -1023, 1023);
  int velocidade_dir = constrain((int)(vel_base - pid_local), -1023, 1023);
  motor.move(velocidade_esq, velocidade_dir); // nada detectado -> pid_local=0 -> anda reto em vel_base
}

void SeekAndDestroy_R(){
  leituraSensoresSDRight();

  if (!_SND_R_travado && leitura[1] && leitura[2]) {
    _SND_R_travado = true;
  }

  if (_SND_R_travado) {
    iSeeYou();
    return;
  }

  float pid_local = KP_SEEK * calculoErroSeekR();
  int velocidade_esq = constrain((int)(vel_base + pid_local), -1023, 1023);
  int velocidade_dir = constrain((int)(vel_base - pid_local), -1023, 1023);
  motor.move(velocidade_esq, velocidade_dir);
}

void __estadosBobo() {
  enum ESTADO_BOBO {
    BOBO_SEM_INIMIGO,
    BOBO_INIMIGO_FRENTE,
    BOBO_INIMIGO_ESQ,
    BOBO_INIMIGO_FRENTE_ESQ,
    BOBO_INIMIGO_DIR,
    BOBO_INIMIGO_FRENTE_DIR,
  } estadoAtual = BOBO_SEM_INIMIGO; // sem inimigo

  if        (leitura[1] && leitura[2]) { //enxergando com os sensores frontais
    estadoAtual = BOBO_INIMIGO_FRENTE;
  } else if (leitura[1]) { // enxergando com o esquerdo frente
    estadoAtual = BOBO_INIMIGO_FRENTE_ESQ;
  } else if (leitura[2]) { // enxergando com o direito fente
    estadoAtual = BOBO_INIMIGO_FRENTE_DIR;
  } else if (leitura[0]) { // enxergando com o esquerdo
    estadoAtual = BOBO_INIMIGO_ESQ;
  } else if (leitura[3]) { // enxergando com o direito
    estadoAtual = BOBO_INIMIGO_DIR;
  } else {
    estadoAtual = BOBO_SEM_INIMIGO; // sem inimigo
  } //! faltam algumas combinações aqui

  switch (estadoAtual) {
    case BOBO_INIMIGO_ESQ:
      Serial.println("Left Detected!");
      motor.move(-VEL_SEEK, VEL_SEEK);
      break;

    case BOBO_INIMIGO_FRENTE_ESQ:
      Serial.println("Left Soft Detected!");
      motor.move(VEL_SEEK/2, VEL_SEEK);
      break;

    case BOBO_INIMIGO_FRENTE:
      Serial.println("ROBOT ATTACK!");
      motor.move(1023, 1023);
      break;

    case BOBO_INIMIGO_FRENTE_DIR:
      Serial.println("Left Soft Detected!");
      motor.move(VEL_SEEK, VEL_SEEK/2);
      break;

    case BOBO_SEM_INIMIGO:
    case BOBO_INIMIGO_DIR:
      Serial.println("Right Detected!");
      motor.move(VEL_SEEK, -VEL_SEEK);
      break;
  }
}

void PID_Contorno() {
  leituraSensores(); pid();
  if (!alvoDetectado) return;

  uint16_t velocidade_esq = constrain(+PID, -1023, 1023);
  uint16_t velocidade_dir = constrain(-PID, -1023, 1023);

  if (PID == 0) { //! epsilon
    motor.move(1023, 1023); // alvo detectado e perfeitamente centralizado -> avança em linha reta
  } else {
    motor.move(velocidade_esq, velocidade_dir);
  }
}

void ContornarLPID() {
  leituraSensoresConservadora();

  enum ESTADO_BOBO {
    BOBO_SEM_INIMIGO,
    BOBO_INIMIGO,
  } estadoAtual = BOBO_SEM_INIMIGO; // sem inimigo

  if (leitura[0] || leitura[1] || leitura[2] || leitura[3]) { // enxergando com qualquer sensor
    estadoAtual = BOBO_INIMIGO;
  } else {
    estadoAtual = BOBO_SEM_INIMIGO; // sem inimigo
  }

  switch (estadoAtual) {
    case BOBO_INIMIGO: PID_Contorno(); break;

    case BOBO_SEM_INIMIGO: {
      Serial.println("Contornando");
      if      (leitura[5]) motor.move(-VEL_CONTORNO_MAX, VEL_CONTORNO_MAX);
      else if (leitura[4]) motor.move( VEL_CONTORNO_MAX,-VEL_CONTORNO_MAX);
      else                 motor.move( VEL_CONTORNO_MAX, VEL_CONTORNO_MIN);
    } break;
  }
}

void ContornarL() {
  leituraSensoresConservadora();

  enum ESTADO_BOBO {
    BOBO_SEM_INIMIGO,
    BOBO_INIMIGO,
  } estadoAtual = BOBO_SEM_INIMIGO; // sem inimigo

  if (leitura[0] || leitura[1] || leitura[2] || leitura[3]) { // enxergando com qualquer sensor
    estadoAtual = BOBO_INIMIGO;
  } else {
    estadoAtual = BOBO_SEM_INIMIGO; // sem inimigo
  }

  switch (estadoAtual) {
    case BOBO_INIMIGO: __estadosBobo(); break;

    case BOBO_SEM_INIMIGO: {
      Serial.println("Contornando");
      if      (leitura[5]) motor.move(-VEL_CONTORNO_MAX, VEL_CONTORNO_MAX);
      else if (leitura[4]) motor.move( VEL_CONTORNO_MAX,-VEL_CONTORNO_MAX);
      else                 motor.move( VEL_CONTORNO_MAX, VEL_CONTORNO_MIN);
    } break;
  }
}

void estadosBobo() {
  leituraSensores();
  __estadosBobo();
}

#endif
