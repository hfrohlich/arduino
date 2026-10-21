// Definição dos pinos
const int pistonPin = 2;             // Pistão
const int openBeerPin = 3;           // Abre-cerveja
const int closeBeerPin = 4;          // Fecha-cerveja
const int enterCO2Pin = 5;           // Entra-CO2
const int exitCO2Pin = 6;            // Saída-CO2
const int buzzerPin = 7;             // Apito
const int startButtonPin = A0;       // Botão de start
const int pistonUpSensorPin = A1;    // Sensor pistão levantado
const int pistonDownSensorPin = A2;  // Sensor pistão recolhido
const int emergencyButtonPin = A3;   // Botão de emergência - HIGH quando pressionado (chave NC)

//Tempos de enchimento
const unsigned long TEMPO_ENCHIMENTO = 20000; //Em ms.
const unsigned long TEMPO_ESPERA = 5000; //Em ms.
const unsigned long TEMPO_CO2 = 3000; //Em ms.
const unsigned long TEMPO_ANTIALIAS = 10; //Em ms.

// Definição dos estados
enum States { REPOUSO, SOBE_GARRAFA, ENCHE_GARRAFA, BAIXA_GARRAFA, PARADA_NO_ALTO };
States currentState = REPOUSO;

//Função para fechar a saída de cerveja
bool FechaCerveja (){
  digitalWrite(exitCO2Pin, LOW);
  digitalWrite(openBeerPin, LOW);
  delay(100);
  digitalWrite(closeBeerPin, HIGH);
  delay(300);
  digitalWrite(closeBeerPin, LOW);
  return true;
}

//Função para abrir a saída de cerveja
bool AbreCerveja(){
  digitalWrite(closeBeerPin, LOW);
//  delay(100);
  digitalWrite(openBeerPin, HIGH);
  delay(100);
  digitalWrite(exitCO2Pin, HIGH);
return true;
}

bool PausaCerveja(){  //Apenas interrompe a saída de CO2 para fins de estabilizar enchimento
  BeepButton();
  digitalWrite(exitCO2Pin, LOW);
  Serial.println("Enchimento pausado (saída CO2 fechada)");
  while (aadRead(startButtonPin) == LOW) {}  //Espera soltar o botão
  while (aadRead(startButtonPin) == HIGH) {}  //Espera Pressionar o botão novamente
  while (aadRead(startButtonPin) == LOW) {}  //Espera soltar o botão novamente
  digitalWrite(exitCO2Pin, HIGH);
  BeepButton();
  Serial.println("Enchimento retomado");
  return true;
}
  

//Função para subir garrafa
bool SobeGarrafa() {  //Retorna falso em caso de botão de parada ativado
//  FechaCerveja(); //Para o caso de botão de parada. Comentar se desnecessário.
  digitalWrite(pistonPin, HIGH);
  Serial.println("Garrafa subindo.");
  while (aadRead(pistonUpSensorPin) == HIGH) {  //Espera o pistão subir
    if (aadRead(emergencyButtonPin) == HIGH){
      Serial.println("Botão de parada: Subida interrompida!");
      digitalWrite(pistonPin, LOW);  //Desce o pistão em caso de botão de parada
      BeepEmergency();
      while (aadRead(pistonDownSensorPin) == HIGH) {} //Espera o pistão descer
      return false;
    }
  }
  Serial.println("Garrafa em cima.");
  delay(200);
  Serial.println("Pressurizando garrafa.");
  digitalWrite(enterCO2Pin, HIGH);
  delay(TEMPO_CO2);
  digitalWrite(enterCO2Pin, LOW);
  return true;
}

//Função para baixar garrafa
bool BaixaGarrafa(){
  digitalWrite(pistonPin, LOW);
  Serial.println("Garrafa descendo.");
  while (aadRead(pistonDownSensorPin) == HIGH) {  //Espera o pistão descer
    if (aadRead(emergencyButtonPin) == HIGH){
      Serial.println("Botão de parada: Descida interrompida!");
      digitalWrite(pistonPin, HIGH);
      BeepEmergency();
      while (aadRead(pistonUpSensorPin) == HIGH) {} //Espera o pistão subir
      FechaCerveja();
      return false;
    }
  } 
  Serial.println("Garrafa embaixo.");
  return true;
}

