#ifndef pid_H
#define pid_H


#include "Sensores.h"
#include "DRV8833.h"

DRV8833 motor(18, 19, 4, 23); // caçadora 19, 18, 23, 4 // ultra t 19, 18, 23, 4


// Leitura dos sensores
int leitura[4]; // [0]=esq, [1]=frente-esq, [2]=frente-dir, [3]=dir

// Velocidades e parâmetros PID
int vel_base = 550;
float erro_linha = 0, erro_linha_anterior = 0;
float P = 0, I = 0, D = 0, PID = 0;
float Kp = 450.0, Ki = 0.0, Kd = 0.0;
bool alvoDetectado = false; // true = pelo menos um sensor está vendo o oponente

// Tempo
unsigned long last_time = 0;


void leituraSensores() {
  leitura[0] = digitalRead(JEsq);
  leitura[1] = digitalRead(JFEsq);
  leitura[2] = digitalRead(JFDir);
  leitura[3] = digitalRead(JDir);
}

void leituraSensoresSD() { // leitura diferente exclusiva pra Seek and Destroy
  leitura[1] = digitalRead(JFEsq);
  leitura[2] = digitalRead(JFDir);
}

void calculoErroSensor() {
  leituraSensores();

  // Peso para cada sensor: esquerda negativo, direita positivo
  float peso[] = {-4, -2, 2, 4}; 
  float soma_pesos = 0;
  int ativos = 0;

  for (int i = 0; i < 4; i++) {
    if (leitura[i]) {
      soma_pesos += peso[i];
      ativos++;
    }
  }

  if (ativos > 0) {
    erro_linha = soma_pesos / ativos;
    alvoDetectado = true;
  } else {
    // Nenhum sensor vendo o oponente: erro neutro -> robô fica PARADO
    // (ver iSeeYou()/Calibragem(), que checam alvoDetectado antes de mover)
    erro_linha = 0;
    alvoDetectado = false;
  }
}

void pid() {
  unsigned long current_time = millis();
  float dt = (current_time - last_time) / 1000.0;

  calculoErroSensor();

  P = erro_linha;
  I += erro_linha * dt;
  D = (erro_linha - erro_linha_anterior) / dt;

  PID = (Kp * P) + (Ki * I) + (Kd * D);

  erro_linha_anterior = erro_linha;
  last_time = current_time;
}


void iSeeYou() { // não é uma estratégia e sim o ataque principal, mas pode ser selecionada no número 4 no controle, deve ser considerada a principal
  leituraSensores();
  pid();

  if (!alvoDetectado) {
    motor.stop(); // sem nenhum sensor vendo o oponente -> fica parado, não gira à toa
    return;
  }

  int velocidade_esq =  - PID;
  int velocidade_dir =  + PID;

  velocidade_esq = constrain(velocidade_esq, -1023, 1023);
  velocidade_dir = constrain(velocidade_dir, -1023, 1023);

  
  if (PID == 0) {
    motor.move(1023, 1023); // alvo detectado e perfeitamente centralizado -> avança em linha reta
  } else {
    motor.move(velocidade_esq, velocidade_dir);
  }
}
void Calibragem() { // MODO TESTE DE CALIBRAGEM DO PID
  leituraSensores();
  pid();

  if (!alvoDetectado) {
    motor.stop(); // sem alvo -> parado
    return;
  }

  int velocidade_esq =  - PID;
  int velocidade_dir =  + PID;

  velocidade_esq = constrain(velocidade_esq, -1023, 1023);
  velocidade_dir = constrain(velocidade_dir, -1023, 1023);

  
  if (PID == 0) {
    motor.stop();
  } else {
    motor.move(velocidade_esq, velocidade_dir);
  }
}

#endif
