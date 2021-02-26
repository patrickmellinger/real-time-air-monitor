#include "PMS.h"
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <AltSoftSerial.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"

// PMS7003M config
AltSoftSerial soft_Serial; 
PMS pms(soft_Serial);
PMS::DATA data;

// BME680 config + init
const double SEALEVELPRESSURE_HPA = 1019.98;
Adafruit_BME680 bme;

// screen config + init
const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 128;
const int DC_PIN = 4; 
const int CS_PIN = 10;
const int RST_PIN = 6;
Adafruit_SSD1351 oled = Adafruit_SSD1351(SCREEN_WIDTH,SCREEN_HEIGHT,&SPI,CS_PIN,DC_PIN,RST_PIN);

// color config
const int BLACK = 0x0000;
const int BLUE = 0x001F;
const int RED = 0xF800;
const int GREEN = 0x07E0;
const int CYAN = 0x07FF;
const int YELLOW = 0xFFE0; 
const int WHITE = 0xFFFF; // unused  
const int MAGENTA = 0xF81F; // unused  

// UI config
const int xStart = 3;
const int dividerX = 52;
const int dividerY = 78;

// set default warn and alert states
boolean VOCWARNSTATE = false;
boolean PMONEWARNSTATE = false;
boolean PMTWOWARNSTATE = false;
boolean PMTENWARNSTATE = false;
boolean VOCALERTSTATE = false;
boolean PMONEALERTSTATE = false;
boolean PMTWOALERTSTATE = false;
boolean PMTENALERTSTATE = false;

void setup()
{
  // hardware init
  Serial.begin(9600);   
  soft_Serial.begin(9600);  // for PMS7003 sensor
  oled.begin();
  oled.setRotation(2);
  if (!bme.begin()) {
    Serial.println("Could not find a valid BME680 sensor, check wiring!");
    while (1);
  }
  
  // bootstrap sequence theater
  oled.fillScreen(BLACK); 
  oled.setTextColor(CYAN, BLACK);
  oled.setTextWrap(false);
  oled.setCursor(0,0);
  oled.setTextSize(1);
  oled.println("Begin initialization");
  oled.println(); 
  delay(500);
  oled.println("Warming up sensors"); 
  oled.println();
  delay(500);
  oled.println("Device more accurate"); 
  oled.println("after 10 minutes"); 
  oled.println();
  delay(2500);

  // configure BME defaults
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320,150); // 320*C for 150 ms

  // clear init screen
  oled.fillScreen(BLACK); 
}

void loop()
{
  // start by gathering data
  if (pms.read(data)){
    if (! bme.performReading()) {
      Serial.println("Failed to perform reading :(");
      return;
    }

    // sensor readings
    int vocInput = bme.gas_resistance / 1000.0;
    int pmOneInput = data.PM_AE_UG_1_0; 
    int pmTwoInput = data.PM_AE_UG_2_5;
    int pmTenInput = data.PM_AE_UG_10_0;
    int pressureInput = bme.pressure / 100.0;
    int humidityInput = bme.humidity;
    int altitudeInput = bme.readAltitude(SEALEVELPRESSURE_HPA);
    double tempInput = bme.temperature;

    // check status and decide border color
    if (!VOCALERTSTATE && !PMONEALERTSTATE && !PMTWOALERTSTATE && !PMTENALERTSTATE) {
      if (!VOCWARNSTATE && !PMONEWARNSTATE && !PMTWOWARNSTATE && !PMTENWARNSTATE) {
        printBorders(GREEN);    
      } else {
        printBorders(YELLOW);
      }      
    } else {
      printBorders(RED);
    }

    // output block
    printVOCs(vocInput);
    printPMOne(pmOneInput);
    printPMTwo(pmTwoInput);    
    printPMTen(pmTenInput);
    // horizontal divider goes here
    printTemp(tempInput);
    printPressure(pressureInput);
    printHumidity(humidityInput);
    printAltitude(altitudeInput);
  }
}

// this prints the line of specified line of text, after concatenation, in the specified color
void printLinePlus(int y, String prefix, int value, String units, int textColor) {
  int columnTwoX = dividerX + 3;
  String suffix = value + units;

  oled.setTextColor(textColor, BLACK);
  
  oled.setCursor(xStart,y);
  oled.print(prefix);
  oled.setCursor(columnTwoX,y);
  oled.println(suffix);
}

// draws the borders, change values to move position of border lines
void printBorders(int color){
  oled.drawFastHLine(0,0,127,color);
  oled.drawFastVLine(0,0,127,color);
  oled.drawFastVLine(127,0,127,color);
  oled.drawFastHLine(0,127,127,color);
  oled.drawFastVLine(dividerX,0,dividerY,color);
  oled.drawFastHLine(0,dividerY,127,color);
  oled.drawFastHLine(0,39,127,color);
}

// I use if/else blocks instead of switch/case blocks for VOCs and the particulate matter readings
// because of the way they scale and the sensor min/max values. 
// for all of the below the limits are suggestions and not medical advice or scientifically valid data