//Função para encher garrafa
//Argumentos:
//t_fill: tempo de enchimento em ms
//t_wait: tempo após enchimento e antes de descer a garrafa, para aguardar estabilização de bolhas/espuma.
bool EncheGarrafa (unsigned long t_fill, unsigned long t_wait){
  if(aadRead(pistonUpSensorPin) == HIGH){
    Serial.println("Erro: Garrafa não está em cima. Abortando enchimento.");
    BeepEmergency();
    return false;  //Retorna falso caso o pistão não esteja em cima.
  }
  AbreCerveja();
  Serial.println("Enchimento iniciado");
  for(int i=0;i<100;i++){  //Divide o tempo de enchimento para permitir monitoramento dos botões. (gambiarra)
    delay(t_fill/100-2*TEMPO_ANTIALIAS);  //Compensa o tempo das 2 leituras aadRead() = TEMPO_ANTIALIAS;
    if (aadRead(emergencyButtonPin) == HIGH) {
      Serial.println("Botão de parada: Enchimento interrompido!");
      FechaCerveja();
//      BaixaGarrafa(); //Não pode chamar BaixaGarrafa porque esta função também monitora o botão de parada.
      digitalWrite(pistonPin, LOW);
      BeepEmergency();
      while (aadRead(pistonDownSensorPin) == HIGH) {}  //Espera o pistão descer
      return false;
    } else if (aadRead(startButtonPin) == LOW) {  //Pausa o enchimento da garrafa (não encerra).
      PausaCerveja();
    }
  }
  FechaCerveja();
  Serial.println("Enchimento concluído. Tempo de espera iniciado.");
  for(int i=0;i<100;i++){  //Divide o tempo de pausa para permitir monitoramento dos botões. (gambiarra)
    delay(t_wait/100);
    if (aadRead(startButtonPin) == LOW) {
      BeepButton();
      break; //Pula o tempo restante de pausa final.
    }
  }  
  return true;
}

//Leitura física com filtro anti-alias
bool aadRead(int pin_rd) { //Leitura com filtro anti-alias
  int cnt = 0;
  for (int i=0; i<11; i++) { //11 iterações
    cnt = cnt + digitalRead(pin_rd); //Incrementa a soma para cada leitura HIGH.
    delay(TEMPO_ANTIALIAS/11); //Este tempo pode ser ajustado ou retirado.
  }
  if (cnt > 5) {
    return HIGH;
  } else {
    return LOW;
  }
}

void BeepButton() {
  digitalWrite(buzzerPin, HIGH);
  delay(100);
  digitalWrite(buzzerPin, LOW);
}

void BeepReady() {
  digitalWrite(buzzerPin, HIGH);
  delay(300);
  digitalWrite(buzzerPin, LOW);
  delay(300);
  digitalWrite(buzzerPin, HIGH);
  delay(300);
  digitalWrite(buzzerPin, LOW);
  delay(300);
  digitalWrite(buzzerPin, HIGH);
  delay(300);
  digitalWrite(buzzerPin, LOW);
}

void BeepEmergency(){
  digitalWrite(buzzerPin, HIGH);
  delay(100);
  digitalWrite(buzzerPin, LOW);
  delay(100);
  digitalWrite(buzzerPin, HIGH);
  delay(100);
  digitalWrite(buzzerPin, LOW);
  delay(100);
  digitalWrite(buzzerPin, HIGH);
  delay(100);
  digitalWrite(buzzerPin, LOW);
  delay(100);
  digitalWrite(buzzerPin, HIGH);
  delay(100);
  digitalWrite(buzzerPin, LOW);
  delay(100);
  digitalWrite(buzzerPin, HIGH);
  delay(100);
  digitalWrite(buzzerPin, LOW);
  delay(100);
  digitalWrite(buzzerPin, HIGH);
  delay(100);
  digitalWrite(buzzerPin, LOW);
}

