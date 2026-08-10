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


int      SND_L_EXTERNO    = 1023;
int      SND_L_INTERNO    = 650;   // CALIBRE AQUI — curvatura da busca ESQUERDA
uint32_t SND_L_DURACAO_MS = 800;   // CALIBRE AQUI — duração do semicírculo esquerdo (ms)

int      SND_R_EXTERNO    = 1023;
int      SND_R_INTERNO    = 300;   // CALIBRE AQUI — curvatura da busca DIREITA
uint32_t SND_R_DURACAO_MS = 800;   // CALIBRE AQUI — duração do semicírculo direito (ms)

bool _SND_L_feito = false;
bool _SND_R_feito = false;


void resetSeekAndDestroy() {
  _SND_L_feito = false;
  _SND_R_feito = false;
}

void _semicirculoBloqueante(int vl, int vr, uint32_t duracao_ms) {
  motor.move_for(vl, vr, duracao_ms);
  delay(duracao_ms); // segura aqui até o tempo do movimento passar
}

void SeekAndDestroy_L(){ // estratégia número 4 no controle — busca pela lateral ESQUERDA
  if (!_SND_L_feito) {
    Serial.println("SeekAndDestroy_L: executando semicirculo (bloqueante)...");
    _semicirculoBloqueante(SND_L_INTERNO, SND_L_EXTERNO, SND_L_DURACAO_MS);
    _SND_L_feito = true;
    Serial.println("SeekAndDestroy_L: semicirculo concluido -> PID (iSeeYou)");
  }
  iSeeYou(); // depois do semicírculo, PID assume o resto do round
}

void SeekAndDestroy_R(){ // estratégia número 5 no controle — busca pela lateral DIREITA
  if (!_SND_R_feito) {
    Serial.println("SeekAndDestroy_R: executando semicirculo (bloqueante)...");
    _semicirculoBloqueante(SND_R_EXTERNO, SND_R_INTERNO, SND_R_DURACAO_MS);
    _SND_R_feito = true;
    Serial.println("SeekAndDestroy_R: semicirculo concluido -> PID (iSeeYou)");
  }
  iSeeYou();
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
