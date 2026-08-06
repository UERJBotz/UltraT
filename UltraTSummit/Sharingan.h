#ifndef Sharingan_H
#define Sharingan_H

#include "sensores.h"
#include "PID.h"
#include "Whiplash.h"

// Estados para máquina de estados não-bloqueante
enum SharinganState {
  SHARINGAN_SORTEIO,
  SHARINGAN_GIRANDO,
  SHARINGAN_WHIPLASH
};

int sorteio = 0; 
int tempoGiro = 150; // em millis
bool escolha = false; // indica se já sorteou
SharinganState sharinganState = SHARINGAN_SORTEIO;
unsigned long sharinganTimer = 0;

void Sharingan(){
  unsigned long agora = millis();
  
  switch (sharinganState) {
    case SHARINGAN_SORTEIO:
      // Faz o sorteio apenas uma vez
      if (!escolha) {
        sorteio = random(1, 3); // sorteio de 1 à 2
        escolha = true;
      }
      // Inicia o giro
      if (sorteio == 1) {
        motor.move(-900, 900);
      } else {
        motor.move(900, -900);
      }
      sharinganTimer = agora;
      sharinganState = SHARINGAN_GIRANDO;
      break;

    case SHARINGAN_GIRANDO:
      // Aguarda o tempo de giro
      if (agora - sharinganTimer >= tempoGiro) {
        // Transiciona para whiplash
        whiplash();
        sharinganState = SHARINGAN_WHIPLASH;
      }
      break;

    case SHARINGAN_WHIPLASH:
      // whiplash() já está em execução, mantém a máquina de estados
      break;
  }
}

#endif