void setup() {
  // Configuração dos pinos de saída
  pinMode(pistonPin, OUTPUT);
  pinMode(openBeerPin, OUTPUT);
  pinMode(closeBeerPin, OUTPUT);
  pinMode(enterCO2Pin, OUTPUT);
  pinMode(exitCO2Pin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  // Configuração dos pinos de entrada com INPUT_PULLUP
  pinMode(startButtonPin, INPUT_PULLUP);
  pinMode(pistonUpSensorPin, INPUT_PULLUP);
  pinMode(pistonDownSensorPin, INPUT_PULLUP);
  pinMode(emergencyButtonPin, INPUT_PULLUP); // Botão de emergência - HIGH quando pressionado (chave NC)
 
  // Inicializa todos os dispositivos desligados
  stopAll();

  // Inicializa a comunicação serial
  Serial.begin(9600);
}

void loop() {
//  // Leitura dos sensores e do botão (com INPUT_PULLUP, o estado LOW indica ativado)
//  int startButtonState = digitalRead(startButtonPin);
//  int pistonUpState = digitalRead(pistonUpSensorPin);
//  int pistonDownState = digitalRead(pistonDownSensorPin);
//  int emergencyButtonState = digitalRead(emergencyButtonPin);

//  // Verifica se o botão de emergência foi pressionado - enquanto estiver pressionado ficará em looping aqui:
//  if (emergencyButtonState == LOW) {
//    stopAll();
//    Serial.println("Emergência ativada! Processo cancelado!!! ");
//    delay(1000);
//    currentState = REPOUSO;
//    return;
//  }

//  delay(100);


  switch (currentState) {
    case REPOUSO:
      Serial.println("Estado: REPOUSO");
      stopAll();
      BeepReady();
//      while (aadRead(startButtonPin) == LOW) {}  //Espera que o botão de início seja solto antes de começar. NÃO PRECISA
//      Serial.println("Botão: desativado");
      while (aadRead(startButtonPin) == HIGH) {}  //Espera que o botão de início seja pressionado.
      BeepButton();
      Serial.println("Botão: ativado");
      if (aadRead(emergencyButtonPin) == LOW) currentState = SOBE_GARRAFA; //Próximo estado, caso não haja botão de parada ativo.
      break;

    case SOBE_GARRAFA:
      Serial.println("Estado: SOBE_GARRAFA");
      if (SobeGarrafa()) {
        currentState = ENCHE_GARRAFA;
      } else {
        currentState = REPOUSO;
      }
      break;

    case ENCHE_GARRAFA:
      Serial.println("Estado: ENCHE_GARRAFA");
//      if (EncheGarrafa(TEMPO_ENCHIMENTO, TEMPO_ESPERA)){
//        currentState = BAIXA_GARRAFA;
//      } else {
//        currentState = REPOUSO;
//      }
      EncheGarrafa(TEMPO_ENCHIMENTO, TEMPO_ESPERA);
      currentState = BAIXA_GARRAFA; //Se deu erro no enchimento e a garrafa já estiver baixa, o estado BAIXA_GARRAFA manda para REPOUSO imediatamente.
      break;

    case BAIXA_GARRAFA:
      Serial.println("Estado: BAIXA_GARRAFA");
      if (BaixaGarrafa()) {
        currentState = REPOUSO;
      } else {
        currentState = PARADA_NO_ALTO;
      }
      
//      // Desliga os dispositivos e encerra o ciclo
//      digitalWrite(openBeerPin, LOW);    // Desliga Abre-Cerveja
//      digitalWrite(exitCO2Pin, LOW);     // Desliga Saída-CO2
//      delay(250);
//
//      // TODO: REVISAR!!!  AQUI FIQUEI MEIO EM DÚVIDA, FAVOR ATENÇÃO MAIOR:
//      Serial.println("Fechando cerveja");
//     digitalWrite(closeBeerPin, HIGH);  // Aciona Fecha-Cerveja
//      delay(250);                        // Aguarda 250ms
//
//      Serial.println("Desacionando Fecha-Cerveja e Pistão");
//      digitalWrite(closeBeerPin, LOW);   // Desaciona Fecha-Cerveja // <- ??????
//      digitalWrite(pistonPin, LOW);      // Desaciona Pistão
//      delay(500);                
//      Serial.println("Ciclo terminou, indo para Aguarda-Fim");        // Aguarda 500ms
//      currentState = AGUARDA_FIM;
      break;

    case PARADA_NO_ALTO:
      Serial.println("Estado: PARADA_NO_ALTO");
      if (aadRead(emergencyButtonPin) == LOW) {
        if (BaixaGarrafa()) currentState = REPOUSO; //Sai do estado de parada só se não houver interrupção em BaixaGarrafa()
      }
      break;
  }
}
  // Função que desliga todos os componentes
  void stopAll() {
    digitalWrite(pistonPin, LOW);    // Desliga Pistão
    digitalWrite(openBeerPin, LOW);  // Desliga Abre-Cerveja
    digitalWrite(closeBeerPin, LOW); // Desliga Fecha-Cerveja
    digitalWrite(enterCO2Pin, LOW);  // Desliga Entra-CO2
    digitalWrite(exitCO2Pin, LOW);   // Desliga Saída-CO2
    digitalWrite(buzzerPin, LOW);    // Silencia Buzzer
  }

  
