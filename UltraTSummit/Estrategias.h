#ifndef Estrategias_H
#define Estrategias_H

#include "PID.h"

#define VEL_SEEK 600
#define VEL_CONTORNO_MIN 600
#define VEL_CONTORNO_MAX 800

void paraTras() { // estratégia número 6 no controle
  // Usa timers não-bloqueantes em vez de delay()
  // Move para frente por 500ms, depois para trás por 350ms, depois executa iSeeYou
  motor.move_for_then(1023, 1023, 500,
                      -1023, 1023, 350);
  iSeeYou();
}

void resetSeekAndDestroy() {
  //noop
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
