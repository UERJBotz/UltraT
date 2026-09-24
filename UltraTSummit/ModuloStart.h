/*
 * ============================================================
 *  ModuloStart.h — Biblioteca de início de combate
 *  Robô Sumô Autônomo  |  ESP32 WROOM
 *  Versão 2.0 — Aprendizado automático de controle remoto
 * ============================================================
 *
 *  Funciona com QUALQUER controle remoto de TV.
 *  Os botões são aprendidos e salvos na memória flash do ESP32
 *  (Preferences/NVS), sobrevivendo a desligamentos.
 *
 *  ── COMO USAR ───────────────────────────────────────────────
 *
 *  PRIMEIRO USO (aprender os botões):
 *    1. Ligue o ESP32 segurando o botão BOOT (GPIO 0)
 *    2. O LED vai piscar 5x rápido — modo aprendizado ativo
 *    3. Aponte o controle e pressione o botão de PREPARAR
 *       (LED pisca 1x para confirmar)
 *    4. Pressione o botão de INICIAR
 *       (LED pisca 2x para confirmar)
 *    5. Pressione o botão de PARAR
 *       (LED pisca 3x para confirmar)
 *    6. LED acende fixo 1 segundo — aprendizado concluído!
 *    7. Os códigos ficam salvos. Nunca mais precisa repetir.
 *
 *  USO NORMAL (depois que os botões já foram aprendidos):
 *    Ligue normalmente, sem segurar BOOT.
 *    O módulo carrega os códigos salvos automaticamente.
 *
 *  RESETAR / TROCAR DE CONTROLE:
 *    Basta entrar no modo aprendizado novamente (segurar BOOT).
 *    Os novos códigos sobrescrevem os anteriores.
 *
 *  ── INTEGRAÇÃO NO .ino ──────────────────────────────────────
 *
 *    #include "ModuloStart.h"
 *
 *    void setup() {
 *      Serial.begin(115200);
 *      moduloStart.begin();
 *    }
 *
 *    void loop() {
 *      moduloStart.atualizar();
 *      if (moduloStart.emCombate()) {
 *        // lógica do robô aqui
 *      }
 *    }
 *
 *  Biblioteca necessária:
 *    IRremoteESP8266 (by crankyoldgit)
 * ============================================================
 */

#ifndef MODULO_START_H
#define MODULO_START_H

#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <Preferences.h>
#include "placa.h"   // NVS — memória flash não volátil do ESP32

// ─── Pinos ───────────────────────────────────────────────────
#define BTN_LEARN_PIN     0   
// ─────────────────────────────────────────────────────────────

// ─── Parâmetros ──────────────────────────────────────────────
#define TEMPO_ROUND_MS     180000   // Duração máxima do round (3 min)
#define LEARN_TIMEOUT_MS    10000   // Tempo máximo esperando cada botão (10 s)
// ─────────────────────────────────────────────────────────────

// ─── Códigos fixos de teste (sempre ativos) ───────────────────
// Estes 3 códigos SEMPRE funcionam, mesmo depois de você aprender o
// controle oficial do evento — os dois (fixo e aprendido) acionam o
// mesmo comando. Ou seja: cada ação (preparar/iniciar/parar) pode ser
// disparada por 2 controles diferentes ao mesmo tempo. Não precisa
// mexer em nada disso antes da competição.
//
// Troque pelos valores reais do SEU controle de testes: ligue o robô,
// aponte o controle e aperte os botões — os códigos em hexadecimal
// aparecem no Serial Monitor (linha "[IR] 0x..."). Copie os 3 valores
// lidos pra cá.
#define CODIGO_FIXO_PREPARAR  0x10UL
#define CODIGO_FIXO_INICIAR   0x810UL
#define CODIGO_FIXO_PARAR     0x410UL
// ─────────────────────────────────────────────────────────────

// ─── Namespace na flash ──────────────────────────────────────
#define NVS_NAMESPACE   "modulostart"
#define NVS_KEY_PREP    "cmd_preparar"
#define NVS_KEY_INIC    "cmd_iniciar"
#define NVS_KEY_PARA    "cmd_parar"
#define NVS_KEY_VALID   "configurado"
// ─────────────────────────────────────────────────────────────

