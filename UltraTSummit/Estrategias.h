#ifndef Estrategias_H
#define Estrategias_H

#include "PID.h"

#define VEL_SEEK 600

void paraTras() { // estratégia número 6 no controle
  // Usa timers não-bloqueantes em vez de delay()
  // Move para frente por 500ms, depois para trás por 350ms, depois executa iSeeYou
  motor.move_for_then(1023, 1023, 500,
                      -1023, 1023, 350);
  iSeeYou();
}




#define KP_SEEK     100.0  // ganho deste PID — CALIBRE AQUI (bem menor que o Kp=450 do iSeeYou, de propósito)
#define PESO_LINHA    6.0  // peso do sensor de linha no erro — CALIBRE AQUI (quanto maior, mais forte o desvio da borda)

float calculoErroSeekL() { // pesos de calculoErroSensor() + peso do sensor de linha
  float soma = 0; int ativos = 0;
  if (leitura[1]) { soma += -2;        ativos++; } // frontal esquerda
  if (leitura[2]) { soma +=  2;        ativos++; } // frontal direita
  if (leitura[3]) { soma +=  4;        ativos++; } // lateral direita
  if (leitura[4]) { soma +=  PESO_LINHA; ativos++; } // linha esquerda -> empurra pra DIREITA (lado oposto)
  return (ativos > 0) ? (soma / ativos) : 0;
}

float calculoErroSeekR() { // pesos de calculoErroSensor() + peso do sensor de linha
  float soma = 0; int ativos = 0;
  if (leitura[0]) { soma += -4;         ativos++; } // lateral esquerda
  if (leitura[1]) { soma += -2;         ativos++; } // frontal esquerda
  if (leitura[2]) { soma +=  2;         ativos++; } // frontal direita
  if (leitura[5]) { soma += -PESO_LINHA; ativos++; } // linha direita -> empurra pra ESQUERDA (lado oposto)
  return (ativos > 0) ? (soma / ativos) : 0;
}

void SeekAndDestroy_L(){ // estratégia número 4 no controle
  leituraSensoresSDLeft();

  float pid_local = KP_SEEK * calculoErroSeekL();
  int velocidade_esq = constrain((int)(vel_base + pid_local), -1023, 1023);
  int velocidade_dir = constrain((int)(vel_base - pid_local), -1023, 1023);
  motor.move(velocidade_esq, velocidade_dir); // nada detectado -> pid_local=0 -> anda reto em vel_base
}

void SeekAndDestroy_R(){ // estratégia número 5 no controle
  leituraSensoresSDRight();

  float pid_local = KP_SEEK * calculoErroSeekR();
  int velocidade_esq = constrain((int)(vel_base + pid_local), -1023, 1023);
  int velocidade_dir = constrain((int)(vel_base - pid_local), -1023, 1023);
  motor.move(velocidade_esq, velocidade_dir);
}

void estadosBobo() {
  leituraSensores();

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

#endif
