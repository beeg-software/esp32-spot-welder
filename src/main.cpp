#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>


// FREERTOS
#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

// OLED PINS
#define OLED_MOSI     23
#define OLED_CLK      18
#define OLED_DC       4
#define OLED_CS       5
#define OLED_RST      2

// ROTARTY ENCODER PINS
#define ENCODER_A 10
#define ENCODER_B 11
#define ENCODER_BTN 12

// SPOT WELDER RANGE
#define MIN_VALUE 10
#define MAX_VALUE 400


// FREERTOS Functions
void TaskMenu( void *pvParameters );


// SPOT WELDER VARIABLES
String valuesNames[3] = {"Pre-impulso", "Pausa", "Impulso"};
int values[3] = {30, 10, 130};

// MENU VARIABLES
int selectedValue = 0;
bool editing = false;

// ENCODER VARIABLES
int encoderPos = 0;
int lastEncA = LOW;
int lastEncB = LOW;


// Create the OLED display
Adafruit_SH1106G display = Adafruit_SH1106G(128, 64,OLED_MOSI, OLED_CLK, OLED_DC, OLED_RST, OLED_CS);

void setup() {
  // Start Serial
  Serial.begin(9600);

  // Now set up two tasks to run independently.
  xTaskCreatePinnedToCore(
    TaskMenu
    ,  "TaskMenu"   // A name just for humans
    ,  1024  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  2  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL 
    ,  ARDUINO_RUNNING_CORE);
}

void loop() {
}

void TaskMenu(void *pvParameters)  // This is a task.
{
  (void) pvParameters;

  // Start OLED
  display.begin(0, true);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);

  for (;;) {
    // leggi l'input dell'encoder
    int encA = digitalRead(ENCODER_A);
    int encB = digitalRead(ENCODER_B);
    int encBtn = digitalRead(ENCODER_BTN);

    // controlla se ci sono nuovi dati in arrivo dalla seriale
    if (Serial.available() > 0) {
      // leggi il valore inviato tramite seriale
      char val = Serial.read();
      // se il valore è un numero da 1 a 3, seleziona il valore corrispondente
      if (val >= '1' && val <= '3') {
        selectedValue = val - '1';
      }
      // se il valore è "u", incrementa il valore selezionato di 10
      if (val == 'u') {
        values[selectedValue] += 10;
        values[selectedValue] = constrain(values[selectedValue], MIN_VALUE, MAX_VALUE);
      }
      // se il valore è "d", decrementa il valore selezionato di 10
      if (val == 'd') {
        values[selectedValue] -= 10;
        values[selectedValue] = constrain(values[selectedValue], MIN_VALUE, MAX_VALUE);
      }
    }

    // se il pulsante dell'encoder viene premuto, cambia lo stato di editing
    if (encBtn == HIGH) {
      editing = !editing;
    }

    // se si è in modalità di editing, utilizza l'encoder per modificare il valore selezionato
    if (editing) {
      // aggiorna la posizione dell'encoder in base al segnale A e B
      if (encA != lastEncA || encB != lastEncB) {
        if (encA == LOW && encB == HIGH) {
          encoderPos--;
        }
        if (encA == HIGH && encB == LOW) {
          encoderPos++;
        }
        // aggiorna il valore selezionato in base alla posizione dell'encoder
        values[selectedValue] += encoderPos;
        values[selectedValue] = constrain(values[selectedValue], MIN_VALUE, MAX_VALUE);
        encoderPos = 0;
      }
    }
    // altrimenti, utilizza l'encoder per selezionare un valore
    else {
      // aggiorna la posizione dell'encoder in base al segnale A e B
      if (encA != lastEncA || encB != lastEncB) {
        if (encA == LOW && encB == HIGH) {
          encoderPos--;
        }
        if (encA == HIGH && encB == LOW) {
          encoderPos++;
        }
        // aggiorna il valore selezionato in base alla posizione dell'encoder
        selectedValue = constrain(encoderPos % 3, 0, 2);
        encoderPos = 0;
      }
    }

    // visualizza i valori sullo schermo OLED
    display.clearDisplay();
    display.setCursor(0,0);

    display.println("SPOT WELDER");
    display.println("");

    if (selectedValue == 0) {
      display.setTextColor(SH110X_BLACK, SH110X_WHITE);
    } else {
      display.setTextColor(SH110X_WHITE);
    }
    display.print(" ");
    display.print(valuesNames[0]);
    display.print(": ");
    if (selectedValue == 0 && editing) {
      display.setTextColor(SH110X_BLACK, SH110X_WHITE);
    } else {
      display.setTextColor(SH110X_WHITE);
    }
    display.println(values[0]);

    if (selectedValue == 1) {
      display.setTextColor(SH110X_BLACK, SH110X_WHITE);
    } else {
      display.setTextColor(SH110X_WHITE);
    }
    display.print(" ");
    display.print(valuesNames[1]);
    display.print(": ");
    if (selectedValue == 1 && editing) {
      display.setTextColor(SH110X_BLACK, SH110X_WHITE);
    } else {
      display.setTextColor(SH110X_WHITE);
    }
    display.println(values[1]);

    if (selectedValue == 2) {
      display.setTextColor(SH110X_BLACK, SH110X_WHITE);
    } else {
      display.setTextColor(SH110X_WHITE);
    }
    display.print(" ");
    display.print(valuesNames[2]);
    display.print(": ");
    if (selectedValue == 2 && editing) {
      display.setTextColor(SH110X_BLACK, SH110X_WHITE);
    } else {
      display.setTextColor(SH110X_WHITE);
    }
    display.println(values[2]);
    display.display();

    lastEncA = encA;
    lastEncB = encB;
  }
}