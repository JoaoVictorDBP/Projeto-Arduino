// Projeto Arduino - Redes de Computadores
// Transmissor Serial Síncrono

// Alunos:
// - João Victor De Bortoli Prado - 13672071
// - Lucas Rodrigues Baptista - 15577631

// Atribuindo pinos e baud rate

#define PINO_TX 13
#define PINO_CLK 5

#define BAUD_RATE 1

#define PINO_RTS 7
#define PINO_CTS 6

#include "Temporizador.h"

// -----------------------------------------------------------------
//  Variaveis globais do transmissor 

volatile char dado;

// Indica qual bit do frame sera enviado no momento

volatile int indice_bit = 0;

// Vetor contendo os 8 bits ASCII + 1 bit de paridade

volatile byte frame[9]; // 8 bits + paridade

// Indica o valor da saida do clock
// Os valores sao escritos na borda de subida do clock
// e lidos na borda de descida

volatile byte clk = LOW;

// Maquina de estados do transmissor:
// IDLE -> aguardando caractere
// ESPERA_CTS -> aguardando autorizacao do receptor
// TRANSMITINDO -> enviando bits
// FINALIZANDO -> aguardando CTS voltar a zero

enum Estado {
  IDLE,
  ESPERA_CTS,
  TRANSMITINDO,
  FINALIZANDO
};

volatile Estado estado = IDLE;

// -----------------------------------------------------------------
// Calcula bit de paridade - Ímpar

bool bitParidade(char dado){
  int count = 0;

  for(int i = 0; i < 8; i++){
    if(dado & (1 << i)){
      count++;
    }
  }

  // paridade ímpar
  return (count % 2 == 0); 
}

// -----------------------------------------------------------------
// Executada automaticamente a cada intervalo do baud rate.
// Envia 1 bit por interrupcao.

ISR(TIMER1_COMPA_vect){

  if (estado != TRANSMITINDO)
    return;

  if(clk == LOW){

    // Escreve o próximo bit na saida
    digitalWrite(PINO_TX, frame[indice_bit]);

    // Serial.print("Enviando bit: ");
    // Serial.println(frame[indice_bit]);

    clk = HIGH;
  }
  else {

    // Troca para o proximo bit
    indice_bit++;

    clk = LOW;
  }

  digitalWrite(PINO_CLK, clk);

  if(indice_bit > 8){

    // Terminou transmissão

    paraTemporizador();

    estado = FINALIZANDO;
    //Serial.println("Transmissao finalizada");

    indice_bit = 0;

    digitalWrite(PINO_RTS, LOW); // encerra handshake
  }
}

// -----------------------------------------------------------------
// Executada uma vez quando o Arduino reseta

void setup(){

  // Desabilita interrupcoes temporariamente

  noInterrupts();

  // Configura porta serial (Serial Monitor - Ctrl + Shift + M)

  Serial.begin(9600);

  // Configura pino TX como saida

  pinMode(PINO_TX, OUTPUT);

  // Configura o pino CLK como saida

  pinMode(PINO_CLK, OUTPUT);

  // Inicializa RTS e CTS

  pinMode(PINO_RTS, OUTPUT);
  pinMode(PINO_CTS, INPUT);

  digitalWrite(PINO_TX, LOW);
  digitalWrite(PINO_RTS, LOW);
  digitalWrite(PINO_CLK, LOW);

  // Configura timer

  configuraTemporizador(BAUD_RATE);

  // Reabilita interrupcoes

  interrupts();

  Serial.println("Transmissor pronto.");
}

// -----------------------------------------------------------------
// O loop() eh executado continuamente (como um while(true))

void loop () {

  switch(estado){

    case IDLE:
      if(Serial.available()){
        dado = Serial.read();

        Serial.print("Dado recebido: ");
        Serial.println(dado);

        // Monta frame
        // Extrai bit a bit do caractere (LSB primeiro)

        for(int i = 0; i < 8; i++){
          frame[i] = (dado >> i) & 1;
        }

        frame[8] = bitParidade(dado);

        Serial.print("Bit de paridade: ");
        Serial.println(frame[8]);

        indice_bit = 0;

        // Inicia handshake
        // RTS = transmissor solicita permissao para enviar dados

        digitalWrite(PINO_RTS, HIGH);
        Serial.println("RTS = 1 (aguardando CTS)");

        estado = ESPERA_CTS;
      }

      break;

    case ESPERA_CTS:

      if(digitalRead(PINO_CTS) == HIGH){
        Serial.println("CTS recebido, iniciando transmissao");

        estado = TRANSMITINDO;
        iniciaTemporizador();
      }

      break;

    case TRANSMITINDO:

      // Tudo acontece no ISR

      break;

    case FINALIZANDO:

      // Receptor terminou processamento e liberou CTS

       if (digitalRead(PINO_CTS) == LOW) {

        Serial.println("CTS = 0. Comunicacao encerrada.");

        estado = IDLE;
      }

      break;
  }
}