// ─── Estados ─────────────────────────────────────────────────
enum EstadoStart {
  START_DESLIGADO,
  START_PREPARADO,
  START_COMBATE,
  START_PARADO
};
// ─────────────────────────────────────────────────────────────

class ModuloStart {
public:

  // ===========================================================
  //  BEGIN — chame no setup() do .ino (após Serial.begin)
  // ===========================================================
  void begin() {
    pinMode(LED_STRIP, OUTPUT);
    pinMode(BTN_LEARN_PIN,  INPUT_PULLUP);
    digitalWrite(LED_STRIP, LOW);

    _irrecv = new IRrecv(IR_PIN);
    _irrecv->enableIRIn();

    _estado = START_DESLIGADO;
    _tRound = 0;

    Serial.println();
    Serial.println("╔══════════════════════════════════════════╗");
    Serial.println("║       MÓDULO START — Robô Sumô  v2.0     ║");
    Serial.println("╚══════════════════════════════════════════╝");

    // Verifica se o botão BOOT está pressionado ao ligar
    if (digitalRead(BTN_LEARN_PIN) == LOW) {
      _modoAprendizado();   // entra no modo aprendizado
    } else {
      _carregarCodigos();   // carrega o controle aprendido da flash (os fixos já valem sempre)
    }
  }

  // ===========================================================
  //  ATUALIZAR — chame SEMPRE como primeira linha do loop()
  // ===========================================================
  void atualizar() {
    unsigned long agora = millis();
    uint64_t codigo = _lerIR();

    // Parada de emergência — válida em qualquer estado ativo
    if (codigo != 0 && _ehParar(codigo) && _estado != START_DESLIGADO) {
      _estado = START_PARADO;
      _log("PARADA DE EMERGENCIA — juiz interrompeu!");
      return;
    }

    switch (_estado) {
      // ── Aguarda PREPARAR ───────────────────────────────────
      case START_DESLIGADO:
        digitalWrite(LED_STRIP, LOW);
        if (codigo != 0 && _ehPreparar(codigo)) {
          _estado = START_PREPARADO;
          _log("PREPARADO — aguardando INICIAR do juiz");
        }
        break;

      // ── Aguarda INICIAR ────────────────────────────────────
      case START_PREPARADO:
        // LED pulsa devagar
        digitalWrite(LED_STRIP, (agora / 700) % 2 == 0);
        if (codigo != 0 && _ehPreparar(codigo)) {
          _log("(PREPARAR recebido novamente — ja preparado)");
        }
        else if (codigo != 0 && _ehIniciar(codigo)) {
          _tRound = agora;
          _estado = START_COMBATE;
          Serial.println();
          Serial.println("╔══════════════════════════════════════════╗");
          Serial.println("║         >>> COMBATE INICIADO! <<<        ║");
          Serial.println("╚══════════════════════════════════════════╝");
          digitalWrite(LED_STRIP, HIGH);
        }
        break;

      // ── Combate em andamento ───────────────────────────────
      case START_COMBATE:
        if (agora - _tRound >= TEMPO_ROUND_MS) {
          _estado = START_PARADO;
          _log("TEMPO ESGOTADO — fim do round!");
          break;
        }
        if (codigo != 0 && _ehIniciar(codigo)) {
          _tRound = agora;
          _log("(INICIAR durante combate — round reiniciado)");
          break;
        }
        digitalWrite(LED_STRIP, HIGH);
        break;

      // ── Parado / emergência ────────────────────────────────
      case START_PARADO:
        digitalWrite(LED_STRIP, (agora / 600) % 2 == 0);
        if (codigo != 0 && _ehPreparar(codigo)) {
          _estado = START_PREPARADO;
          _log("PREPARADO — aguardando INICIAR do juiz");
        }
        break;
    }
  }

  // ===========================================================
  //  CONSULTAS DE ESTADO
  // ===========================================================
  bool emCombate() { return _estado == START_COMBATE;   }
  bool preparado() { return _estado == START_PREPARADO; }
  bool parado()    { return _estado == START_PARADO;    }
  bool desligado() { return _estado == START_DESLIGADO; }
  
