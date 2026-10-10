#ifndef Estrategias_H
#define Estrategias_H

#include "PID.h"

#define VEL_SEEK 600
#define VEL_CONTORNO_MIN 600
#define VEL_CONTORNO_MAX 800

void MadMax() {
  motor.move(1023, 1023);
}

void ParaTras() {
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

enum vista {
  SEM_INIMIGO,
  INIMIGO_FRENTE,
  INIMIGO_ESQ,
  INIMIGO_FRENTE_ESQ,
  INIMIGO_DIR,
  INIMIGO_FRENTE_DIR,
};

enum vista ver(bool leitura[6]) {
  //! faltam algumas combinações aqui
  if (leitura[1] && leitura[2]) return INIMIGO_FRENTE;

  if (leitura[0]) return INIMIGO_ESQ;
  if (leitura[1]) return INIMIGO_FRENTE_ESQ;
  if (leitura[2]) return INIMIGO_FRENTE_DIR;
  if (leitura[3]) return INIMIGO_DIR;

  return SEM_INIMIGO;
}

void __MaquinaEstados(enum vista vista) {
  switch (vista) {
    case INIMIGO_ESQ:
      Serial.println("Left Detected!");
      motor.move(-VEL_SEEK, VEL_SEEK);
      break;

    case INIMIGO_FRENTE_ESQ:
      Serial.println("Left Soft Detected!");
      motor.move(-VEL_SEEK/2, VEL_SEEK/2);
      break;

    case INIMIGO_FRENTE:
      Serial.println("ROBOT ATTACK!");
      motor.move(1023, 1023);
      break;

    case INIMIGO_FRENTE_DIR:
      Serial.println("Right Soft Detected!");
      motor.move(VEL_SEEK/2, -VEL_SEEK/2);
      break;

    case SEM_INIMIGO:
    case INIMIGO_DIR:
      Serial.println("Right Detected!");
      motor.move(VEL_SEEK, -VEL_SEEK);
      break;
  }
}

void TesteContornarLParar() {
  leituraSensoresConservadora();

  static enum ESTADO_BOBO {
    BOBO_SEM_INIMIGO,
    BOBO_INIMIGO,
  } estadoAtual = BOBO_SEM_INIMIGO;

  if (leitura[0] || leitura[1] || leitura[2] || leitura[3]) estadoAtual = BOBO_INIMIGO;
  //else estadoAtual = BOBO_SEM_INIMIGO;

  switch (estadoAtual) {
    case BOBO_INIMIGO: motor.move((int16_t)0,(int16_t)0); break;

    case BOBO_SEM_INIMIGO:
      {
        Serial.println("Contornando");
        if (leitura[5]) motor.move(-VEL_CONTORNO_MAX, VEL_CONTORNO_MAX);
        else if (leitura[4]) motor.move(VEL_CONTORNO_MAX, -VEL_CONTORNO_MAX);
        else motor.move(VEL_CONTORNO_MAX, VEL_CONTORNO_MIN);
      }
      break;
  }
}

void ContornarLPID() {
  leituraSensoresConservadora();

  static enum ESTADO_BOBO {
    BOBO_SEM_INIMIGO,
    BOBO_INIMIGO,
  } estadoAtual = BOBO_SEM_INIMIGO;

  #define PID_MIX
  #ifdef PID_MIX
    pid();
    if (alvoDetectado) estadoAtual = BOBO_INIMIGO;
    else estadoAtual = BOBO_SEM_INIMIGO;
  #else
    if (leitura[0] || leitura[1] ||
        leitura[2] || leitura[3]) estadoAtual = BOBO_INIMIGO;
    if (alvoDetectado) estadoAtual = BOBO_INIMIGO;
    else estadoAtual = BOBO_SEM_INIMIGO;
  #endif

  switch (estadoAtual) {
    case BOBO_INIMIGO: {
      #ifdef PID_MIX
        if (PID == 0) { //! epsilon
          motor.move(1023, 1023); // alvo detectado e perfeitamente centralizado -> avança em linha reta
        } else {
          motor.move(
            constrain(+PID, -1023, 1023),
            constrain(-PID, -1023, 1023)
          );
        }
      #else
        iSeeYou();
      #endif
    } break;

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

  static enum ESTADO_BOBO {
    BOBO_SEM_INIMIGO,
    BOBO_INIMIGO,
  } estadoAtual = BOBO_SEM_INIMIGO;

  if (leitura[0] || leitura[1] || leitura[2] || leitura[3]) estadoAtual = BOBO_INIMIGO;
  //else estadoAtual = BOBO_SEM_INIMIGO;

  switch (estadoAtual) {
    case BOBO_INIMIGO: __MaquinaEstados(ver(leitura)); break;

    case BOBO_SEM_INIMIGO:
      {
        Serial.println("Contornando");
        if (leitura[5]) motor.move(-VEL_CONTORNO_MAX, VEL_CONTORNO_MAX);
        else if (leitura[4]) motor.move(VEL_CONTORNO_MAX, -VEL_CONTORNO_MAX);
        else motor.move(VEL_CONTORNO_MAX, VEL_CONTORNO_MIN);
      }
      break;
  }
}

void MaquinaEstados() {
  leituraSensores();
  __MaquinaEstados(ver(leitura));
}

#endif
