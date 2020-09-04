// Programma realizzato da Giuseppe Del Campo, IIS E.Majorana 2017 - 2018
// Serra Automatizzata realizzata su Arduino Mega 2560 R3, con utilizzo di sensore di temperatura e umidità (DHT22),
// display LCD 1602A, Modulo Audio DFPlayer SKU:DFR0299, Pompa dell'acqua DC30B, sensori YL-69 e Modulo YL-38 etc.
// Copyright (C) 2018 Del Campo Giuseppe

// Dichiarazione librerie utilizzate
#include <DFRobotDFPlayerMini.h>
#include "Arduino.h"
#include <DHT.h>;
#include <LiquidCrystal.h>
#include <SoftwareSerial.h>
LiquidCrystal lcd(12, 13, 5, 4, 8, 2);
#define DHTPIN 3
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
SoftwareSerial mySoftwareSerial(10, 11); // RX, TX
DFRobotDFPlayerMini myDFPlayer;

// Variabile sensore DHT22
int chk;

// Variabili assegnazione valori dei sensori
int temperatura;
int humidity;
int earthHumidity;

// Inizializzazione richieste e variabili di controllo. Solo valori positivi
unsigned short int requestTemp = 0;
unsigned short int requestHum = 0;
unsigned short int busy = 0;
unsigned short int earthHumControl = 0;
unsigned short int readDone = 0;
unsigned short int needIrrigation = 0;

// Variabili temporali
long int time = 0;
long int previousHumTime = 0;
long int afterHumTime = 0;
long int automaticTime = 0;
long int autoTimeEarth = 0;
long int temp = 5000;
long int beforeIrrigationTime = 0;

// Bottoni modalità manuale e automatica
const int buttonAutomatic = 24;
const int buttonManual = 25;
short int manualMode = 0;
short int autoMode = 0;

// 5V Proveniente dal DigitalPin 6, per sensore YL-38
const int currentEarthSensor = 6;

// Pulsanti temperatura e umidità
const int buttonTemp = 9;
const int buttonHumidity = 7;
const int buttonIrrigation = 27;

// Flag pulsanti
short int flagTemp = 0;
short int flagHumidity = 0;
short int flagIrrigation = 0;
short int flagAutomatic = 0;
short int flagManual = 0;
short int generalFlag = 0;
short int flagAutoTime = 0;
short int flagEarth = 0;

// Stati pulsanti
int buttonStateHumidity = 0;
int buttonStateTemp = 0;
int buttonStateIrrigation = 0;
int buttonStateAutomatic = 0;
int buttonStateManual = 0;

// Pompa dell'acqua DC30B
const int pump = 26;

// Led di controllo per lo stato dei pulsanti (lato del contenitore)
const int ledManual = 30;
const int ledAutomatic = 31;
const int ledIrrigation = 32;

// Sezione Setup
void setup()
{
  mySoftwareSerial.begin(9600);
  Serial.begin(115200);
  myDFPlayer.begin(mySoftwareSerial);
  myDFPlayer.volume(27); // up to 30
  lcd.begin(16, 2);
  pinMode(ledManual, OUTPUT);
  pinMode(ledAutomatic, OUTPUT);
  pinMode(ledIrrigation, OUTPUT);
  pinMode(buttonTemp, INPUT);
  pinMode(buttonHumidity, INPUT);
  pinMode(A0, INPUT);
  pinMode(currentEarthSensor, OUTPUT);
  pinMode(pump, OUTPUT);
  lcd.print("ACCENSIONE..");
  myDFPlayer.play(1);
  delay(2000);
  lcd.clear();
  lcd.print("SELEZIONA MOD. !");
  myDFPlayer.play(11);
}