  const char* estadoTexto() {
    switch (_estado) {
      case START_DESLIGADO: return "DESLIGADO";
      case START_PREPARADO: return "PREPARADO";
      case START_COMBATE:   return "COMBATE";
      case START_PARADO:    return "PARADO";
      default:              return "DESCONHECIDO";
    }
  }

  // Retorna true se os botões já foram aprendidos e salvos
  bool configurado() { return _configurado; }
  decode_results* ultimoResultado() { 
    if (_ultimoCodigoValido) {
      _ultimoCodigoValido = false;
      return &_irResult;
    }
    return nullptr;
  }
// ─────────────────────────────────────────────────────────────
private:

  IRrecv*        _irrecv      = nullptr;
  decode_results _irResult;
  EstadoStart    _estado      = START_DESLIGADO;
  unsigned long  _tRound      = 0;
  bool           _configurado = false;
  bool _ultimoCodigoValido = false;
  // Códigos APRENDIDOS — carregados da flash ou pelo modo aprendizado.
  // 0 = ainda não foi aprendido nenhum código pra esse botão.
  // Os códigos FIXOS (CODIGO_FIXO_*) são constantes e sempre válidos,
  // independente destes aqui — os dois funcionam ao mesmo tempo.
  uint64_t _cmd_preparar = 0;
  uint64_t _cmd_iniciar  = 0;
  uint64_t _cmd_parar    = 0;

  Preferences _prefs;

  // ===========================================================
  //  Comparações de botão: aceitam o código FIXO ou o APRENDIDO
  //  (qualquer um dos dois controles aciona o mesmo comando)
  // ===========================================================
  bool _ehPreparar(uint64_t c) {
    return (c == CODIGO_FIXO_PREPARAR) || (_cmd_preparar != 0 && c == _cmd_preparar);
  }
  bool _ehIniciar(uint64_t c) {
    return (c == CODIGO_FIXO_INICIAR) || (_cmd_iniciar != 0 && c == _cmd_iniciar);
  }
  bool _ehParar(uint64_t c) {
    return (c == CODIGO_FIXO_PARAR) || (_cmd_parar != 0 && c == _cmd_parar);
  }

  // ===========================================================
  //  MODO APRENDIZADO
  //  Chamado quando o BOOT está pressionado ao ligar
  // ===========================================================
  void _modoAprendizado() {
    Serial.println();
    Serial.println("══════════════════════════════════════════");
    Serial.println("  MODO APRENDIZADO ATIVADO");
    Serial.println("  Aponte o controle remoto para o sensor");
    Serial.println("══════════════════════════════════════════");

    // Sinaliza entrada no modo aprendizado (5x pisca rápido)
    _blink(5, 80);

    // ── Passo 1: botão PREPARAR ───────────────────────────────
    Serial.println();
    Serial.println("  [1/3] Pressione o botao de PREPARAR...");
    // Exclui os códigos fixos dos OUTROS botões, pra não aprender por
    // engano um código que já significa outra coisa
    _cmd_preparar = _aguardarBotao(CODIGO_FIXO_INICIAR, CODIGO_FIXO_PARAR);
    if (_cmd_preparar == 0) { _erroAprendizado(); return; }
    Serial.print("         Salvo: 0x");
    serialPrintUint64(_cmd_preparar, HEX);
    Serial.println();
    _blink(1, 200);   // 1 piscada = confirmação passo 1
    delay(600);

    // ── Passo 2: botão INICIAR ────────────────────────────────
    Serial.println();
    Serial.println("  [2/3] Pressione o botao de INICIAR...");
    // Exclui o já aprendido (preparar) + os códigos fixos dos outros botões
    _cmd_iniciar = _aguardarBotao(_cmd_preparar, CODIGO_FIXO_PREPARAR, CODIGO_FIXO_PARAR);
    if (_cmd_iniciar == 0) { _erroAprendizado(); return; }
    Serial.print("         Salvo: 0x");
    serialPrintUint64(_cmd_iniciar, HEX);
    Serial.println();
    _blink(2, 200);   // 2 piscadas = confirmação passo 2
    delay(600);

    // ── Passo 3: botão PARAR ──────────────────────────────────
    Serial.println();
    Serial.println("  [3/3] Pressione o botao de PARAR...");
    // Exclui os dois já aprendidos + os códigos fixos dos outros botões
    _cmd_parar = _aguardarBotao(_cmd_preparar, _cmd_iniciar, CODIGO_FIXO_PREPARAR, CODIGO_FIXO_INICIAR);
    if (_cmd_parar == 0) { _erroAprendizado(); return; }
    Serial.print("         Salvo: 0x");
    serialPrintUint64(_cmd_parar, HEX);
    Serial.println();
    _blink(3, 200);   // 3 piscadas = confirmação passo 3
    delay(600);

    // ── Salva na flash ────────────────────────────────────────
    _salvarCodigos();

    // Confirmação final — LED aceso por 1 segundo
    digitalWrite(LED_STRIP, HIGH);
    Serial.println();
    Serial.println("══════════════════════════════════════════");
    Serial.println("  APRENDIZADO CONCLUIDO!");
    Serial.println("  Codigos salvos na memoria flash.");
    Serial.println("  Pode desligar e religar normalmente.");
    Serial.println("══════════════════════════════════════════");
    delay(1000);
    digitalWrite(LED_STRIP, LOW);

    _imprimirCodigos();
  }

