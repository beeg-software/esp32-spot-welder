#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>


// FreeRTOS Configuration
#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

// Define OLED Pins
#define OLED_MOSI     23
#define OLED_CLK      18
#define OLED_DC       4
#define OLED_CS       5
#define OLED_RST      2

// Define rotary encoder pins
#define ENCODER_A 19
#define ENCODER_B 21
#define ENCODER_BTN 22

// Define spot welder specific constants and pins
#define RELAY_PIN 13
#define COMMAND_PIN 27
#define MIN_VALUE 10
#define MAX_VALUE 400
#define ADVANCE 5


// Declare FreeRTOS task functions and handlers
void TaskImpulse( void *pvParameters );
TaskHandle_t TaskImpulseHandle;
void TaskDrawMenu( void *pvParameters );
TaskHandle_t TaskDrawMenuHandle;
void TaskSerialCommands( void *pvParameters );
TaskHandle_t TaskSerialCommandsHandle;
void TaskEncoder( void *pvParameters );
TaskHandle_t TaskEncoderHandle;

// Declare normal functions
void preCalcTiming();
void serialFlush();
void menuLineOLED(int lineNum);
void printMenuLineOLED(String name, String value = "", bool selected = false, bool editMode = false);
void printOLED(String toPrint, bool inverted = false);
void printlnOLED(String toPrint, bool inverted = false);

// System status variables
String status = "BOOTING";

// Spot welder specific variables
String valuesNames[3] = {"Pre-impulso", "Pausa", "Impulso"};
int values[3] = {30, 10, 130};
int timings[3] = {25, 5, 125};

// Menu variables
int selectedValue = 0;
bool editing = false;
bool menuBlinkState = false;

// Encoder variables
int encoderPos = 0;
int lastEncA = LOW;
int lastEncB = LOW;

// Create the OLED display object
Adafruit_SH1106G display = Adafruit_SH1106G(128, 64,OLED_MOSI, OLED_CLK, OLED_DC, OLED_RST, OLED_CS);

void setup() {
  // Start Serial
  Serial.begin(9600);

  //Starting the four main tasks
  xTaskCreatePinnedToCore(
    TaskImpulse
    ,  "TaskImpulse"
    ,  2048  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  3  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  &TaskImpulseHandle 
    ,  ARDUINO_RUNNING_CORE);

  xTaskCreatePinnedToCore(
    TaskDrawMenu
    ,  "TaskDrawMenu"
    ,  2048
    ,  NULL
    ,  2
    ,  &TaskDrawMenuHandle 
    ,  ARDUINO_RUNNING_CORE);

  xTaskCreatePinnedToCore(
    TaskSerialCommands
    ,  "TaskSerialCommands"
    ,  2048
    ,  NULL
    ,  1
    ,  &TaskSerialCommandsHandle 
    ,  ARDUINO_RUNNING_CORE);

  xTaskCreatePinnedToCore(
    TaskEncoder
    ,  "TaskEncoder"
    ,  2048
    ,  NULL
    ,  1
    ,  &TaskEncoderHandle 
    ,  ARDUINO_RUNNING_CORE);
}

// Empty main loop because... FreeRTOS
void loop() {
}

// The task to handle the actual zapping
void TaskImpulse(void *pvParameters)
{
  (void) pvParameters;

  // Wait one second to be sure everything is ready to be started
  vTaskDelay(1000 / portTICK_PERIOD_MS );

  // Check for strange bootup statuses and stop everything if needed
  if(status != "BOOTING") {
    vTaskDelete(TaskImpulseHandle);
    status == "ERROR";
  } else {
    // Enable Relay pin output
    pinMode(RELAY_PIN, OUTPUT);
  }

  // Infinite task loop
  for (;;) {
    // Check if we're entering for the first time
    if(status == "BOOTING") {
      // Everything ready, suspend the task to wait for a zapp command
      status = "READY";
      vTaskSuspend(TaskImpulseHandle);
      status == "ERROR";
    } else if(status == "READY") {
      // This is a real zapp command, pre-calculate timing values
      preCalcTiming();
      status = "ZAPP";
      //Pre-impulse routine
      digitalWrite(RELAY_PIN, HIGH);
      vTaskDelay(timings[0] / portTICK_PERIOD_MS );
      digitalWrite(RELAY_PIN, LOW);
      vTaskDelay(ADVANCE / portTICK_PERIOD_MS );

      //Pause routine (can be simplified and merged up or down, keep for now)
      vTaskDelay(timings[1] / portTICK_PERIOD_MS );
      vTaskDelay(ADVANCE / portTICK_PERIOD_MS );

      //Impulse routine
      digitalWrite(RELAY_PIN, HIGH);
      vTaskDelay(timings[2] / portTICK_PERIOD_MS );
      digitalWrite(RELAY_PIN, LOW);
      vTaskDelay(ADVANCE / portTICK_PERIOD_MS );

      // Wait 5 seconds of cool-down (to avoid multiple firings and real overheating)
      status = "COOLING";
      vTaskDelay(5000 / portTICK_PERIOD_MS );
      status = "READY";

      // Goodnight, see you next zapp
      vTaskSuspend(TaskImpulseHandle);
    }
  }
}

