/*
  16/11/2025, para a summit;
  Auto e RC usando ESPNOW;
  Alimentar o ESP32 C3 Mini do rádio pistola com um power bank ou um celular, as pilhas podem não dar conta,
  afetando a direção do robô;

  https://github.com/TRZN11/UltraT/blob/main/UltraTSummit
*/

#include "DRV8833.h"
#include "PID.h"
#include "Whiplash.h"
#include "Estrategias.h"
#include "Sharingan.h"
#include "LEDFX.h"
#include "ModuloStart.h"
#include "SeletorEstrategia.h"

#define boot 0

void setup() {
  Serial.begin(115200);
  motor.begin();
  moduloStart.begin();   // sensor IR no pino 15 (não mudar)
  setupSensores();
  pixels.begin();
  pixels.setBrightness(40); // 0 (apagado) a 255 (máximo) — ajuste aqui pra calibrar o brilho
  pinMode(boot, INPUT_PULLUP);
}

void LED_Estrategias() {
  uint32_t cores[7] = {
    pixels.Color(0,125,125),   // 1 — (iSeeYou)
    pixels.Color(0,125,125),   // 2 — (Whiplash)
    pixels.Color(0,125,125),   // 3 — (Sharingan)
    pixels.Color(0,125,125),   // 4 — (SeekAndDestroy L)
    pixels.Color(0,125,125),   // 5 — (SeekAndDestroy R)
    pixels.Color(0,125,125),   // 6 — (Para Tras)
    pixels.Color(0,125,125),   // 0 — (Calibragem)
  };

  int idx      = seletorEstrategia.estrategiaAtual(); // 0 a 6
  int num_leds = idx + 1;                             // 1 a 7 LEDs

  pixels.clear();
  for (int i = 0; i < num_leds; i++) {
    pixels.setPixelColor(i, cores[idx]);
  }
  pixels.show();
}

void loop() {
  moduloStart.atualizar();
  motor.update();  // Processa fila de movimentos com timers

  // Detecta o INÍCIO de cada combate pra permitir que o semicírculo do
  // SeekAndDestroy (estratégias 4 e 5) rode de novo no próximo round
  static bool emCombateAnterior = false;
  bool emCombateAgora = moduloStart.emCombate();
  if (emCombateAgora && !emCombateAnterior) {
    resetSeekAndDestroy();
  }
  emCombateAnterior = emCombateAgora;

  // ── DESLIGADO: antes do PREPARAR ─────────────────────────
  if (moduloStart.desligado()) {
    LED_Estrategias();
    seletorEstrategia.atualizar(moduloStart.ultimoResultado());
    Serial.print("Estrategia: ");
    Serial.println(seletorEstrategia.nomeAtual()); // mostra leitura dos sensores nos LEDs
  }

  // ── PREPARADO: escolha de estratégia ─────────────────────
  else if (moduloStart.preparado()) {
    pixels.clear();
    ledDetection();
    motor.stop();
    Serial.print("Estrategia: ");
    Serial.println(seletorEstrategia.nomeAtual());
  }

  // ── COMBATE: executa a estratégia escolhida ───────────────
  else if (moduloStart.emCombate()) {
    pixels.clear();
    ledLight(0, 255, 0); // LED verde = combate ativo

    switch (seletorEstrategia.estrategiaAtual()) {
      case ESTRATEGIA_1: iSeeYou();          break;
      case ESTRATEGIA_2: BOBO();          break;
      // case ESTRATEGIA_2: whiplash();         break;
      case ESTRATEGIA_3: Sharingan();        break;
      case ESTRATEGIA_4: SeekAndDestroy_L(); break;
      case ESTRATEGIA_5: SeekAndDestroy_R(); break;
      case ESTRATEGIA_6: paraTras();         break;
      case ESTRATEGIA_0: Calibragem();       break;
    }
  }
  else if (moduloStart.preparado()) {
    Serial.println("(PREPARAR recebido novamente — ja preparado)"); // não retirar essa linha (aparentemente dá erro para iniciar com o IR
  }
  else if (moduloStart.emCombate()) { // número 2 no controle
    pixels.clear();
    ledLight(0, 125, 0);
    Serial.println(seletorEstrategia.nomeAtual());
  }
  else if (moduloStart.parado()) { // número 3 no controle
    motor.clear_moving();  // Limpa fila de movimentos pendentes
    motor.stop();
    pixels.clear();
    Serial.println("-> sumo stop");
  }
}