  // ===========================================================
  //  Aguarda um botão válido do controle (com timeout)
  //  Rejeita automaticamente: repeat, códigos já usados,
  //  valores inválidos, e botão pressionado continuamente
  // ===========================================================
  uint64_t _aguardarBotao(uint64_t excluir1 = 0, uint64_t excluir2 = 0, uint64_t excluir3 = 0, uint64_t excluir4 = 0) {
    unsigned long inicio = millis();

    while (millis() - inicio < LEARN_TIMEOUT_MS) {

      // LED pisca lentamente enquanto aguarda
      digitalWrite(LED_STRIP, (millis() / 400) % 2 == 0);

      if (!_irrecv->decode(&_irResult)) continue;

      uint64_t codigo = _irResult.value;
      _irrecv->resume();

      // Rejeita: repeat, inválido, zero
      if (codigo == 0xFFFFFFFFFFFFFFFFULL || codigo == 0) continue;

      // Rejeita se for igual a um código já usado por outro botão
      // (já aprendido nesta sessão, ou fixo de outra ação)
      if ((excluir1 != 0 && codigo == excluir1) ||
          (excluir2 != 0 && codigo == excluir2) ||
          (excluir3 != 0 && codigo == excluir3) ||
          (excluir4 != 0 && codigo == excluir4)) {
        Serial.println("    (botao ja usado — pressione um diferente)");
        continue;
      }

      // Aceito!
      digitalWrite(LED_STRIP, LOW);
      return codigo;
    }

    // Timeout — nenhum botão pressionado a tempo
    return 0;
  }

  // ===========================================================
  //  Salva os três códigos na memória flash (NVS)
  // ===========================================================
  void _salvarCodigos() {
    _prefs.begin(NVS_NAMESPACE, false); // false = leitura/escrita
    _prefs.putULong64(NVS_KEY_PREP,  _cmd_preparar);
    _prefs.putULong64(NVS_KEY_INIC,  _cmd_iniciar);
    _prefs.putULong64(NVS_KEY_PARA,  _cmd_parar);
    _prefs.putBool   (NVS_KEY_VALID, true);
    _prefs.end();
    _configurado = true;
  }