void TaskDrawMenu(void *pvParameters)  // This is a task.
{
  (void) pvParameters;

  // Start OLED
  display.begin(0, true);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);

  TickType_t previousMenuTick;
  TickType_t currentMenuTick;
  previousMenuTick = xTaskGetTickCount();

  for (;;) {
    currentMenuTick = xTaskGetTickCount();
    if(currentMenuTick - previousMenuTick >= 750 / portTICK_PERIOD_MS) {
        menuBlinkState = !menuBlinkState;
        previousMenuTick = currentMenuTick;
    }
    // 25-ish FPS screen update
    vTaskDelay(40 / portTICK_PERIOD_MS );

    display.clearDisplay();
    display.setCursor(0,0);

    // Header
    printlnOLED("SPOT WELDER " + status);
    printlnOLED("");

    // Pre-impulse entry
    menuLineOLED(0);

    // Pause entry
    menuLineOLED(1);

    // Impulse entry
    menuLineOLED(2);

    display.display();
  }
}

void TaskSerialCommands(void *pvParameters)  // This is a task.
{
  (void) pvParameters;

  for (;;) {
    // controlla se ci sono nuovi dati in arrivo dalla seriale
    if (Serial.available() > 0) {
      // leggi il valore inviato tramite seriale
      char val = Serial.read();
      // se il valore è un numero da 1 a 3, seleziona il valore corrispondente
      if (val >= '1' && val <= '3') {
        selectedValue = val - '1';
        editing = false;
      }
      // se il valore è "u", incrementa il valore selezionato di 10
      if (val == 'i') {
        values[selectedValue] += 10;
        values[selectedValue] = constrain(values[selectedValue], MIN_VALUE, MAX_VALUE);
      }
      // se il valore è "d", decrementa il valore selezionato di 10
      if (val == 'd') {
        values[selectedValue] -= 10;
        values[selectedValue] = constrain(values[selectedValue], MIN_VALUE, MAX_VALUE);
      }
      if (val == 'e') {
        editing = !editing;
      }
      if (val == '!') {
        vTaskResume(TaskImpulseHandle);
      }
      serialFlush();
    }
  }
}

// Task to handle the menu navigation using the encoder
void TaskEncoder(void *pvParameters)
{
  (void) pvParameters;
  pinMode(COMMAND_PIN, INPUT);

  // Infinite task loop
  for (;;) {
    // leggi l'input dell'encoder
    int encA = digitalRead(ENCODER_A);
    int encB = digitalRead(ENCODER_B);
    int encBtn = digitalRead(ENCODER_BTN);

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

    lastEncA = encA;
    lastEncB = encB;
  }
}

// Function to pre-calculate timings from current values and default
// advance to switch the relaypin in time for the next 0-crossing of the
// mains voltage
void preCalcTiming() {
  timings[0] = values[0] - ADVANCE;
  timings[1] = values[1] - ADVANCE;
  timings[2] = values[2] - ADVANCE;
}

// Function to flush serial buffer
void serialFlush() {
  char t = 0;
  while(Serial.available() > 0) {
    t = Serial.read();
  }
}

// Function to handle a the printing process of a single line of the menu
void menuLineOLED(int lineNum) {
  if(lineNum == selectedValue) {
    if(editing && menuBlinkState) {
      printMenuLineOLED(valuesNames[lineNum], (String)values[lineNum], true, true);
    } else {
      printMenuLineOLED(valuesNames[lineNum], (String)values[lineNum], true, false);
    }
  } else {
    printMenuLineOLED(valuesNames[lineNum], (String)values[lineNum], false, false);
  }
}

// Function to assemble and print a single line of the menu
void printMenuLineOLED(String name, String value, bool selected, bool editMode) {
    if (selected) {
      printOLED(" " + name + ":", true);
    } else {
      printOLED(" " + name + ":");
    }

    printOLED(" ");

    if (selected && editMode) {
      printlnOLED(value, true);
    } else {
      printlnOLED(value);
    }
}

// Function to print string on OLED without minding changing colors
void printOLED(String toPrint, bool inverted) {
  if(inverted) {
    display.setTextColor(SH110X_BLACK, SH110X_WHITE);
  } else {
    display.setTextColor(SH110X_WHITE);
  }
  display.print(toPrint);
}

// Function to print string on OLED without minding changing colors (new line)
void printlnOLED(String toPrint, bool inverted) {
  if(inverted) {
    display.setTextColor(SH110X_BLACK, SH110X_WHITE);
  } else {
    display.setTextColor(SH110X_WHITE);
  }
  display.println(toPrint);
}