// builds the VOC string and decides what color the output should be based on the sensor readings
void printVOCs(int vocInput) {
  int textColor = GREEN;
  String vocPref = "VOCs    ";
  String vocUnits = " KOhms ";
  
  if (vocInput >= 120)  {
    VOCALERTSTATE = false;
    VOCWARNSTATE = false;
  } else if (vocInput < 120) {
    VOCALERTSTATE = false;
    VOCWARNSTATE = true;
    textColor = YELLOW;    
  } else if (vocInput < 50) {
    VOCALERTSTATE = true;
    textColor = RED;
  }

  printLinePlus(3,vocPref,vocInput,vocUnits,textColor);
}

// builds the PM 1.0 string and decides what color the output should be based on the sensor readings
void printPMOne(int pmOneInput) {
  int textColor = GREEN;
  String pmOnePref = "PM 1.0  ";
  String pmOneUnits = " ug/m3 ";

  if (pmOneInput <= 20) {
    PMONEALERTSTATE = false;
    PMONEWARNSTATE = false;
  } else if (pmOneInput > 20 && pmOneInput <= 50) {
    PMONEALERTSTATE = false;
    PMONEWARNSTATE = true;
    textColor = YELLOW;      
  } else if (pmOneInput > 50) {
    PMONEALERTSTATE = true;
    textColor = RED;
  }
  
  printLinePlus(12,pmOnePref,pmOneInput,pmOneUnits,textColor);
}

// builds the PM 2.5 string and decides what color the output should be based on the sensor readings
void printPMTwo(int pmTwoInput) {
  int textColor = GREEN;
  String pmTwoPref = "PM 2.5  ";
  String pmTwoUnits = " ug/m3 ";

  if (pmTwoInput <= 36) {
    PMTWOALERTSTATE = false;
    PMTWOWARNSTATE = false;    
  } else if (pmTwoInput > 36 && pmTwoInput <= 75 ) {
    PMTWOALERTSTATE = false;
    PMTWOWARNSTATE = true;
    textColor = YELLOW;      
  } else if (pmTwoInput > 75) {
    PMTWOALERTSTATE = true;
    textColor = RED;    
  }
  
  printLinePlus(21,pmTwoPref,pmTwoInput,pmTwoUnits,textColor);  
}

// builds the PM 10.0 string and decides what color the output should be based on the sensor readings
void printPMTen(int pmTenInput) {
  int textColor = GREEN;
  String pmTenPref = "PM 10.0 ";
  String pmTenUnits = " ug/m3 ";

  if (pmTenInput <= 155) {
    PMTENALERTSTATE = false;
    PMTENWARNSTATE = false;     
  } else if (pmTenInput > 155 && pmTenInput <= 215) {
    PMTENALERTSTATE = false;
    PMTENWARNSTATE = true;
    textColor = YELLOW;        
  } else if (pmTenInput > 215) {
    PMTENALERTSTATE = true;
    textColor = RED;    
  }
  
  printLinePlus(30,pmTenPref,pmTenInput,pmTenUnits,textColor);
}

// builds the temp string and decides what color the output should be based on the sensor readings
void printTemp(double tempInput) {
  int textColor = GREEN;
  int tempC = tempInput;
  int tempF = (tempInput * 1.8) + 32;
  String tempPref = "Air Temp";
  String tempCUnit = " C/";
  String tempFUnit = " F ";
  String tempSuffix = tempCUnit + tempF + tempFUnit;

  switch(tempC) {
    case -40 ... 10:
      textColor = BLUE;
      break;
    case 27 ... 38:
      textColor = YELLOW;
      break;
    case 39 ... 85:
      textColor = RED;
      break;
  }
    
  printLinePlus(42,tempPref,tempC,tempSuffix,textColor);
}

// builds the pressure string and decides what color the output should be based on the sensor readings
void printPressure(int pressureInput) {
  int textColor = GREEN;
  String pressurePref = "Pressure";
  String pressureUnits = " hPa ";

  switch(pressureInput) {
    case 300 ... 870: 
      textColor = RED;
      break;
    case 871 ... 943: 
      textColor = YELLOW;
      break;
    case 1049 ... 1084: 
      textColor = YELLOW;
      break;
    case 1085 ... 1100: 
      textColor = RED;
      break;
  }
  
  printLinePlus(51,pressurePref,pressureInput,pressureUnits,textColor);  
}

// builds the humidity string and decides what color the output should be based on the sensor readings
void printHumidity(int humidityInput) {
  int textColor = GREEN;
  String humidityPref = "Humidity";
  String humidityUnits = " % ";

  switch(humidityInput) {
    case 0 ... 19:
      textColor = RED;
      break;
    case 20 ... 29:
      textColor = YELLOW;
      break;
    case 70 ... 79:
      textColor = CYAN;
      break;
    case 80 ... 100:
      textColor = BLUE;
      break;
  }

  printLinePlus(60,humidityPref,humidityInput,humidityUnits,textColor);
}

// builds the altitude string and decides what color the output should be based on the sensor readings
void printAltitude(int altitudeInput) {
  String altitudePref = "Altitude";
  String altitudeUnits = " m ";
  
  printLinePlus(69,altitudePref,altitudeInput,altitudeUnits,GREEN);  
}