void loop()
{

  // Lettura stato bottoni
  buttonStateTemp = digitalRead(buttonTemp);
  buttonStateHumidity = digitalRead(buttonHumidity);
  buttonStateIrrigation = digitalRead(buttonIrrigation);
  buttonStateAutomatic = digitalRead(buttonAutomatic);
  buttonStateManual = digitalRead(buttonManual);

  // Controllo bottoni manuali se non è attiva la modalità manuale
  if (manualMode == 0 && generalFlag == 0)
  {
    if (buttonStateTemp == HIGH || buttonStateHumidity == HIGH || buttonStateIrrigation == HIGH)
    {
      lcd.clear();
      lcd.print("SELEZIONA MOD. !");
      myDFPlayer.play(11);
      generalFlag = 1;
    }
  }

  // Controllo stati bottoni per stampa di un singolo messaggio 'Select mode' (per non farlo ripetere)
  if (buttonStateTemp == LOW && buttonStateHumidity == LOW)
    generalFlag = 0;

  // CONTROLLO BOTTONI MODALITA' SERRA

  // Attivazione Modalità Manuale
  if (buttonStateManual == HIGH)
  {
    if (flagManual == 0 && busy == 0)
    {
      myDFPlayer.play(5);
      lcd.clear();
      lcd.print("Mod. Manuale ON");
      flagManual = 1;
      autoMode = 0;
      manualMode = 1;
      digitalWrite(ledManual, HIGH);
      digitalWrite(ledAutomatic, LOW);
    }
  }

  // Attivazione Modalità Automatica
  if (buttonStateAutomatic == HIGH)
  {
    if (flagAutomatic == 0 && busy == 0)
    {
      automaticTime = millis();
      autoTimeEarth = millis();
      myDFPlayer.play(6);
      lcd.clear();
      lcd.print("Mod. Auto ON");
      flagAutomatic = 1;
      manualMode = 0;
      autoMode = 1;
      flagEarth = 0;
      digitalWrite(ledAutomatic, HIGH);
      digitalWrite(ledManual, LOW);
    }
  }

  // Controllo flag bottoni automatico e manuale
  if (buttonStateManual == LOW)
    flagManual = 0;
  if (buttonStateAutomatic == LOW)
    flagAutomatic = 0;

  // MODALITA' MANUALE
  if (manualMode == 1)
  {

    // Premi il bottone per la temperatura -> Diventa occupato e stampi il busy
    if (buttonStateTemp == HIGH)
    {
      if (flagTemp == 0 && busy == 0)
      {
        myDFPlayer.play(3);
        temperatura = dht.readTemperature();
        flagTemp = 1;
        requestTemp = 1;

        // Eventuale controllo per chi preme più bottoni insieme
        digitalWrite(pump, LOW);
        flagIrrigation = 0;

        busy = 1;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Occupato...");
        time = millis();
      }
    }

    // Premi il bottone per l'umidità -> Diventa occupato e stampi il busy
    if (buttonStateHumidity == HIGH)
    {
      if (flagHumidity == 0 && busy == 0)
      {
        myDFPlayer.play(4);

        // Accensione sensore umidità terreno
        digitalWrite(currentEarthSensor, HIGH);
        previousHumTime = millis();
        earthHumControl = 1;

        // Eventuale controllo per chi preme più bottoni insieme
        digitalWrite(pump, LOW);
        flagIrrigation = 0;

        // Lettura sensore umidità dell'aria
        humidity = dht.readHumidity();
        flagHumidity = 1;
        requestHum = 1;
        busy = 1;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Occupato...");
        time = millis();
      }
    }

    if (busy == 0)
    {
      if ((buttonStateIrrigation == HIGH) && (flagIrrigation == 0))
      {
        digitalWrite(pump, HIGH);
        flagIrrigation = 1;
        lcd.clear();
        lcd.print("Irrigazione!");
        digitalWrite(ledIrrigation, HIGH);
      }
    }

    // Se il bottone della pompa viene rilasciato
    if ((buttonStateIrrigation == LOW) && (flagIrrigation == 1) && (busy == 0))
    {
      digitalWrite(pump, LOW);
      flagIrrigation = 0;
      lcd.clear();
      lcd.print("Fine irrigazione");
      digitalWrite(ledIrrigation, LOW);
    }

    // Se i bottoni non sono premuti
    if (buttonStateTemp == LOW)
      flagTemp = 0;
    if (buttonStateHumidity == LOW)
      flagHumidity = 0;

    // Accensione e spegnimento controllato della schedina, per il sensore della terra (per evitare la corrosione)
    if ((earthHumControl == 1) && (millis() >= previousHumTime + 50))
    {
      afterHumTime = millis();
      earthHumidity = analogRead(A0);
      earthHumControl = 0;
      readDone = 1;
    }

    // Attesa di 10 millisecondi dopo aver effettuato la misura
    if ((readDone == 1) && (millis() >= afterHumTime + 10))
    {
      // Spegnimento del sensore
      digitalWrite(currentEarthSensor, LOW);
      earthHumControl = 0;
      readDone = 0;
    }

    // Aggiornamento del time e controllo di disponibilità al calcolo temp e humidity
    if ((busy == 1) && (millis() >= time + 2000))
    {
      time = millis();
      busy = 0;

      // Se è stata fatta richiesta di calcolo temperatura
      if (requestTemp == 1)
      {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Temp. Aria: ");
        lcd.setCursor(0, 1);
        lcd.print(temperatura);
        lcd.print((char)223);
        lcd.print("C");
        requestTemp = 0;
      }

      // Se è stata fatta richiesta di calcolo umidità
      if (requestHum == 1)
      {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Air Humid: ");
        lcd.print(humidity);
        lcd.print("%");
        lcd.setCursor(0, 1);

        // Controllo umidità e stato del terreno
        if (earthHumidity >= 1000)
        {
          lcd.print("SENSORE NON COL.");
          myDFPlayer.play(7);
        }
        if (earthHumidity < 1000 && earthHumidity >= 600)
        {
          lcd.print("TERRENO ASCIUTTO");
          myDFPlayer.play(8);
        }
        if (earthHumidity < 600 && earthHumidity >= 370)
        {
          lcd.print("TERRENO UMIDO   ");
          myDFPlayer.play(9);
        }
        if (earthHumidity < 370)
        {
          lcd.print("SENSORE IN ACQUA");
          myDFPlayer.play(10);
        }
        requestHum = 0;
      }
    }
  }

  // MODALITA' AUTOMATICA
  else if (autoMode == 1)
  {

    if (millis() >= automaticTime + 3000)
    {

      // Stampa messaggio iniziale di calcolo del sensore della terra
      if (flagEarth == 0)
      {
        lcd.setCursor(0, 1);
        lcd.print("CALCOLO DATI..");
      }

      // Calcolo sensori, tranne per sensore terra
      lcd.setCursor(0, 0);
      humidity = dht.readHumidity();
      temperatura = dht.readTemperature();
      lcd.print("Hum:");
      lcd.print(humidity);
      lcd.print("%");
      lcd.print("   T:");
      lcd.print(temperatura);
      lcd.print((char)223);
      lcd.print("C");
      automaticTime = millis();
    }

    // Accensione e spegnimento controllato della schedina per il sensore della terra, per evitare la corrosione
    if ((earthHumControl == 1) && (millis() >= previousHumTime + 50))
    {
      afterHumTime = millis();
      earthHumidity = analogRead(A0);
      earthHumControl = 0;
      readDone = 1;
      flagEarth = 1;
    }
    if ((readDone == 1) && (millis() >= afterHumTime + 10))
    {
      digitalWrite(currentEarthSensor, LOW);
      earthHumControl = 0;
      readDone = 0;

      // Il sensore si trova nell'aria, non nel terreno. Vuol dire che il sensore si è mosso e non si trova più posizionato nella terra.
      if (earthHumidity >= 1000)
      {
        lcd.print("SENSORE NON COL.");
        myDFPlayer.play(7);
        temp = 20000;
      }

      // Se la terra è secca c'è bisogno di irrigare il campo, e quindi l'elettrovalvola bisogna che sia attivata
      if (earthHumidity < 1000 && earthHumidity >= 600)
      {
        lcd.print("TERRENO ASCIUTTO");
        myDFPlayer.play(8);
        needIrrigation = 1;
        // Se il terreno è asciutto, il ciclo di controllo scende a 5 secondi
        temp = 5000;
        beforeIrrigationTime = millis();
        digitalWrite(pump, HIGH);
        digitalWrite(ledIrrigation, HIGH);
      }

      // La terra è umida
      if (earthHumidity < 600 && earthHumidity >= 370)
      {
        lcd.print("TERRENO UMIDO   ");
        myDFPlayer.play(9);
        temp = 25000;
      }

      // Il sensore si trova immerso nell'acqua. Condizione di irrigazione eccessiva, quindi il ciclo di controllo sale a 30 secondi.
      if (earthHumidity < 370)
      {
        lcd.print("SENSORE IN ACQUA");
        myDFPlayer.play(10);
        temp = 30000;
      }
    }

    // Se passa il tempo prefissato in base allo stato del terreno, accendi il sensore del terreno per poi spegnerlo (per evitare la corrosione del sensore).
    if (millis() >= autoTimeEarth + temp)
    {
      // Calcolo sensore terra
      lcd.setCursor(0, 1);
      digitalWrite(currentEarthSensor, HIGH);
      previousHumTime = millis();
      earthHumControl = 1;
      autoTimeEarth = millis();
    }

    // Irrigazione del terreno per 1 secondo nel caso che il terreno sia asciutto
    if ((needIrrigation == 1) && (millis() >= beforeIrrigationTime + 1000))
    {
      digitalWrite(pump, LOW);
      digitalWrite(ledIrrigation, LOW);
      needIrrigation = 0;
      beforeIrrigationTime = millis();
    }
  }
}