  // ===========================================================
  //  Carrega os códigos APRENDIDOS salvos na memória flash (NVS).
  //  Os códigos FIXOS (topo do arquivo) valem sempre, independente
  //  de ter ou não um controle aprendido — os dois funcionam juntos,
  //  pra sempre. Não é preciso mexer em nada antes da competição.
  // ===========================================================
  void _carregarCodigos() {
    _prefs.begin(NVS_NAMESPACE, true); // true = somente leitura
    bool valido = _prefs.getBool(NVS_KEY_VALID, false);

    if (valido) {
      _cmd_preparar = _prefs.getULong64(NVS_KEY_PREP, 0);
      _cmd_iniciar  = _prefs.getULong64(NVS_KEY_INIC, 0);
      _cmd_parar    = _prefs.getULong64(NVS_KEY_PARA, 0);
      _configurado  = true;
      _prefs.end();

      Serial.println("  Controle oficial aprendido (carregado da flash):");
      _imprimirCodigos();
    } else {
      _prefs.end();
      _configurado = false; // nenhum controle oficial aprendido ainda

      Serial.println();
      Serial.println("  Nenhum controle oficial aprendido ainda.");
      Serial.println("  (segure o BOOT ao ligar pra aprender um — ele passa a");
      Serial.println("   funcionar junto com os fixos, sem substituir nada)");
    }

    Serial.println("  Codigos FIXOS de teste (sempre ativos, mesmo com um controle aprendido):");
    Serial.print  ("    PREPARAR: 0x"); serialPrintUint64(CODIGO_FIXO_PREPARAR, HEX); Serial.println();
    Serial.print  ("    INICIAR : 0x"); serialPrintUint64(CODIGO_FIXO_INICIAR,  HEX); Serial.println();
    Serial.print  ("    PARAR   : 0x"); serialPrintUint64(CODIGO_FIXO_PARAR,    HEX); Serial.println();
    Serial.println("  Aguardando comando PREPARAR...");
    Serial.println("------------------------------------------");
    _blink(2, 300);
  }

  // ===========================================================
  //  Lê o receptor IR — retorna 0 se nada ou repeat
  // ===========================================================
  uint64_t _lerIR() {
    if (!_irrecv->decode(&_irResult)) return 0;

    uint64_t codigo = _irResult.value;
    _irrecv->resume();

    if (codigo == 0xFFFFFFFFFFFFFFFFULL || codigo == 0) return 0;

    Serial.print("  [IR] 0x");
    serialPrintUint64(codigo, HEX);
    Serial.print(" (");
    Serial.print(typeToString(_irResult.decode_type, _irResult.repeat));
    Serial.println(")");
    
    _ultimoCodigoValido = true;
    return codigo;
  }

  // ===========================================================
  //  Utilitários
  // ===========================================================

  void _imprimirCodigos() {
    Serial.println("  ┌─────────────────────────────────────┐");
    Serial.print  ("  │  PREPARAR (aprendido) : 0x"); serialPrintUint64(_cmd_preparar, HEX); Serial.println();
    Serial.print  ("  │  INICIAR  (aprendido) : 0x"); serialPrintUint64(_cmd_iniciar,  HEX); Serial.println();
    Serial.print  ("  │  PARAR    (aprendido) : 0x"); serialPrintUint64(_cmd_parar,    HEX); Serial.println();
    Serial.println("  └─────────────────────────────────────┘");
  }

  void _erroAprendizado() {
    Serial.println();
    Serial.println("  !! ERRO: tempo esgotado sem receber sinal !!");
    Serial.println("  Reinicie e tente novamente.");
    // Pisca rápido por 2 segundos para indicar erro
    for (int i = 0; i < 20; i++) {
      digitalWrite(LED_STRIP, !digitalRead(LED_STRIP));
      delay(100);
    }
    digitalWrite(LED_STRIP, LOW);
  }

  void _log(const char* msg) {
    Serial.println();
    Serial.print("  "); Serial.println(msg);
    Serial.println("------------------------------------------");
  }

  void _blink(int n, int ms) {
    for (int i = 0; i < n; i++) {
      digitalWrite(LED_STRIP, HIGH); delay(ms);
      digitalWrite(LED_STRIP, LOW);  delay(ms);
    }
  }

  // Pisca padrão SOS (3 curto + 3 longo + 3 curto) — sem controle salvo
  void _blinkSOS() {
    for (int i = 0; i < 3; i++) { digitalWrite(LED_STRIP, HIGH); delay(150); digitalWrite(LED_STRIP, LOW); delay(150); }
    for (int i = 0; i < 3; i++) { digitalWrite(LED_STRIP, HIGH); delay(400); digitalWrite(LED_STRIP, LOW); delay(150); }
    for (int i = 0; i < 3; i++) { digitalWrite(LED_STRIP, HIGH); delay(150); digitalWrite(LED_STRIP, LOW); delay(150); }
  }
};

// Instância global — pronta para usar no .ino
ModuloStart moduloStart;

#endif // MODULO_START_H
