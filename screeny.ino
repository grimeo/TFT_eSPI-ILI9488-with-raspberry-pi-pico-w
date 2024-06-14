/*
    Pico to tft display
    TFT_MISO  0
    TFT_MOSI  3
    TFT_SCLK  2
    TFT_CS   20
    TFT_DC   22
    TFT_RST  21
    TOUCH_CS 14
*/

#include <FS.h>
#include <LittleFS.h>
#include <SPI.h>
#include <TFT_eSPI.h>

#include <TinyStepper_28BYJ_48.h>
#include <Wire.h>
#include "Adafruit_TCS34725.h"

#include <WiFi.h>
#include <HTTPClient.h>

#include "Icons.h"

#define CALIBRATION_FILE "/TouchCalData2"
#define FILESYSTEM LittleFS
#define REPEAT_CAL true

#define PRIMARY_FONT &FreeSansBold18pt7b
#define SECONDARY_FONT &FreeSansBold12pt7b
#define SECONDARY_NB_FONT &FreeSans12pt7b  // not bold
#define TERTIARY_FONT &FreeSansBold9pt7b
#define TERTIARY_NB_FONT &FreeSans9pt7b  // not bold

#define PRIME_COLOR 0x4ACF
#define SECONDARY_COLOR 0xFFFF

#define keydelay = 30;

// ================================== SCREENS =====================================
// * SCEENS are displayed in array form kase ayun ang sinusuggest                 *
// * na pattern ng library na gawa ni Bodmer Parang floor lang yung setup ng      *
// * screens.                                                                     *
// *                                                                              *
// *                                                                              *
// *  200 - Analyze                                                               *
// *    201 - Patient Information input                                           *
// *    202 - Parameters Btns                                                     *
// *    203 - Analyzing                                                           *
// *    204 - test stript not detected                                            *
// *    205 - Result Page 1                                                       *
// *    206 - Result Page 2                                                       *
// *    207 - Result Page 3                                                       *
// *    208 - Result Page 4                                                       *
// *    209 - sms sent                                                            *
// *                                                                              * 
// *  301 - Patient Record                                                        *
// *    302 - Result: Juan Dela Frooze                                            *
// *                                                                              *
// *   401 - Calibrating...                                                       *
// *    402 - Calibrated                                                          *
// *                                                                              *
// *  501 - Clean Option                                                          *
// *    502 - Eject                                                               *
// *    503 - Lock                                                                *
// *                                                                              *
// *  600 - keynoard                                                              *
// *                                                                              *
// ================================================================================

class Btn {
public:
  uint16_t btn_w;
  uint8_t btn_h;
  uint16_t btn_x;
  uint16_t btn_y;
  uint8_t radius;
  char btn_label;
  uint8_t btn_text_size;
  Btn(uint16_t w,
      uint8_t h,
      uint16_t x,
      uint16_t y,
      uint8_t r,
      char label,
      uint8_t text_size) {
    btn_w = w;
    btn_h = h;
    btn_x = x;
    btn_y = y;
    radius = r;
    btn_label = label;
    btn_text_size = text_size;
  }
  void init(uint16_t w,
            uint8_t h,
            uint16_t x,
            uint16_t y,
            uint8_t r,
            char label,
            uint8_t text_size) {
    btn_w = w;
    btn_h = h;
    btn_x = x;
    btn_y = y;
    radius = r;
    btn_label = label;
    btn_text_size = text_size;
  }
};

class ScreenWithHeader {
public:
  uint16_t header_w;
  uint8_t header_h;
  uint16_t header_x;
  uint16_t header_y;
  uint8_t text_size;
  char *header_label[];
  ScreenWithHeader(uint16_t w,
                   uint8_t h,
                   uint16_t x,
                   uint16_t y,
                   uint8_t t_size) {
    header_w = w;
    header_h = h;
    header_x = x;
    header_y = y;
    text_size = t_size;
  }
};

uint8_t screenLength = 13;
uint16_t SCREENS[] = { 100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
uint16_t CURRENT_SCREEN_ON_DISPLAY = 0;

// ScreenWithHeader cleanOption(header_w, header_h, header_x, header_y, text_sizem label);
ScreenWithHeader screenWithHeaderTemplate(480, 40, 0, 0, 1);

TFT_eSPI tft = TFT_eSPI();

// okay and back button
TFT_eSPI_Button Okay_Btn[1];
char *OkayBtnLabel[1] = { "Okay" };
char *BackBtnLabel[1] = { "Back" };
Btn OkayBtn(60, 35, 435, 285, 15, 'n', 1);

// test strips option btns
TFT_eSPI_Button testStripOption_Btn[5];
char *testStripOption_BtnLabel[5] = { "AI Analyze", "Save", "Reanalyze", "Back", "Send SMS" };
Btn testStripOptions_btns_Arr[5] = {
  Btn(100, 35, 67, 285, 15, 'n', 1),
  Btn(60, 35, 154, 285, 15, 'n', 1),
  Btn(100, 35, 241, 285, 15, 'n', 1),
  Btn(60, 35, 328, 285, 15, 'n', 1),
  Btn(100, 35, 415, 285, 15, 'n', 1)
};

// prev next btn
TFT_eSPI_Button PrevNext_Btns[2];
char *PrevNextLabel[2] = { "Prev", "Next" };
Btn prevNext_btns_Arr[2] = {
  // Btn(50, 35, 80, 240, 5, 'n', 1),
  // Btn(50, 35, 395, 240, 5, 'n', 1)
  Btn(70, 35, 140, 285, 5, 'n', 1),
  Btn(70, 35, 315, 285, 5, 'n', 1)
};

//100 Main menu
uint8_t mainScreenBtnCount = 4;
TFT_eSPI_Button mainScreenBtn[4];
char *mainMenu_BtnLabel[4] = { "ANALYZE", "PATIENT DATA", "CALIBRATE", "CLEAN" };
//btn(w,h,x,y,r,char,textsize, )
Btn mainMenu_Btn(320, 50, 240, 25, 15, 'n', 1);

// 201 analyze
char *patientInfoLabelHeader[1] = { "Patient Information" };
char *patientInfoInputLabel[4] = { "Name:", "Age:", "Gender:", "Mobile Number:" };

// =============================================================== your user data ===================================
String patientInfoInputValue[4] = { "", "", "", "" }; // change the value depends on input

TFT_eSPI_Button patientInfoInputBox[4];
Btn patientInfoInput_Box(220, 35, 315, 120, 1, 'n', 1);
TFT_eSPI_Button nextBtn[1];
char *nextBtnLabel[1] = { "Next" };

// 202
char *testStrip_BtnLabels[4] = { "URS - 4SG", "URS - 10T", "URS - 11T", "URS - 14" };
//btn(w,h,x,y,r,char,textsize, )
Btn testStrip_Btn(320, 40, 240, 25, 15, 'n', 1);
TFT_eSPI_Button testStripBtn[4];

char *parameterLabelHeader[1] = { "Parameter" };

// test strip not detected initialization
TFT_eSPI_Button testStripNotDetectedBtn[1];

// 300 Patient Data
char *AIAnalyzeHeaderLabel[1] = { "AI Analyze" };
char *AIAnalyzeResultParamsLabels[12] = {
  "Glucose: ",
  "Indications: ",
  "Prescription: ",
  "pH Level: ",
  "Indications: ",
  "Prescription: ",
  "Specific Gravity: ",
  "Indications: ",
  "Prescription: ",
  "Protein: ",
  "Indications: ",
  "Prescription: ",
};

// change the values of each index depends on the analyzed data of your AI
char *AIAnalyzeResultValuesLabels[12] = {
  "Trace",
  "Normal",
  "Normal",
  "Negative",
  "Normal",
  "-",
  "Negative",
  "Normal",
  "-",
  "Trace",
  "Normal",
  "Normal",
};

// 301
const uint16_t numberOfRecords = 150;  //change the value based on the number of patients
// PatientsRecordsInSpreadSheet real data from spreadsheet, declared below, just use it
char *patientRecodsNamesLabel[numberOfRecords];
// char *patientRecodsNamesLabel[numberOfRecords] = {
//   "Juan Dela Cruz",
//   "John Smith",
//   "Emily Johnson",
//   "Pepito Manaloto",
//   "Michael Brown",
//   "Sarah Davis",
//   "David Wilson",
//   "Jessica Martinez",
//   "Maria Garcia",
//   "Mark Reyes",
//   "Luz Fernandez",
//   "Jose Gonzales",
//   "Anna Santos",
//   "Pedro Hernandez",
//   "Angela Ramirez",
//   "Francisco Lopez",
//   "Carmen Torres",
//   "Ramon Cruz",
//   "Maricel Reyes",
//   "Ricardo Martinez",
//   "Karen Alvarez",
//   "Jorge Rodriguez",
//   "Marianne Del Rosario",
//   "Roberto Fernandez",
//   "Cristina Reyes",
//   "Raul Garcia",
//   "Maribel Santiago",
//   "Antonio Cruz",
//   "Lorna Lim",
//   "Dennis Reyes",
//   "Elena Gonzales",
//   "Gabriel Cruz",
//   "Melissa Santos",
//   "Diego Ramos",
//   "Leah Tan",
//   "Carlos Garcia"
// };

TFT_eSPI_Button PatientRecordsNames_Btn[numberOfRecords];  // create a method that will accepts fix size with constant value
uint16_t scrollPage = 1;
char *patientRecordHeader[1] = { "Patient Record" };
Btn PatientRecordsBtn(380, 35, 235, 20, 0, 'n', 1);
// Btn PatientRecordsBtn(380, 35, 235, 5, 0, 'n', 1);

//500
TFT_eSPI_Button CleanOption_Btn[2];
char *cleanLabelHeader[1] = { "Clean" };
char *cleanOptionLabel[2] = { "Eject", "Lock" };
Btn CleanOptionBtn(320, 50, 240, 25, 15, 'n', 1);

// parameter labels
char *_URS_4SG_ParamLabel[4] = { "GLUCOSE:", "pH LEVEL:", "SPECIFIC GRAVITY:", "PROTEIN:" };
char *_URS_10T_ParamLabel[10] = { "LEUKOCYTES:", "NITRITE:", "UROBILINOGEN:", "PROTEIN:", "pH LEVEL:", "BLOOD:", "SPECIFIC GRAVITY:", "KETONE:", "BILIRUBIN:", "GLUCOSE:" };
char *_URS_11T_ParamLabel[11] = { "LEUKOCYTES:", "NITRITE:", "UROBILINOGEN:", "PROTEIN:", "pH LEVEL:", "BLOOD:", "SPECIFIC GRAVITY:", "ASCORBATE:", "KETONE:", "BILIRUBIN:", "GLUCOSE:" };
char *_URS_14_ParamLabel[14] = { "UROBILINOGEN:", "BILIRUBIN:", "KETONE:", "CREATINE:", "BLOOD:", "PROTEIN:", "MICROALBUMIN:", "NITRITE:", "LEUKOCYTE:", "GLUCOSE:", "SPECIFIC GRAVITY:", "pH LEVEL:", "ASCOBRATE:", "CALCIUM:" };

// ### 
// edit the value base on your results; ex: for glucose, set the value of _URS_4SG_ParamValue[0] = "15"; treat it as string in multidimension array ; dont declare any string
char *_URS_4SG_ParamValue[4] = { "15", "7", "1.010", "Neg." };
char *_URS_10T_ParamValue[10] = { "Neg.", "Neg.", "3.2", "Neg.", "7", "10", "1.010", "0.5", "Neg.", "15" };
char *_URS_11T_ParamValue[11] = { "Neg.", "Neg.", "3.2", "Neg.", "7", "10", "1.010", "0.6", "0.5", "Neg.", "15" };
// char* _URS_14_ParamValue[14] = { "3.2", "Neg.", "0.5", "0.9", "10", "Neg.", "10", "Neg.", "Neg.", "15", "1.010", "7", "0.6", "2.5" };

char *ResultWithNameLabelHeader[1] = { "Result: " }; 
String name = ""; //for changing the value of name; name =  "patient name"

uint8_t currentTargetOnKeyboardInput = 0; //  change depends on the target input of user. ex: name in patient information to store the value in the array below
// the target array is below which is already declread above in 201 declaration section
// {"name", "age", "gender", "mobile"}
// char *patientInfoInputValue[4] = { "", "", "", "" }; // change the value depends on input


// ================== keyboard declaration, do not edit ====================

TFT_eSPI_Button row1keys_btn[10];
TFT_eSPI_Button row2keys_btn[10];
TFT_eSPI_Button row3keys_btn[10];
TFT_eSPI_Button row4keys_btn[9];
TFT_eSPI_Button row5keys_btn[5];

char *row1keys[10] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"};
char *row2keys[10] = {"q", "w", "e", "r", "t", "y", "u", "i", "o", "p"};
char *row3keys[10] = {"a", "s", "d", "f", "g", "h", "j", "k", "l","<||"};
char *row4keys[9] = {"^", "z", "x", "c", "v", "b", "n", "m", "<|"};
char *row5keys[5] = {"'", ",", " ", ".", "||>"};

char *row2CAPSkeys[10] = {"Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P"};
char *row3CAPSkeys[10] = {"A", "S", "D", "F", "G", "H", "J", "K", "L", "<|"};
char *row4CAPSkeys[9] = {"^", "Z", "X", "C", "V", "B", "N", "M", "<"};

const uint8_t inputLen = 50;
char userInput[inputLen] = "";
uint8_t currentCaret = 0;  


bool keyboardOnCaps = false;

// =================== keyboard end ======================================


// ======================= http bs =============================

// 0 = 4sg 
// 1 = 10t
// 2 = 11t
// 3 = 14
uint8_t currentTesting;

// the index of array of user info for displaying urs14 results which is included in this PatientsRecordsInSpreadSheet array
uint8_t currentUserInfo;

// array for saving data in spreadsheet;
String PatientsRecordsInSpreadSheet[150][18];

bool isDoingHttpRequest = false;


//======================== / http bs ===========================

// ================= wifi ================================

const char *ssid = "Red"; 
const char *pass = "kissmuna";

WiFiMulti WiFiMulti;

// ================/ wifi =========================


//==================================== TCS

const int MOTOR_IN1_PIN = 6;
const int MOTOR_IN2_PIN = 7;
const int MOTOR_IN3_PIN = 8;
const int MOTOR_IN4_PIN = 9;

String patientInputValues[4];
String patientURSResult[14];

const int STEPS_PER_REVOLUTION = 2048;

TinyStepper_28BYJ_48 stepper;

Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_614MS, TCS34725_GAIN_1X);

// tcs cals
uint16_t calibrated_R, calibrated_G, calibrated_B;
byte RGBvals[14][3];

// reading results // URS 14
String readingResults[14] = {};

int pos=0;


//====================================




void setup() {
  Serial.begin(115200);
  // tft
  tft.init();

  // set to 3 kapag ayaw magcalibrate pero nakabaliktad itu sa setup q kaya baliw ang touch here, set to 1 para goods pero need ng calibrate
  tft.setRotation(3);

  tft.setSwapBytes(true);

  touch_calibrate();
  
  tft.fillScreen(TFT_BLACK);
  delay(500);

  
  stepper.connectToPins(MOTOR_IN1_PIN, MOTOR_IN2_PIN, MOTOR_IN3_PIN, MOTOR_IN4_PIN);

  stepper.setSpeedInStepsPerSecond(400);
  stepper.setAccelerationInStepsPerSecondPerSecond(512);

  Wire.begin();
  if (tcs.begin()) {
    Serial.println("Found sensor");
  } else {
    Serial.println("No TCS34725 found");
    while (1)
      ;
  }

  
  // wifi
  
  // while(!Serial) {;} 
  WiFi.mode(WIFI_STA);
  Serial.print("First attempt to connect to WiFi... ");
  WiFi.begin(ssid, pass);
  delay(5000);
  Serial.println(WiFi.status()); // results

  if (WiFi.status() != WL_CONNECTED){
    WiFiMulti.clearAPList();
    Serial.print("Second Attempt... ");
    WiFiMulti.addAP(ssid, pass);
    delay(5000);
    Serial.println(WiFiMulti.run()); // result
  }


  goLoadingScreen();
  delay(3000);

}

void loop() {
  
  /*
    The only process that should be always running are the checking of the display and touch.
  */

  // // // call the display
  if (CURRENT_SCREEN_ON_DISPLAY != getCurrentScreenOnArray()) {
    setNewScreenOnDisplay(getCurrentScreenOnArray());
    setTouchOnDisplay(getCurrentScreenOnArray());
    CURRENT_SCREEN_ON_DISPLAY = getCurrentScreenOnArray();
  }
   // call the touch of that display
  if (CURRENT_SCREEN_ON_DISPLAY == getCurrentScreenOnArray())setTouchOnDisplay(getCurrentScreenOnArray());
  


  delay(10); //debounce to stop the keys from pressing repeatedly
}


// ======================================================
// *                                                    *
// *              START OF SCREEN DISPLAYS              *
// *                                                    * 
// *     the codes below are just the screen displays.  *
// *     if you want to edit something on the display   *
// *     such as the with and height, this section is   *
// *     the answer                                     *
// *                                                    *
// ======================================================

// ========== start of test strips displays =============
/*
  note that this section does not need editing 
  if you want to change the values, just change the values of the array on the top
  to find it click ctrl + f and type '###'
*/

void display_URS_4SG_() {
  uint8_t y = 70;
  uint8_t labelY = 90;
  tft.setCursor(155, 110);
  tft.setTextFont(2);  // this is font 2 of the library; not declared;
  tft.setTextSize(1);
  tft.setTextColor(PRIME_COLOR, SECONDARY_COLOR);
  tft.setTextDatum(CC_DATUM);
  for (uint8_t i = 0; i < 4; i++) {
    tft.drawString(_URS_4SG_ParamLabel[i], 155, labelY),
      tft.drawString(_URS_4SG_ParamValue[i], 325, labelY),
      tft.drawRect(70, y, 170, 40, PRIME_COLOR);
    tft.drawRect(239, y, 170, 40, PRIME_COLOR);
    y = y + 39;
    labelY = labelY + 39;
    
  }
}

void display_URS_10T_() {
  uint8_t y = 77;
  uint8_t labelY = 93;
  tft.setCursor(155, 110);
  tft.setTextFont(2);  // this is font 2 of the library; not declared;
  tft.setTextSize(1);
  tft.setTextColor(PRIME_COLOR, SECONDARY_COLOR);
  tft.setTextDatum(CC_DATUM);
  for (uint8_t i = 0; i < 5; i++) {
    tft.drawString(_URS_10T_ParamLabel[i], 80, labelY);
    tft.drawString(_URS_10T_ParamValue[i], 189, labelY);
    tft.drawRect(18, y, 125, 30, PRIME_COLOR);
    tft.drawRect(142, y, 95, 30, PRIME_COLOR);

    tft.drawString(_URS_10T_ParamLabel[i + 5], 307, labelY);
    tft.drawString(_URS_10T_ParamValue[i + 5], 416, labelY);
    tft.drawRect(245, y, 125, 30, PRIME_COLOR);
    tft.drawRect(369, y, 95, 30, PRIME_COLOR);

    y = y + 29;
    labelY = labelY + 29;
  }
}

void display_URS_11T_() {
  uint8_t y = 67;
  uint8_t labelY = 83;
  tft.setCursor(155, 110);
  tft.setTextFont(2);  // this is font 2 of the library; not declared;
  tft.setTextSize(1);
  tft.setTextColor(PRIME_COLOR, SECONDARY_COLOR);
  tft.setTextDatum(CC_DATUM);
  for (uint8_t i = 0; i < 5; i++) {
    tft.drawString(_URS_11T_ParamLabel[i], 80, labelY);
    tft.drawString(_URS_11T_ParamValue[i], 189, labelY);
    tft.drawRect(18, y, 125, 30, PRIME_COLOR);
    tft.drawRect(142, y, 95, 30, PRIME_COLOR);

    tft.drawString(_URS_11T_ParamLabel[i + 5], 307, labelY);
    tft.drawString(_URS_11T_ParamValue[i + 5], 416, labelY);
    tft.drawRect(245, y, 125, 30, PRIME_COLOR);
    tft.drawRect(369, y, 95, 30, PRIME_COLOR);
    y = y + 29;
    labelY = labelY + 29;
  }
  tft.drawString(_URS_11T_ParamLabel[10], 307, labelY);
  tft.drawString(_URS_11T_ParamValue[10], 416, labelY);
  tft.drawRect(245, y, 125, 30, PRIME_COLOR);
  tft.drawRect(369, y, 95, 30, PRIME_COLOR);
}

void display_URS_14_() {
  uint8_t y = 52;
  uint8_t labelY = 68;
  tft.setTextFont(2);  // this is font 2 of the library; not declared;
  tft.setTextSize(1);
  tft.setTextColor(PRIME_COLOR, SECONDARY_COLOR);
  tft.setTextDatum(CC_DATUM);
  
  for (uint8_t i = 0; i < 7; i++) {
    tft.drawString(_URS_14_ParamLabel[i], 80, labelY);
    tft.drawString(readingResults[i], 189, labelY);
    tft.drawRect(18, y, 125, 30, PRIME_COLOR);
    tft.drawRect(142, y, 95, 30, PRIME_COLOR);

    tft.drawString(_URS_14_ParamLabel[i + 7], 307, labelY);
    tft.drawString(readingResults[i + 7], 416, labelY);
    tft.drawRect(245, y, 125, 30, PRIME_COLOR);
    tft.drawRect(369, y, 95, 30, PRIME_COLOR);
    y = y + 29;
    labelY = labelY + 29;
  }
}

void display_URS_14_from_spreadSheet(int id) {
  uint8_t y = 52;
  uint8_t labelY = 68;
  tft.setTextFont(2);  // this is font 2 of the library; not declared;
  tft.setTextSize(1);
  tft.setTextColor(PRIME_COLOR, SECONDARY_COLOR);
  tft.setTextDatum(CC_DATUM);
  
  // PatientsRecordsInSpreadSheet
  // currentUserInfo
  for (uint8_t i = 0; i < 7; i++) {
    tft.drawString(_URS_14_ParamLabel[i], 80, labelY);
    tft.drawString(PatientsRecordsInSpreadSheet[id][i + 4], 189, labelY);
    tft.drawRect(18, y, 125, 30, PRIME_COLOR);
    tft.drawRect(142, y, 95, 30, PRIME_COLOR);

    tft.drawString(_URS_14_ParamLabel[i + 7], 307, labelY);
    tft.drawString(PatientsRecordsInSpreadSheet[id][i + 7 + 4], 416, labelY);
    tft.drawRect(245, y, 125, 30, PRIME_COLOR);
    tft.drawRect(369, y, 95, 30, PRIME_COLOR);
    y = y + 29;
    labelY = labelY + 29;
  }
}


void displayResultBtns() {
  tft.setTextColor(PRIME_COLOR, SECONDARY_COLOR);
  tft.setFreeFont(TERTIARY_FONT);
  
  for (uint8_t btnCol = 0; btnCol < 5; btnCol++) {
    testStripOption_Btn[btnCol].initButton(&tft,
                                           testStripOptions_btns_Arr[btnCol].btn_x,  // X
                                           testStripOptions_btns_Arr[btnCol].btn_y,  // Y
                                           testStripOptions_btns_Arr[btnCol].btn_w,  // WIDTH
                                           testStripOptions_btns_Arr[btnCol].btn_h,  // HEIGHT
                                           PRIME_COLOR,                              // OUTLINE BORDER COLOR
                                           PRIME_COLOR,                              // FILL COLOR
                                           TFT_WHITE,                                // TEXT COLOR
                                           "",                                       // label na magolo
                                           1);                                       // TEXT SIZE

    // tft.setTextColor(PRIME_COLOR, SECONDARY_COLOR);
    testStripOption_Btn[btnCol].setLabelDatum(0, 0, CC_DATUM);
    //drawButton(bool inverted, String long_name )false, mainMenu_BtnLabel[btnCol]
    testStripOption_Btn[btnCol].press(false);
    testStripOption_Btn[btnCol].drawButton(false, testStripOption_BtnLabel[btnCol]);
  }
}

// =========== end of test strips displays ==============


// 205
void display_URS_4SG_Result(){
  displayScreenWithLeftHeaderTemplate(ResultWithNameLabelHeader);
  display_URS_4SG_();
  displayResultBtns();
}

// 206
void display_URS_10T_Result(){
  displayScreenWithLeftHeaderTemplate(ResultWithNameLabelHeader);
  display_URS_10T_();
  displayResultBtns();
}

// 207
void display_URS_11T_Result(){
  displayScreenWithLeftHeaderTemplate(ResultWithNameLabelHeader);
  display_URS_11T_();
  displayResultBtns();

}
// 208
void display_URS_14_Result(){
  displayScreenWithLeftHeaderTemplate(ResultWithNameLabelHeader);
  display_URS_14_();
  displayResultBtns();

}

// test stript not detected
void displayTestStriptNotDetected(){
  tft.fillScreen(TFT_BLACK);
    tft.setCursor(70, 180);
    tft.setFreeFont(SECONDARY_FONT);
    tft.setTextSize(1);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.println("TEST STRIP NOT DETECTED");
    testStripNotDetectedBtn[0].initButton(&tft,
                                        240,                 // X
                                        250,                 // Y
                                        190,                 // WIDTH
                                        40,                  // HEIGHT
                                        PRIME_COLOR,         // OUTLINE BORDER COLOR
                                        PRIME_COLOR,         // FILL COLOR
                                        TFT_WHITE,           // TEXT COLOR
                                        "",                  // label na magolo
                                        1);                  // TEXT SIZE
      testStripNotDetectedBtn[0].setLabelDatum(0, 2, CC_DATUM);
      tft.setFreeFont(TERTIARY_FONT);
      testStripNotDetectedBtn[0].press(false);
      testStripNotDetectedBtn[0].drawButton(false,"ANALYZE AGAIN");
}

// suppose to be a 203 but
// not in the array cuz it can stand alone 
void displayAnalyzing() {
  tft.fillScreen(SECONDARY_COLOR);
  tft.setCursor(175, 230);
  tft.setTextColor(PRIME_COLOR, SECONDARY_COLOR);
  tft.setFreeFont(SECONDARY_FONT);
  tft.pushImage(180, 80,  iconWidth,  iconHeight, analyzeIcon);
  tft.setTextSize(1);
  tft.print("Analyzing");
  // change this loop for analyzing mechanism of your device
  for (uint8_t i = 0; i < 3; i++) {
    tft.print(".");
    delay(1000);
  }
}

// 202 parameter option
void displayParameters(){
  displayScreenWithHeaderTemplate(parameterLabelHeader);
  tft.setTextColor(PRIME_COLOR, SECONDARY_COLOR);
  tft.setFreeFont(TERTIARY_FONT);
      uint16_t y = 80;
  for (uint8_t btnCol = 0; btnCol < 4; btnCol++) {
    testStripBtn[btnCol].initButton(&tft,
                                     testStrip_Btn.btn_x,  // X
                                     y,                   // Y
                                     testStrip_Btn.btn_w,  // WIDTH
                                     testStrip_Btn.btn_h,  // HEIGHT
                                     PRIME_COLOR,         // OUTLINE BORDER COLOR
                                     PRIME_COLOR,         // FILL COLOR
                                     TFT_WHITE,           // TEXT COLOR
                                     "",                  // label na magolo
                                     1);                  // TEXT SIZE
    // tft.setTextColor(PRIME_COLOR, SECONDARY_COLOR);
    testStripBtn[btnCol].setLabelDatum(0, 2, CC_DATUM);
    //drawButton(bool inverted, String long_name )false, mainMenu_BtnLabel[btnCol]
    testStripBtn[btnCol].press(false);
    testStripBtn[btnCol].drawButton(false, testStrip_BtnLabels[btnCol]);
      y = y + 50;
  }

  displayOkayBtn(Okay_Btn, OkayBtn, 0, BackBtnLabel);
}

// 201 input display
void displayPatientInformationInputField(){
  displayScreenWithHeaderTemplate(patientInfoLabelHeader);
  displayPatientInfoInputLabel();
  displayPatientInfoInputBox();
  displayNextBtn();
  displayOkayBtn(Okay_Btn, OkayBtn, 0, BackBtnLabel);
}

// 201
void displayPatientInfoInputLabel(){
  tft.setCursor(120, 100);
  tft.setTextDatum(MR_DATUM);
  tft.setTextColor(TFT_BLACK, SECONDARY_COLOR);
  uint16_t y  = 95;
  for (uint8_t btnCol = 0; btnCol < 4; btnCol++) {
  tft.setFreeFont(TERTIARY_FONT);
    tft.drawString(patientInfoInputLabel[btnCol], 180, y);
    y = y + 40;
  }
}

// 201
void displayPatientInfoInputBox(){
  tft.setCursor(120, 100);
  tft.setTextDatum(MR_DATUM);
  tft.setTextColor(TFT_BLACK, SECONDARY_COLOR);
  uint16_t y  = 95;
  for (uint8_t btnCol = 0; btnCol < 4; btnCol++) {
    tft.setFreeFont(TERTIARY_FONT);
    patientInfoInputBox[btnCol].initButton(&tft,
                                     patientInfoInput_Box.btn_x,  // X
                                     y,                   // Y
                                     patientInfoInput_Box.btn_w,  // WIDTH
                                     patientInfoInput_Box.btn_h,  // HEIGHT
                                     TFT_BLACK,         // OUTLINE BORDER COLOR
                                     SECONDARY_COLOR,         // FILL COLOR
                                     TFT_BLACK,           // TEXT COLOR 
                                     "",                  // label na magolo
                                     1);                  // TEXT SIZE
    patientInfoInputBox[btnCol].setLabelDatum(0, 2, CC_DATUM);
    patientInfoInputBox[btnCol].press(false);
    tft.setFreeFont(TERTIARY_NB_FONT);
    patientInfoInputBox[btnCol].drawButton(false, patientInfoInputValue[btnCol]);

    y = y + 40;
  }
}

// 201
void displayNextBtn(){
  tft.setFreeFont(TERTIARY_FONT);
  nextBtn[0].initButton(&tft,
                                    45,  // X
                                    290,  // Y
                                    60,  // WIDTH
                                    35,  // HEIGHT
                                    PRIME_COLOR,        // OUTLINE BORDER COLOR
                                    PRIME_COLOR,        // FILL COLOR
                                    TFT_WHITE,          // TEXT COLOR
                                    "",                 // label na magolo
                                    1);                 // TEXT SIZE

  nextBtn[0].setLabelDatum(0, 2, CC_DATUM);
  //drawButton(bool inverted, String long_name )
  nextBtn[0].drawButton(false, nextBtnLabel[0]);
}

// 301
void displayPatientRecord() {
  tft.setTextSize(1);
  displayScreenWithHeaderTemplate(patientRecordHeader);
  displayOkayBtn(Okay_Btn, OkayBtn, 0, BackBtnLabel);  
  displayRecordNames(); // call this again to display next page if (scrollpage is not equal to new scroll page)
  displayNextPrevBtn();
}

//  301 part
void displayRecordNames() {  // fix need

  tft.setTextColor(PRIME_COLOR, SECONDARY_COLOR);
  tft.setFreeFont(TERTIARY_FONT);

  uint16_t y = PatientRecordsBtn.btn_y + 70;

  // display 4 per page
  uint8_t scrollPageItems = scrollPage * 4;

  uint16_t scrollPageStart = scrollPageItems - 4;
  uint16_t scrollPageEnd = scrollPageItems;

  for (scrollPageStart; scrollPageStart < scrollPageEnd  ; scrollPageStart++) {
    PatientRecordsNames_Btn[scrollPageStart].initButton(&tft,
                                               PatientRecordsBtn.btn_x,  // X
                                               y,                        // Y
                                               PatientRecordsBtn.btn_w,  // WIDTH
                                               PatientRecordsBtn.btn_h,  // HEIGHT
                                               PRIME_COLOR,              // OUTLINE BORDER COLOR
                                               SECONDARY_COLOR,          // FILL COLOR
                                               PRIME_COLOR,              // TEXT COLOR
                                               "",                       // label na magolo
                                               1);                       // TEXT SIZE

    PatientRecordsNames_Btn[scrollPageStart].setLabelDatum(-(PatientRecordsBtn.btn_w / 2) + 10, 2, ML_DATUM);
    PatientRecordsNames_Btn[scrollPageStart].press(false);
    PatientRecordsNames_Btn[scrollPageStart].drawButton(false, PatientsRecordsInSpreadSheet[scrollPageStart][0]);
    y = y + 5 + PatientRecordsBtn.btn_h;
  }
}
// 
void displayNextPrevBtn(){

  uint8_t btnCol = 0;

  for (btnCol; btnCol < 2; btnCol++){
    PrevNext_Btns[btnCol].initButton(&tft,
                                                prevNext_btns_Arr[btnCol].btn_x,  // X
                                                prevNext_btns_Arr[btnCol].btn_y,  // Y
                                                prevNext_btns_Arr[btnCol].btn_w,  // WIDTH
                                                prevNext_btns_Arr[btnCol].btn_h,  // HEIGHT
                                                PRIME_COLOR,              // OUTLINE BORDER COLOR
                                                PRIME_COLOR,          // FILL COLOR
                                                SECONDARY_COLOR,              // TEXT COLOR
                                                "",                       // label na magolo
                                                1);                       // TEXT SIZE

    PrevNext_Btns[btnCol].setLabelDatum(0, 2, CC_DATUM);
    PrevNext_Btns[btnCol].drawButton(false, PrevNextLabel[btnCol]);
    PrevNext_Btns[btnCol].press(false);
  }
  
}

// 302 
void displayOnePatientRecord(int id) {
  displayScreenWithLeftHeaderTemplate(ResultWithNameLabelHeader);
  // display_URS_4SG_();
  // display_URS_10T_(); // depends on the test strip, change this if you want to display other strips
  // display_URS_11T_();
  // display_URS_14_();
  display_URS_14_from_spreadSheet(currentUserInfo);
  displayResultBtns();
}

// 303
void displayAIAnalyze() {
  displayScreenWithHeaderTemplate(AIAnalyzeHeaderLabel);
  displayOkayBtn(Okay_Btn, OkayBtn, 0, BackBtnLabel);  //0 kasi isa lang btn ng okay;
  uint8_t y = 52;
  uint8_t labelY = 68;
  tft.setTextFont(1);  // this is font 2 of the library; not declared;
  tft.setTextSize(1);
  tft.setTextColor(PRIME_COLOR, SECONDARY_COLOR);
  tft.setTextDatum(ML_DATUM);
  for (uint8_t i = 0; i < 12; i++) {
    tft.drawString(AIAnalyzeResultParamsLabels[i], 65, labelY);
    tft.drawString(AIAnalyzeResultValuesLabels[i], 174, labelY);
    if(i==2 || i==5 || i==8) labelY = labelY + 24;
    else labelY = labelY + 12;
  }
}

//304
void displaySMSSent(){
  tft.fillScreen(SECONDARY_COLOR);
  tft.setCursor(120, 230);
  tft.setTextColor(PRIME_COLOR, SECONDARY_COLOR);
  tft.setFreeFont(SECONDARY_FONT);
  tft.pushImage(180, 70,  iconWidth,  iconHeight, successIcon);
  tft.setTextSize(1);
  tft.print("SMS Sent Successful");
  displayOkayBtn(Okay_Btn, OkayBtn, 0, OkayBtnLabel);  //0 kasi isa lang btn ng okay;
}

// need heavy modification for calibration of the machine;
// Tips: use a function and call it inside. Use while or do while loop
// 401
void displayCalibrating() {
  tft.fillScreen(SECONDARY_COLOR);
  tft.setCursor(175, 230);
  tft.setTextColor(PRIME_COLOR, SECONDARY_COLOR);
  tft.setFreeFont(SECONDARY_FONT);
  tft.pushImage(180, 70,  iconWidth,  iconHeight, calibrateIcon);
  tft.setTextSize(1);
  tft.print("Calibrating");
  for (uint8_t i = 0; i < 3; i++) {
    tft.print(".");
    delay(1000);
  }

  calibrateTCS();

  addNewScreenOnArray(402);
}

// 402
void displayCalibrated() {
  tft.fillScreen(SECONDARY_COLOR);
  tft.setCursor(180, 230);
  tft.setTextColor(PRIME_COLOR, SECONDARY_COLOR);
  tft.setFreeFont(SECONDARY_FONT);
  tft.pushImage(180, 70,  iconWidth,  iconHeight, successIcon);
  tft.setTextSize(1);
  tft.print("Calibrated");
  displayOkayBtn(Okay_Btn, OkayBtn, 0, OkayBtnLabel);  //0 kasi isa lang btn ng okay;
}

// 503
void displayLocked() {
  tft.fillScreen(SECONDARY_COLOR);
  tft.setCursor(200, 230);
  tft.setTextColor(PRIME_COLOR, SECONDARY_COLOR);
  tft.pushImage(180, 70,  iconWidth,  iconHeight, successIcon);
  tft.setFreeFont(SECONDARY_FONT);
  tft.setTextSize(1);
  tft.print("Locked");
  displayOkayBtn(Okay_Btn, OkayBtn, 0, OkayBtnLabel);  //0 kasi isa lang btn ng okay;
}

// 5h
void displayEjected() {
  tft.fillScreen(SECONDARY_COLOR);
  tft.setCursor(200, 230);
  tft.setTextColor(PRIME_COLOR, SECONDARY_COLOR);
  tft.setFreeFont(SECONDARY_FONT);
  tft.pushImage(180, 70,  iconWidth,  iconHeight, successIcon);
  tft.setTextSize(1);
  tft.print("Ejected");
  displayOkayBtn(Okay_Btn, OkayBtn, 0, OkayBtnLabel);  //0 kasi isa lang btn ng okay;
}

//501 clean screen options
void displayCleanOptionBtns() {  //
  uint16_t y = 0;
  for (uint8_t btnCol = 0; btnCol < 2; btnCol++) {
    if (btnCol == 0) {
      y = CleanOptionBtn.btn_h + 80;
    } else {
      y = y + 20 + CleanOptionBtn.btn_h;
    }
    CleanOption_Btn[btnCol].initButton(&tft,
                                       CleanOptionBtn.btn_x,  // X
                                       y,                     // Y
                                       CleanOptionBtn.btn_w,  // WIDTH
                                       CleanOptionBtn.btn_h,  // HEIGHT
                                       PRIME_COLOR,           // OUTLINE BORDER COLOR
                                       PRIME_COLOR,           // FILL COLOR
                                       TFT_WHITE,             // TEXT COLOR
                                       "",                    // label na magolo
                                       1);                    // TEXT SIZE

    CleanOption_Btn[btnCol].setLabelDatum(0, 0, CC_DATUM);
    CleanOption_Btn[btnCol].press(false);
    CleanOption_Btn[btnCol].drawButton(false, cleanOptionLabel[btnCol]);
  }
}

// 500
void displayCleanOptions() {
  displayScreenWithHeaderTemplate(cleanLabelHeader);
  displayCleanOptionBtns();
  displayOkayBtn(Okay_Btn, OkayBtn, 0, BackBtnLabel);  //0 kasi isa lang btn ng okay;
}

void displayOkayBtn(TFT_eSPI_Button btnReference[], Btn btnInstance, uint8_t btnCount, char *btnLabels[]) {
  tft.setFreeFont(TERTIARY_FONT);
  btnReference[btnCount].initButton(&tft,
                                    btnInstance.btn_x,  // X
                                    btnInstance.btn_y,  // Y
                                    btnInstance.btn_w,  // WIDTH
                                    btnInstance.btn_h,  // HEIGHT
                                    PRIME_COLOR,        // OUTLINE BORDER COLOR
                                    PRIME_COLOR,        // FILL COLOR
                                    TFT_WHITE,          // TEXT COLOR
                                    "",                 // label na magolo
                                    1);                 // TEXT SIZE

  btnReference[btnCount].setLabelDatum(0, 2, CC_DATUM);
  //drawButton(bool inverted, String long_name )
  btnReference[btnCount].drawButton(false, btnLabels[btnCount]);
}

// 100 btns
void displayMainMenu() {
  tft.fillScreen(SECONDARY_COLOR);
  tft.setTextColor(PRIME_COLOR, SECONDARY_COLOR);
  tft.setFreeFont(TERTIARY_FONT);
  uint16_t y = mainMenu_Btn.btn_y;
  for (uint8_t btnCol = 0; btnCol < mainScreenBtnCount; btnCol++) {
    if (btnCol == 0) {
      y = mainMenu_Btn.btn_y + 40;
    } else {
      y = y + 10 + mainMenu_Btn.btn_h;
    }
    // x, y, w, h, outline, fill, text
    mainScreenBtn[btnCol].initButton(&tft,
                                     mainMenu_Btn.btn_x,  // X
                                     y,                   // Y
                                     mainMenu_Btn.btn_w,  // WIDTH
                                     mainMenu_Btn.btn_h,  // HEIGHT
                                     PRIME_COLOR,         // OUTLINE BORDER COLOR
                                     PRIME_COLOR,         // FILL COLOR
                                     TFT_WHITE,           // TEXT COLOR
                                     "",                  // label na magolo
                                     1);                  // TEXT SIZE

    // tft.setTextColor(PRIME_COLOR, SECONDARY_COLOR);
    mainScreenBtn[btnCol].setLabelDatum(0, 0, CC_DATUM);
    //drawButton(bool inverted, String long_name )false, mainMenu_BtnLabel[btnCol]
    mainScreenBtn[btnCol].press(false);
    mainScreenBtn[btnCol].drawButton(false, mainMenu_BtnLabel[btnCol]);
  }
}

// 600 keyboard
void displayKeyboard(){
  tft.fillScreen(SECONDARY_COLOR);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.drawRect(10, 25, 460, 45, PRIME_COLOR);
  tft.setFreeFont(SECONDARY_NB_FONT);
  tft.setTextDatum(TL_DATUM);
  displayKeys(); 
}

void displayKeys(){

  uint16_t x = 40;
  uint16_t y = 120;
  uint8_t w = 40;
  uint8_t h = 40 ;

    for(uint8_t i = 0; i < 10; i++){
      row1keys_btn[i].initButton(&tft,
                        x,
                        y,
                        w,
                        h,
                        PRIME_COLOR,           // OUTLINE BORDER COLOR
                        SECONDARY_COLOR,          // FILL COLOR
                        PRIME_COLOR,         // TEXT COLOR
                        row1keys[i],      // label na magolo
                        1);             // TEXT SIZE
                        
      row1keys_btn[i].setLabelDatum(0, 2, CC_DATUM);
      row1keys_btn[i].press(false);
      row1keys_btn[i].drawButton();
      x = x + 44;
    }
     x = 40;
    
    // displays the keyboard's case depending on the value of keyboardOnCaps
  if(keyboardOnCaps == false){
    for(uint8_t i = 0; i < 10; i++){
      row2keys_btn[i].initButton(&tft,
                        x,
                        y + 44,
                        w,
                        h,
                        PRIME_COLOR,           // OUTLINE BORDER COLOR
                        SECONDARY_COLOR,          // FILL COLOR
                        PRIME_COLOR,         // TEXT COLOR
                        row2keys[i],      // label na magolo
                        1);             // TEXT SIZE
                        
      row2keys_btn[i].setLabelDatum(0, 2, CC_DATUM);
      row2keys_btn[i].press(false);
      row2keys_btn[i].drawButton();
      x = x + 44;
    }
     x = 40; //65
    for(uint8_t i = 0; i < 10; i++){
      row3keys_btn[i].initButton(&tft,
                        x,
                        y + 88,
                        w,
                        h,
                        PRIME_COLOR,           // OUTLINE BORDER COLOR
                        SECONDARY_COLOR,          // FILL COLOR
                        PRIME_COLOR,         // TEXT COLOR
                        row3keys[i],      // label na magolo
                        1);             // TEXT SIZE
                        
      row3keys_btn[i].setLabelDatum(0, 2, CC_DATUM);
      row3keys_btn[i].press(false);
      row3keys_btn[i].drawButton();
      x = x + 44;
    }
     x = 65;
    for(uint8_t i = 0; i < 9; i++){
      row4keys_btn[i].initButton(&tft,
                        x,
                        y + 132,
                        w,
                        h,
                        PRIME_COLOR,           // OUTLINE BORDER COLOR
                        SECONDARY_COLOR,          // FILL COLOR
                        PRIME_COLOR,         // TEXT COLOR
                        row4keys[i],      // label na magolo
                        1);             // TEXT SIZE
                        
      row4keys_btn[i].setLabelDatum(0, 2, CC_DATUM);
      row4keys_btn[i].press(false);
      row4keys_btn[i].drawButton();
      x = x + 44;
    }
  
  } else {//rowCAPSkeys
    for(uint8_t i = 0; i < 10; i++){
      row2keys_btn[i].initButton(&tft,
                        x,
                        y + 44,
                        w,
                        h,
                        PRIME_COLOR,           // OUTLINE BORDER COLOR
                        SECONDARY_COLOR,          // FILL COLOR
                        PRIME_COLOR,         // TEXT COLOR
                        row2CAPSkeys[i],      // label na magolo
                        1);             // TEXT SIZE
                        
      row2keys_btn[i].setLabelDatum(0, 2, CC_DATUM);
      row2keys_btn[i].press(false);
      row2keys_btn[i].drawButton();
      x = x + 44;
    }
     x = 40; //65
    for(uint8_t i = 0; i < 10; i++){
      row3keys_btn[i].initButton(&tft,
                        x,
                        y + 88,
                        w,
                        h,
                        PRIME_COLOR,           // OUTLINE BORDER COLOR
                        SECONDARY_COLOR,          // FILL COLOR
                        PRIME_COLOR,         // TEXT COLOR
                        row3CAPSkeys[i],      // label na magolo
                        1);             // TEXT SIZE
                        
      row3keys_btn[i].setLabelDatum(0, 2, CC_DATUM);
      row3keys_btn[i].press(false);
      row3keys_btn[i].drawButton();
      x = x + 44;
    }
     x = 65;
    for(uint8_t i = 0; i < 9; i++){
      row4keys_btn[i].initButton(&tft,
                        x,
                        y + 132,
                        w,
                        h,
                        PRIME_COLOR,           // OUTLINE BORDER COLOR
                        SECONDARY_COLOR,          // FILL COLOR
                        PRIME_COLOR,         // TEXT COLOR
                        row4CAPSkeys[i],      // label na magolo
                        1);             // TEXT SIZE
                        
      row4keys_btn[i].setLabelDatum(0, 2, CC_DATUM);
      row4keys_btn[i].press(false);
      row4keys_btn[i].drawButton();
      x = x + 44;
    }



  }

    x = 57;
    for(uint8_t i = 0; i < 5; i++){
      if(i==2){
        w = 230;
        x = x + 95;
      } else {
        w = 40;
      }
      row5keys_btn[i].initButton(&tft,
                        x,
                        y + 176,
                        w,
                        h,
                        PRIME_COLOR,           // OUTLINE BORDER COLOR
                        SECONDARY_COLOR,          // FILL COLOR
                        PRIME_COLOR,         // TEXT COLOR
                        row5keys[i],      // label na magolo
                        1);             // TEXT SIZE
                        
      row5keys_btn[i].setLabelDatum(0, 2, CC_DATUM);
      row5keys_btn[i].press(false);
      row5keys_btn[i].drawButton();
      x = x + 44;
      if(i==2) x = x + 95;
    }
  
  
}


// display with left header template
void displayScreenWithLeftHeaderTemplate(char *label[]) {
  // check the label[] parameter
  // label[1] = the name next to the result (Result : Juan Dela Cruz)
  tft.fillScreen(SECONDARY_COLOR);
  tft.fillRect(screenWithHeaderTemplate.header_x, screenWithHeaderTemplate.header_y, screenWithHeaderTemplate.header_w, screenWithHeaderTemplate.header_h, PRIME_COLOR);
  tft.setFreeFont(TERTIARY_FONT);
  tft.setTextColor(SECONDARY_COLOR, PRIME_COLOR);
  tft.setCursor(22, 25);
  tft.print(label[0]);
  tft.print(name);
  // display info
}

// display with header template
void displayScreenWithHeaderTemplate(char *label[]) {
  // template
  tft.fillScreen(SECONDARY_COLOR);
  tft.fillRect(screenWithHeaderTemplate.header_x, screenWithHeaderTemplate.header_y, screenWithHeaderTemplate.header_w, screenWithHeaderTemplate.header_h, PRIME_COLOR);
  tft.setFreeFont(TERTIARY_FONT);
  tft.setTextColor(SECONDARY_COLOR, PRIME_COLOR);
  tft.setCursor(0, 0);
  tft.setTextDatum(CC_DATUM);
  tft.drawString(label[0], tft.getViewportWidth() / 2, screenWithHeaderTemplate.header_h / 2);
  // display info
}

// loading screen: pinakaunang screen na magloloading
void goLoadingScreen() {
  tft.fillScreen(SECONDARY_COLOR);
  // tft.fillSmoothRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t radius, uint32_t color, uint32_t bg_color)
  tft.fillSmoothRoundRect(80, 120, 320, 70, 30, PRIME_COLOR, SECONDARY_COLOR);
  tft.setTextColor(SECONDARY_COLOR, PRIME_COLOR);
  tft.setCursor(125, 165);
  tft.setFreeFont(PRIMARY_FONT);
  tft.setTextSize(1);
  tft.println("PeeCheck 2.0");
}
// ======================================================
// *                                                    *
// *               END OF SCREEN DISPLAYS               *
// *                                                    *
// ======================================================

// ===================================================================================

// ======================================================
// *                                                    *
// *           Start of Screen Flow Handler             *
// *                                                    *
// *    Every screen displayed are stacked on an array. *
// *    the last item in the array is the current       *
// *    screen on display.                              *
// *                                                    *
// *    Each back buttons removes the current screen    *
// *    and displays the previous screen depends on     *
// *    how much the remove function called             *
// *                                                    *
// *    The the purpose of the function below are to    *
// *    get the current screen for checking the         *
// *    previous and the new requested screen.          *  
// *                                                    *
// *                                                    *
// *      In short:                                     *
// *      Nothing to do here cuz this is fixed.         *
// *      you just have to call the function base on    *
// *      the screen u want to display                  *
// *                                                    *
// ======================================================

// get current screen
// return the last item on array( most of the time the current screen)
int getCurrentScreenOnArray() {
  uint16_t currentScreen;
  if (SCREENS[1] == 0) {
    currentScreen = SCREENS[0];
  } else {
    for (uint8_t i = 1; i < screenLength; i++) {
      if (SCREENS[i] == 0) {
        currentScreen = SCREENS[i - 1];
        break;
      }
    }
  }
  return currentScreen;
}

// add the new screen to the stack
// to use this addScreenOnArray(screenNumber) ex: addScreenOnArray(401); // calibration screen 
void addNewScreenOnArray(uint16_t ScreenToSet) {
  for (uint8_t i = 1; i < screenLength; i++) {
    if (SCREENS[i] == 0) {
      SCREENS[i] = ScreenToSet;
      break;
    }
  }
}

// remove current screen and go back to previous screen
void removeLastScreenOnArray() {
  if (SCREENS[1] == 0 && SCREENS[0] == 100) { return; }
  for (uint8_t i = 1; i < screenLength; i++) {  // i = 1
    if (SCREENS[i] == 0) {
      SCREENS[i - 1] = 0;
      break;
    }
  }
}

// Handler of all screen
// dont edit this unless you need to change the flow of screen.
void setNewScreenOnDisplay(uint16_t screenNumber) {
  Serial.print("SCREENS: [");
  for (uint8_t i = 0; i < screenLength; i++) {
    Serial.print(SCREENS[i]);
    Serial.print(", ");
  }
  Serial.print("] ; Current screen number: ");
  Serial.println(screenNumber);
  switch (screenNumber) {
    case 100:  // mainmenu
      displayMainMenu();
      break;
    case 201:  // analyze
      displayPatientInformationInputField();
      break;
    case 202: // 202 display
      displayParameters();
      break;
    case 203: // analyzing 
      displayAnalyzing();
      break;
    case 204: // test strip not detected
      displayTestStriptNotDetected();
      break;
    case 205: // 4sg
      display_URS_4SG_Result();
      break;
    case 206: // 10T
        display_URS_10T_Result();
      break;
    case 207: // 11T
      display_URS_11T_Result();
      break;
    case 208: // 14 
      display_URS_14_Result();
      break;
    case 209: // sent
      displaySMSSent();
      break;
    case 210: // displayAIAnalyze
      displayAIAnalyze();
      break;
    case 301:  // patient data
        displayPatientRecord();
      break;
    case 302:
        displayOnePatientRecord(currentUserInfo);
      break;
    case 303:
        displayAIAnalyze();
      break;
    case 304:
        displaySMSSent();
      break;
    case 401:  // calibrate
      displayCalibrating();
    case 402:
      displayCalibrated();
      break;
    case 501:  // clean option
      displayCleanOptions();
      break;
    case 502:  // eject
      displayEjected();
      break;
    case 503:  // lcok
      displayLocked();
      break;
    case 600:
      displayKeyboard();
      break;
    default:
      tft.fillScreen(TFT_BROWN);
  }
}

// ======================================================
// *                                                    *
// *           End of Screen Flow Handler               *
// *                                                    *
// ======================================================


// ============ keyboard functions ===========
// do not edit  

void addNewCharToInput(const char input[]){
  userInput[currentCaret] = input[0];
  currentCaret++;
  tft.setTextColor(PRIME_COLOR, SECONDARY_COLOR);
  tft.drawString(userInput, 20, 38);
}

void removeLastCharToInput(){
  if(currentCaret != 0) currentCaret--;
  userInput[currentCaret] = ' ';
  tft.setTextColor(PRIME_COLOR, SECONDARY_COLOR);
  tft.fillRect(11,26 ,458 ,43 , SECONDARY_COLOR);
  // tft.drawRect(, 25, 460, 45, PRIME_COLOR);
  tft.drawString(userInput, 20, 38);
}

void clearInputField(){
  memset(userInput, '\0', sizeof(userInput));
  currentCaret = 0;
  tft.setTextColor(PRIME_COLOR, SECONDARY_COLOR);
  // tft.drawString(userInput, 20, 38);
  tft.fillRect(11,26 ,458 ,43 , SECONDARY_COLOR);
}

// ==============end of keyboard functions ===========


// =============================================================================

// ======================================================
// *                                                    *
// *           Start of Screen Touch Handler            *
// *                                                    *
// *     the below codes are built to check             *
// *     what are the available parts of the screen     *  
// *     to be touch.                                   *
// *                                                    *
// *     TAKE NOTE: if the screen has a button,         *
// *               expect that it has a corresponding   *
// *               touch handler.                       *  
// *                                                    *
// *                                                    *
// *                                                    *
// ======================================================

// this is the input box of name, age, gender, mobile num
void getTouchOfPatientInfoInputBox(){
  uint16_t t_x = 0, t_y = 0;  // touch coord
  bool pressed = tft.getTouch(&t_x, &t_y);

  // Check if touch is inside any btn coord
  for (uint8_t b = 0; b < 4; b++) {

    if (pressed && patientInfoInputBox[b].contains(t_x, t_y)) {
      patientInfoInputBox[b].press(true);  // tell the button it is pressed

    } else {
      patientInfoInputBox[b].press(false);  // tell the button it is NOT pressed
    }
  }

  // btn UI states
  for (uint8_t b = 0; b < 4; b++) {

    if (patientInfoInputBox[b].justPressed()) {
      // patientInfoInputBox[b].drawButton(true, btnLabels[b]);  // if pressed, draw(inverted, label)
    }
    if (patientInfoInputBox[b].justReleased()) {
      patientInfoInputBoxPressCheck(b);    // pass the pressed btn for response sheesh
      // patientInfoInputBox[b].drawButton(false, btnLabels[b]);  // draw normal again
    }
  }
}

void getTouchOfAnalyzeAgainBtn() {
  uint16_t t_x = 0, t_y = 0;
  bool pressed = tft.getTouch(&t_x, &t_y);
  if (pressed && testStripNotDetectedBtn[0].contains(t_x, t_y)) {
    testStripNotDetectedBtn[0].press(true);  // tell the button it is pressed
  } else {
    testStripNotDetectedBtn[0].press(false);  // tell the button it is NOT pressed
  }
  // btn UI states
  if (testStripNotDetectedBtn[0].justPressed()) {
    testStripNotDetectedBtn[0].drawButton(true, "ANALYZE AGAIN");  // if pressed, draw(inverted, label)
  }
  if (testStripNotDetectedBtn[0].justReleased()) {
    testStripNotDetectedBtn[0].drawButton();
    responseOfAnalyzeAgain();
  }
}

// edit this for 205, 206, 207, 208
void getTouchOfAnalyzedResult(TFT_eSPI_Button btnReference[], uint8_t btnCount, char *btnLabels[]) {
  uint16_t t_x = 0, t_y = 0;  // touch coord
  bool pressed = tft.getTouch(&t_x, &t_y);

  // Check if touch is inside any btn coord
  for (uint8_t b = 0; b < btnCount; b++) {

    if (pressed && btnReference[b].contains(t_x, t_y)) {
      btnReference[b].press(true);  // tell the button it is pressed

    } else {
      btnReference[b].press(false);  // tell the button it is NOT pressed
    }
  }

  // btn UI states
  for (uint8_t b = 0; b < btnCount; b++) {

    if (btnReference[b].justPressed()) {
      btnReference[b].drawButton(true, btnLabels[b]);  // if pressed, draw(inverted, label)
    }
    if (btnReference[b].justReleased()) {
      analyzedResultBtnPressCheck(b);    // pass the pressed btn for response sheesh
      btnReference[b].drawButton(false, btnLabels[b]);  // draw normal again
    }
  }
}

// 202
void getTouchOfParameterOption(TFT_eSPI_Button btnReference[], uint8_t btnCount, char *btnLabels[]) {
  uint16_t t_x = 0, t_y = 0;  // touch coord
  bool pressed = tft.getTouch(&t_x, &t_y);

  // Check if touch is inside any btn coord
  for (uint8_t b = 0; b < btnCount; b++) {

    if (pressed && btnReference[b].contains(t_x, t_y)) {
      btnReference[b].press(true);  // tell the button it is pressed
    } else {
      btnReference[b].press(false);  // tell the button it is NOT pressed
    }
  }

  // btn UI states
  for (uint8_t b = 0; b < btnCount; b++) {

    if (btnReference[b].justPressed()) {
      btnReference[b].drawButton(true, btnLabels[b]);  // if pressed, draw(inverted, label)
    }
    if (btnReference[b].justReleased()) {
      btnReference[b].drawButton();  // draw normal again
      parameterOptionPressCheck(b);    // pass the pressed btn for response sheesh
    }
  }
}

// 201
void getTouchOfNextBtn(TFT_eSPI_Button btnReference[], char *btnLabels[], void (*response)()) {
  uint16_t t_x = 0, t_y = 0;
  bool pressed = tft.getTouch(&t_x, &t_y);
  if (pressed && btnReference[0].contains(t_x, t_y)) {
    btnReference[0].press(true);  // tell the button it is pressed
  } else {
    btnReference[0].press(false);  // tell the button it is NOT pressed
  }
  // btn UI states
  if (btnReference[0].justPressed()) {
    btnReference[0].drawButton(true, btnLabels[0]);  // if pressed, draw(inverted, label)
  }
  if (btnReference[0].justReleased()) {
    btnReference[0].drawButton();
    response();
  }
}

// 301
void getTouchOfPatientRecordResult(TFT_eSPI_Button btnReference[], uint8_t btnCount, char *btnLabels[]) {
  uint16_t t_x = 0, t_y = 0;  // touch coord
  bool pressed = tft.getTouch(&t_x, &t_y);

  // Check if touch is inside any btn coord
  for (uint8_t b = 0; b < btnCount; b++) {

    if (pressed && btnReference[b].contains(t_x, t_y)) {
      btnReference[b].press(true);  // tell the button it is pressed

    } else {
      btnReference[b].press(false);  // tell the button it is NOT pressed
    }
  }

  // btn UI states
  for (uint8_t b = 0; b < btnCount; b++) {

    if (btnReference[b].justPressed()) {
      btnReference[b].drawButton(true, btnLabels[b]);  // if pressed, draw(inverted, label)
    }
    if (btnReference[b].justReleased()) {
      patientRecordResultPressCheck(b);    // pass the pressed btn for response sheesh
      btnReference[b].drawButton(false, btnLabels[b]);  // draw normal again
    }
  }
}

// 301
void getTouchOfPrevNext(TFT_eSPI_Button btnReference[], uint8_t btnCount, char *btnLabels[]){
  uint16_t t_x = 0, t_y = 0;  // touch coord
  bool pressed = tft.getTouch(&t_x, &t_y);

  for (uint8_t b = 0; b < btnCount; b++) {

    if (pressed && btnReference[b].contains(t_x, t_y)) {
      btnReference[b].press(true);  // tell the button it is pressed

    } else {
      btnReference[b].press(false);  // tell the button it is NOT pressed
    }
  }

  for (uint8_t b = 0; b < btnCount; b++) {

    if (btnReference[b].justPressed()) {
      btnReference[b].drawButton(true, btnLabels[b]);  // if pressed, draw(inverted, label)
    }
    if (btnReference[b].justReleased()) {
      prevNextPressCheck(b);    // pass the pressed btn for response sheesh
      btnReference[b].drawButton(false,btnLabels[b]);  // draw normal again
    }
  }
}

// 301
void getTouchOfPatientRecordsNames(TFT_eSPI_Button btnReference[], uint8_t btnCount, char *btnLabels[]){
  uint16_t t_x = 0, t_y = 0;  // touch coord
  bool pressed = tft.getTouch(&t_x, &t_y);

  uint8_t scrollPageItems = scrollPage * 4;

  uint16_t scrollPageStart = scrollPageItems - 4;
  uint16_t scrollPageEnd = scrollPageItems;

  for (uint8_t b = scrollPageStart; b < scrollPageEnd; b++) {

    if (pressed && btnReference[b].contains(t_x, t_y)) {
      btnReference[b].press(true);  // tell the button it is pressed

    } else {
      btnReference[b].press(false);  // tell the button it is NOT pressed
    }
  }

  for (uint8_t b = scrollPageStart; b < scrollPageEnd; b++) {

    if (btnReference[b].justPressed()) {
      btnReference[b].drawButton(true, btnLabels[b]);  // if pressed, draw(inverted, label)
    }
    if (btnReference[b].justReleased()) {
      btnReference[b].drawButton();  // draw normal again
      patientRecordNamesPressCheck(b);    // pass the pressed btn for response sheesh
    }
  }
}

// 501
void getTouchOfCleanOption(TFT_eSPI_Button btnReference[], uint8_t btnCount, char *btnLabels[]) {
  uint16_t t_x = 0, t_y = 0;  // touch coord
  bool pressed = tft.getTouch(&t_x, &t_y);

  // Check if touch is inside any btn coord
  for (uint8_t b = 0; b < btnCount; b++) {

    if (pressed && btnReference[b].contains(t_x, t_y)) {
      btnReference[b].press(true);  // tell the button it is pressed

    } else {
      btnReference[b].press(false);  // tell the button it is NOT pressed
    }
  }

  // btn UI states
  for (uint8_t b = 0; b < btnCount; b++) {

    if (btnReference[b].justPressed()) {
      btnReference[b].drawButton(true, btnLabels[b]);  // if pressed, draw(inverted, label)
    }
    if (btnReference[b].justReleased()) {
      cleanOptionBtnPressCheck(b);   // pass the pressed btn for response sheesh
      btnReference[b].drawButton();  // draw normal again
    }
  }
}

// touch (tftBtn, btncount, btnlabel, function response)
void getTouchOfOkayBtn(TFT_eSPI_Button btnReference[], uint8_t btnCount, char *btnLabels[], void (*okayResponse)()) {
  uint16_t t_x = 0, t_y = 0;
  bool pressed = tft.getTouch(&t_x, &t_y);
  if (pressed && btnReference[0].contains(t_x, t_y)) {
    btnReference[0].press(true);  // tell the button it is pressed
  } else {
    btnReference[0].press(false);  // tell the button it is NOT pressed
  }
  // btn UI states
  if (btnReference[0].justPressed()) {
    btnReference[0].drawButton(true, btnLabels[0]);  // if pressed, draw(inverted, label)
  }
  if (btnReference[0].justReleased()) {
    btnReference[0].drawButton();
    okayResponse();
  }
}
// 100 main screen
void getTouchOfMainScreen(TFT_eSPI_Button btnReference[], uint8_t btnCount, char *btnLabels[]) {
  uint16_t t_x = 0, t_y = 0;  // touch coord
  bool pressed = tft.getTouch(&t_x, &t_y);

  // Check if touch is inside any btn coord
  for (uint8_t b = 0; b < btnCount; b++) {

    if (pressed && btnReference[b].contains(t_x, t_y)) {
      btnReference[b].press(true);  // tell the button it is pressed

    } else {
      btnReference[b].press(false);  // tell the button it is NOT pressed
    }
  }

  // btn UI states
  for (uint8_t b = 0; b < btnCount; b++) {

    if (btnReference[b].justPressed()) {
      btnReference[b].drawButton(true, btnLabels[b]);  // if pressed, draw(inverted, label)
    }
    if (btnReference[b].justReleased()) {
      btnReference[b].drawButton();  // draw normal again
      mainScreenBtnPressCheck(b);    // pass the pressed btn for response sheesh
    }
  }
}

// 600
void getTouchOfRow1(){
  uint16_t t_x = 0, t_y = 0; // To store the touch coordinates
  bool pressed = tft.getTouch(&t_x, &t_y);
  for (uint8_t b = 0; b < 10; b++) {
    if (pressed && row1keys_btn[b].contains(t_x, t_y)) {
      row1keys_btn[b].press(true);  // tell the button it is pressed
    } else {
      row1keys_btn[b].press(false);  // tell the button it is NOT pressed
    }
  }

  // Check if any key has changed state
  for (uint8_t b = 0; b < 10; b++) {

    if (row1keys_btn[b].justReleased()){
      row1keys_btn[b].drawButton(); // draw normal again
      row1BtnPressCheck(b); // Call the button press function and pass the button number
    }
    if (row1keys_btn[b].justPressed()){
      row1keys_btn[b].drawButton(true);  // draw invert (background becomes text colour and text becomes background colour
    }
  }
}
void getTouchOfRow2(){
  uint16_t t_x = 0, t_y = 0; // To store the touch coordinates
  bool pressed = tft.getTouch(&t_x, &t_y);
  for (uint8_t b = 0; b < 10; b++) {
    if (pressed && row2keys_btn[b].contains(t_x, t_y)) {
      row2keys_btn[b].press(true);  // tell the button it is pressed
    } else {
      row2keys_btn[b].press(false);  // tell the button it is NOT pressed
    }
  }

  // Check if any key has changed state
  for (uint8_t b = 0; b < 10; b++) {

    if (row2keys_btn[b].justReleased()) {
      row2keys_btn[b].drawButton(); // draw normal again
      row2BtnPressCheck(b); // Call the button press function and pass the button number
    }
    if (row2keys_btn[b].justPressed()){
      row2keys_btn[b].drawButton(true);  // draw invert (background becomes text colour and text becomes background colour
    }
  }
}
void getTouchOfRow3(){
  uint16_t t_x = 0, t_y = 0; // To store the touch coordinates
  bool pressed = tft.getTouch(&t_x, &t_y);
  for (uint8_t b = 0; b < 10; b++) {
    if (pressed && row3keys_btn[b].contains(t_x, t_y)) {
      row3keys_btn[b].press(true);  // tell the button it is pressed
    } else {
      row3keys_btn[b].press(false);  // tell the button it is NOT pressed
    }
  }

  // Check if any key has changed state
  for (uint8_t b = 0; b < 10; b++) {

    if (row3keys_btn[b].justReleased()) {
      row3keys_btn[b].drawButton(); // draw normal again
      row3BtnPressCheck(b); // Call the button press function and pass the button number
      }
    if (row3keys_btn[b].justPressed()){
      row3keys_btn[b].drawButton(true);  // draw invert (background becomes text colour and text becomes background colour
    }
  }
}
void getTouchOfRow4(){
  uint16_t t_x = 0, t_y = 0; // To store the touch coordinates
  bool pressed = tft.getTouch(&t_x, &t_y);
  for (uint8_t b = 0; b < 9; b++) {
    if (pressed && row4keys_btn[b].contains(t_x, t_y)) {
      row4keys_btn[b].press(true);  // tell the button it is pressed
    } else {
      row4keys_btn[b].press(false);  // tell the button it is NOT pressed
    }
  }

  // Check if any key has changed state
  for (uint8_t b = 0; b < 9; b++) {

    if (row4keys_btn[b].justReleased()) {
      row4keys_btn[b].drawButton(); // draw normal again
      row4BtnPressCheck(b); // Call the button press function and pass the button number
    }
    if (row4keys_btn[b].justPressed()){
      row4keys_btn[b].drawButton(true);  // draw invert (background becomes text colour and text becomes background colour
    }
  }
}
void getTouchOfRow5(){
  uint16_t t_x = 0, t_y = 0; // To store the touch coordinates
  bool pressed = tft.getTouch(&t_x, &t_y);
  for (uint8_t b = 0; b < 5; b++) {
    if (pressed && row5keys_btn[b].contains(t_x, t_y)) {
      row5keys_btn[b].press(true);  // tell the button it is pressed
    } else {
      row5keys_btn[b].press(false);  // tell the button it is NOT pressed
    }
  }

  // Check if any key has changed state
  for (uint8_t b = 0; b < 5; b++) {

    if (row5keys_btn[b].justReleased()) {
      row5keys_btn[b].drawButton(); // draw normal again
      row5BtnPressCheck(b); // Call the button press function and pass the button number
    }
    if (row5keys_btn[b].justPressed()){
      row5keys_btn[b].drawButton(true);  // draw invert (background becomes text colour and text becomes background colour
    }
  }
}


// Main handler of all touch 
// dont edit this unless you need to change the flow of touch
void setTouchOnDisplay(uint16_t screenNumber) {
  switch (screenNumber) {
    case 100:  // mainmenu
      getTouchOfMainScreen(mainScreenBtn, mainScreenBtnCount, mainMenu_BtnLabel);
      break;
    case 201:  //analyze
      // input field
      getTouchOfOkayBtn(Okay_Btn, 1, BackBtnLabel, &responseOfBackOnPatientInput);
      getTouchOfNextBtn(nextBtn, nextBtnLabel, &responseOfNextbtn);
      getTouchOfPatientInfoInputBox();
      break;
    case 202:
      getTouchOfParameterOption(testStripBtn, 4, testStrip_BtnLabels);
      getTouchOfOkayBtn(Okay_Btn, 1, BackBtnLabel, &responseOfBackOnParameterOption);
      break;
    case 203: // analyzing. no touch here
      
      break;
    case 204: // test strip not detected
      getTouchOfAnalyzeAgainBtn();
      break;
    case 205:
      getTouchOfAnalyzedResult(testStripOption_Btn, 5, testStripOption_BtnLabel);
      break;
    case 206:
      getTouchOfAnalyzedResult(testStripOption_Btn,5,testStripOption_BtnLabel);
      break;
    case 207:
      getTouchOfAnalyzedResult(testStripOption_Btn,5,testStripOption_BtnLabel);
      break;
    case 208:
      getTouchOfAnalyzedResult(testStripOption_Btn,5,testStripOption_BtnLabel);
      break;
    case 209:
      getTouchOfOkayBtn(Okay_Btn, 1, OkayBtnLabel, &responseOfOkayOnAnalyzedSMSSent);
      break;
    case 210: 
      getTouchOfOkayBtn(Okay_Btn, 1, BackBtnLabel, &responseOfBackOnAIAnalyze);
      break;
    case 301:  // patient data
      getTouchOfOkayBtn(Okay_Btn, 1, BackBtnLabel, &responseOfBackOnPatientRecord);
      getTouchOfPrevNext(PrevNext_Btns, 2 ,PrevNextLabel);
      getTouchOfPatientRecordsNames(PatientRecordsNames_Btn,numberOfRecords,patientRecodsNamesLabel);
      break;
    case 302:
      getTouchOfPatientRecordResult(testStripOption_Btn,5, testStripOption_BtnLabel);
      break;
    case 303:
      getTouchOfOkayBtn(Okay_Btn, 1, BackBtnLabel, &responseOfBackOnAIAnalyze);
      break;
    case 304:
      getTouchOfOkayBtn(Okay_Btn, 1, OkayBtnLabel, &responseOfOkayOnSMSSent);
      break;
    case 401:  // no btns here kasi loading duhhh
      // charot! calibrating kasi wala naman pipindutin
      break;
    case 402:  //calibrate
      getTouchOfOkayBtn(Okay_Btn, 1, OkayBtnLabel, &responseOfOkayOncalibration);
      break;
    case 501:  // *clean
      getTouchOfCleanOption(CleanOption_Btn, 2, cleanOptionLabel);
      getTouchOfOkayBtn(Okay_Btn, 1, BackBtnLabel, &responseOfBackCleanOptionBtn);
      break;
    case 502:  // *eject
      getTouchOfOkayBtn(Okay_Btn, 1, OkayBtnLabel, &responseOfOkayOnEjected);
      break;
    case 503:  // *lock
      getTouchOfOkayBtn(Okay_Btn, 1, OkayBtnLabel, &responseOfOkayOnLocked);
      break;
    case 600:
      getTouchOfRow1();
      getTouchOfRow2();
      getTouchOfRow3();
      getTouchOfRow4();
      getTouchOfRow5();
      break;
    default:
      Serial.println("setTouchOnDisplayError: target screen unknown.");
      break;                              
    }
}

// ======================================================
// *                                                    *
// *         End of Screen Screen Touch Handler         *
// *                                                    *
// ======================================================


// ================================================================================================


// ======================================================
// *                                                    *
// *            Start of Touch Checkers                 *
// *                                                    *
// *                                                    *
// *  this is the checkers of the button that has been  *
// *  touch which calls the function of the btns        *
// *                                                    *
// *                                                    *
// ======================================================


// patientInfoInputBoxPressCheck();
void patientInfoInputBoxPressCheck(uint8_t button) {
  switch (button) {
    case 0:
      currentTargetOnKeyboardInput = 0;
      memset(userInput, '\0', sizeof(userInput)); // clear the input array first 
      currentCaret = 0;
      addNewScreenOnArray(600); // call the keyboard sheeesh
      break;
    case 1:
      currentTargetOnKeyboardInput = 1;
      memset(userInput, '\0', sizeof(userInput)); // clear the input array first 
      currentCaret = 0;
      addNewScreenOnArray(600); //
      break;
    case 2:
      currentTargetOnKeyboardInput = 2;
      memset(userInput, '\0', sizeof(userInput)); // clear the input array first 
      currentCaret = 0;
      addNewScreenOnArray(600); // 
      break;
    case 3:
      currentTargetOnKeyboardInput = 3;
      memset(userInput, '\0', sizeof(userInput)); // clear the input array first 
      currentCaret = 0;
      addNewScreenOnArray(600); // 
      break;
      
  }
}

// 205, 206, 207, 208
void analyzedResultBtnPressCheck(uint8_t button) {
  switch (button) {
    case 0:
      addNewScreenOnArray(210); // AI Analyze
      break;
    case 1:
      // Save

      saveToSpreadSheet();

      // add your save feature here.tip: create a function and call it here.
      //
      break;
    case 2:
      // reanalyze
      // your feature of "analyze again"
      break;
    case 3:
      // back
      responseOfBackOnAnalyzedResult();
      break;
    case 4:
      // send sms
      addNewScreenOnArray(209); 
      break;
  }
}

// 202
void parameterOptionPressCheck(uint8_t button) {
  switch (button) {
    case 0:
      currentTesting = 0; //set current testing parameters
      // displays the analyzing screen
      displayAnalyzing();
      // 
      // add your logic of analyzing mechanism.
      // wrap that mechanism code to a function and call it here
      // if success and u have all the data. change the value of of the results base on your analysis
      // ctrl + f and type ### and edit the dummy results base on your analyzed data
      // lastly call the below code for displaying the next screen after the analysis
      
      // also, dont forget to do the same process in the following cases(other test)
      
      // this is the success screen displaying the data
      addNewScreenOnArray(205); // 4 result test 
      break;

    case 1:
      currentTesting = 1; //set current testing parameters
      displayAnalyzing();// analyze first
      // if there is no test strip, dispaly the error on strip to the screen, else, analyze again
      // display error 
      addNewScreenOnArray(204); //test strip not detected screen
      // do it here again
      // addNewScreenOnArray(206); // 10 result test // this line is called in the response of ANALYZE AGAIN button
      break;

    case 2:
      currentTesting = 2; //set current testing parameters
      displayAnalyzing();
      // here again
      addNewScreenOnArray(207); // 11
      break;
    case 3:
      currentTesting = 3; //set current testing parameters
      displayAnalyzing();
      readStrip();
      addNewScreenOnArray(208); // 14
      stepper.moveRelativeInSteps(-8000);
      break;
  }
}

// 302
void patientRecordResultPressCheck(uint8_t button) {
  switch (button) {
    case 0:
      // AI analyze
      addNewScreenOnArray(303); 
      break;
    case 1:
      // Save
      // add your save feature here. create a function and call it here.
      break;
    case 2:
      // reanalyze
      // your feature of "analyze again"
      break;
    case 3:
      // back
      responseOfBackOnOnePatienResult();
      break;
    case 4:
      // send sms
      addNewScreenOnArray(304); 
      break;
    default:
      tft.fillScreen(TFT_BROWN);
  }
}

// 301
void prevNextPressCheck(uint16_t button){
  switch (button) {
    case 0:
      if(scrollPage > 1 && scrollPage <= 9 ){
        scrollPage = scrollPage - 1;
        displayRecordNames();
      } 
      break;
    case 1:
      if(scrollPage >= 1 && scrollPage < 9) {
        scrollPage = scrollPage + 1;
        displayRecordNames();
      }
      break;
  }
    // // ======
    // if(button == 0){
    //   if(scrollPage > 1 && scrollPage <= 9 ){
    //     scrollPage = scrollPage - 1;
    //     displayRecordNames();
    //   } 
    // }
    // else if(button ==  1){
    //   if(scrollPage >= 1 && scrollPage < 9) {
    //     scrollPage = scrollPage + 1;
    //     displayRecordNames();
    //   }
    // }
    // // ===

}

// 301
void patientRecordNamesPressCheck(uint8_t btnIndex){
  addNewScreenOnArray(302); 
  currentUserInfo = btnIndex;
  for (uint8_t i = 0 ; i < numberOfRecords; i++){
    if(btnIndex == i){
      name = PatientsRecordsInSpreadSheet[i][0]; 
      break;
    }
  }
}

// 501 touch checker
void cleanOptionBtnPressCheck(uint8_t button) {
  switch (button) {
    case 0:
      addNewScreenOnArray(502);
      break;
    case 1:
      addNewScreenOnArray(503);
      break;
  }
}

//100 touch checker
void mainScreenBtnPressCheck(uint8_t button) {
  switch (button) {
    case 0:
      memset(userInput, '\0', sizeof(userInput)); // clear the input array first 
      currentCaret = 0;
      memset(patientInfoInputValue, '\0', sizeof(patientInfoInputValue));
      addNewScreenOnArray(201); // Analyze
      break;
    case 1:
      pullData();
    // while(isDoingHttpRequest == true){;}
      addNewScreenOnArray(301); // patient data
      break;
    case 2:
      addNewScreenOnArray(401);  // calibrate
      break;
    case 3:
      addNewScreenOnArray(501);  // clean
      break;
  }
}

// ====== keybaords ==========
// nothing to edit here 

// 600 keyboard
void row5BtnPressCheck(int button) {
  switch (button) {
    case 0: // '
      addNewCharToInput(row5keys[button]);
      break;
    case 1:// ,
      addNewCharToInput(row5keys[button]);
      break;
    case 2:// space bar
      addNewCharToInput(row5keys[button]);
      break;
    case 3:// .
      addNewCharToInput(row5keys[button]);
      break;
    case 4:// this is enter button
      // pass the value name, age, gender, #
      // patientInfoInputValue[currentTargetOnKeyboardInput] = userInput;
      // Serial.println(currentTargetOnKeyboardInput);
      // pass the name after the result
      if(currentTargetOnKeyboardInput == 0){ // check first if that is the name cuz that sucks if you pass the results followed by a mobile number;
        name = userInput;
        patientInfoInputValue[currentTargetOnKeyboardInput] = userInput;
      }
      else if(currentTargetOnKeyboardInput == 1 ){ 
        patientInfoInputValue[currentTargetOnKeyboardInput] = userInput;
      }
      else if(currentTargetOnKeyboardInput == 2 ){ 
        patientInfoInputValue[currentTargetOnKeyboardInput] = userInput;
      }
      else if(currentTargetOnKeyboardInput == 3 ){ 
        patientInfoInputValue[currentTargetOnKeyboardInput] = userInput;
      }

      Serial.println(currentTargetOnKeyboardInput);
      Serial.println(name);
      Serial.println(userInput);


      
      // remove the keyboard and go back to the previous screen
      removeLastScreenOnArray();

      // reset to false so the next display is on lowercase
      keyboardOnCaps = false;

      // haba ng flow sheesh
      break;
  }
}

void row4BtnPressCheck(int button)
{
  switch (button) {
    case 0: // this is shift
      if(keyboardOnCaps == true) keyboardOnCaps = false;
      else keyboardOnCaps = true;
      displayKeys();
      break;
    case 1:
      if(keyboardOnCaps == false) addNewCharToInput(row4keys[button]);
      else addNewCharToInput(row4CAPSkeys[button]);
      break;
    case 2:
      if(keyboardOnCaps == false) addNewCharToInput(row4keys[button]);
      else addNewCharToInput(row4CAPSkeys[button]);
      break;
    case 3:
      if(keyboardOnCaps == false) addNewCharToInput(row4keys[button]);
      else addNewCharToInput(row4CAPSkeys[button]);
      break;
    case 4:
      if(keyboardOnCaps == false) addNewCharToInput(row4keys[button]);
      else addNewCharToInput(row4CAPSkeys[button]);
      break;
    case 5:
      if(keyboardOnCaps == false) addNewCharToInput(row4keys[button]);
      else addNewCharToInput(row4CAPSkeys[button]);
      break;
    case 6:
      if(keyboardOnCaps == false) addNewCharToInput(row4keys[button]);
      else addNewCharToInput(row4CAPSkeys[button]);
      break;
    case 7:
      if(keyboardOnCaps == false) addNewCharToInput(row4keys[button]);
      else addNewCharToInput(row4CAPSkeys[button]);
      break;
    case 8: // del
      removeLastCharToInput();
      break;
  }
}

void row3BtnPressCheck(int button)
{
  switch (button) {
    case 0:
      if(keyboardOnCaps == false) addNewCharToInput(row3keys[button]);
      else addNewCharToInput(row3CAPSkeys[button]);
      break;
    case 1:
      if(keyboardOnCaps == false) addNewCharToInput(row3keys[button]);
      else addNewCharToInput(row3CAPSkeys[button]);
      break;
    case 2:
      if(keyboardOnCaps == false) addNewCharToInput(row3keys[button]);
      else addNewCharToInput(row3CAPSkeys[button]);
      break;
    case 3:
      if(keyboardOnCaps == false) addNewCharToInput(row3keys[button]);
      else addNewCharToInput(row3CAPSkeys[button]);
      break;
    case 4:
      if(keyboardOnCaps == false) addNewCharToInput(row3keys[button]);
      else addNewCharToInput(row3CAPSkeys[button]);
      break;
    case 5:
      if(keyboardOnCaps == false) addNewCharToInput(row3keys[button]);
      else addNewCharToInput(row3CAPSkeys[button]);
      break;
    case 6:
      if(keyboardOnCaps == false) addNewCharToInput(row3keys[button]);
      else addNewCharToInput(row3CAPSkeys[button]);
      break;
    case 7:
      if(keyboardOnCaps == false) addNewCharToInput(row3keys[button]);
      else addNewCharToInput(row3CAPSkeys[button]);
      break;
    case 8:
      if(keyboardOnCaps == false) addNewCharToInput(row3keys[button]);
      else addNewCharToInput(row3CAPSkeys[button]);
      break;
    case 9:
      clearInputField();
      break;
  }
}

void row2BtnPressCheck(int button)
{
  switch (button) {
    case 0:
      if(keyboardOnCaps == false) addNewCharToInput(row2keys[button]);
      else addNewCharToInput(row2CAPSkeys[button]);
      break;
    case 1:
      if(keyboardOnCaps == false) addNewCharToInput(row2keys[button]);
      else addNewCharToInput(row2CAPSkeys[button]);
      break;
    case 2:
      if(keyboardOnCaps == false) addNewCharToInput(row2keys[button]);
      else addNewCharToInput(row2CAPSkeys[button]);
      break;
    case 3:
      if(keyboardOnCaps == false) addNewCharToInput(row2keys[button]);
      else addNewCharToInput(row2CAPSkeys[button]);
      break;
    case 4:
      if(keyboardOnCaps == false) addNewCharToInput(row2keys[button]);
      else addNewCharToInput(row2CAPSkeys[button]);
      break;
    case 5:
      if(keyboardOnCaps == false) addNewCharToInput(row2keys[button]);
      else addNewCharToInput(row2CAPSkeys[button]);
      break;
    case 6:
      if(keyboardOnCaps == false) addNewCharToInput(row2keys[button]);
      else addNewCharToInput(row2CAPSkeys[button]);
      break;
    case 7:
      if(keyboardOnCaps == false) addNewCharToInput(row2keys[button]);
      else addNewCharToInput(row2CAPSkeys[button]);
      break;
    case 8:
      if(keyboardOnCaps == false) addNewCharToInput(row2keys[button]);
      else addNewCharToInput(row2CAPSkeys[button]);
      break;
    case 9:
      if(keyboardOnCaps == false) addNewCharToInput(row2keys[button]);
      else addNewCharToInput(row2CAPSkeys[button]);
      break;
  }
}

void row1BtnPressCheck(int button)
{
  switch (button) {
    case 0:
      addNewCharToInput(row1keys[button]);
      break;
    case 1:
      addNewCharToInput(row1keys[button]);
      break;
    case 2:
      addNewCharToInput(row1keys[button]);
      break;
    case 3:
      addNewCharToInput(row1keys[button]);
      break;
    case 4:
      addNewCharToInput(row1keys[button]);
      break;
    case 5:
      addNewCharToInput(row1keys[button]);
      break;
    case 6:
      addNewCharToInput(row1keys[button]);
      break;
    case 7:
      addNewCharToInput(row1keys[button]);
      break;
    case 8:
      addNewCharToInput(row1keys[button]);
      Serial.println(row1keys[button]);
      break;
    case 9:
      addNewCharToInput(row1keys[button]);
      break;
  }
}

// ==== keyboard end =====

// ======================================================
// *                                                    *
// *           End of Screen Touch Checker              *
// *                                                    *
// ======================================================

// ========================================================================================

// ======================================================
// *                                                    *
// *            Start of Touch Responses                *
// *                                                    *
// *                                                    *
// *  this are the responses of every button            *
// *  that requires a function to be called             *
// *                                                    *
// ======================================================

// add the code below depends on your mechanism what will happened after the test strip is not detected
void responseOfAnalyzeAgain(){
  removeLastScreenOnArray(); // removes the not detected strip screen and add the new result 
   displayAnalyzing(); //analyze again
  addNewScreenOnArray(206); // the result of 10 parameter
}

//
void responseOfBackOnAnalyzedResult(){
  removeLastScreenOnArray();
}

// 209
void responseOfOkayOnAnalyzedSMSSent(){
  removeLastScreenOnArray();
  removeLastScreenOnArray();
  removeLastScreenOnArray();
  removeLastScreenOnArray();
  removeLastScreenOnArray();
}

// 202
void responseOfBackOnParameterOption(){
  // comment this if you dont want your data cleared after clicking the back button on 202 (patient parameter option)
  // clear the input in keyboard and input box
  memset(userInput, '\0', sizeof(userInput)); // clear the input array first 
  currentCaret = 0;
  memset(patientInfoInputValue, '\0', sizeof(patientInfoInputValue));

  removeLastScreenOnArray();
}

// 201
void responseOfNextbtn(){
  addNewScreenOnArray(202);
}

// 201
void responseOfBackOnPatientInput(){
  removeLastScreenOnArray();
  // comment this if you dont want your data cleared after clicking the back button on 201 (patient info input)
  // clear the input in keyboard and input box
  memset(userInput, '\0', sizeof(userInput)); // clear the input array first 
  currentCaret = 0;
  memset(patientInfoInputValue, '\0', sizeof(patientInfoInputValue));
  
}

// 304
void responseOfOkayOnSMSSent(){
  removeLastScreenOnArray();// each of this removes the current screen in the array 
  removeLastScreenOnArray();
  removeLastScreenOnArray();
}

// 303
void responseOfBackOnAIAnalyze(){
  removeLastScreenOnArray();
}

// 302
void responseOfBackOnOnePatienResult(){
  removeLastScreenOnArray();
}

// 301
void responseOfBackOnPatientRecord(){
  removeLastScreenOnArray();
}

// 402 okay btn response
void responseOfOkayOncalibration() {
  removeLastScreenOnArray();
  removeLastScreenOnArray();
  // addNewScreenOnArray(getCurrentScreenOnArray());
}

// 501 back
void responseOfBackCleanOptionBtn() {
  removeLastScreenOnArray();
}

// 502 okay btn response on ejected
void responseOfOkayOnEjected() {
  removeLastScreenOnArray();
  // addNewScreenOnArray(501);
}
// 503 okay response on locked
void responseOfOkayOnLocked() {
  removeLastScreenOnArray();
  // addNewScreenOnArray(501);
}


// ======================================================
// *                                                    *
// *           End of Screen Touch response             *
// *                                                    *
// ======================================================

// ========================================================================================

// ================================ COLOR ================


void calibrateTCS() {
  uint16_t r, g, b, c;
  delay(2000);

  stepper.moveRelativeInSteps(2500);
  delay(4000);
  tcs.getRawData(&r, &g, &b, &c);

  calibrated_R = r;
  calibrated_G = g;
  calibrated_B = b;

  delay(4000);
  stepper.moveRelativeInSteps(-3000);
}

void readStrip() {

  uint16_t r, g, b, c;
  float dividedRes;

  stepper.moveRelativeInSteps(1780);

  for (int i = 0; i < 14; i++) {

    tcs.getRawData(&r, &g, &b, &c);
    delay(60);

    // compute
    dividedRes = r / calibrated_R;
    r = int(round(dividedRes * 256));
    RGBvals[i][0] = r;

    dividedRes = g / calibrated_G;
    g = int(round(dividedRes * 256));
    RGBvals[i][1] = g;

    dividedRes = b / calibrated_B;
    b = int(round(dividedRes * 256));
    RGBvals[i][2] = b;

    // move the motor 1 step
    stepper.moveRelativeInSteps(420);
    delay(100);
  }

  readingResults[0] = getURU(RGBvals[0][0], RGBvals[0][1], RGBvals[0][2]);
  readingResults[1] = getBIL(RGBvals[1][0], RGBvals[1][1], RGBvals[1][2]);
  readingResults[2] = getKET(RGBvals[2][0], RGBvals[2][1], RGBvals[2][2]);
  readingResults[3] = getCRE(RGBvals[3][0], RGBvals[3][1], RGBvals[3][2]);
  readingResults[4] = getBLO(RGBvals[4][0], RGBvals[4][1], RGBvals[4][2]);
  readingResults[5] = getPRO(RGBvals[5][0], RGBvals[5][1], RGBvals[5][2]);
  readingResults[6] = getMIC(RGBvals[6][0], RGBvals[6][1], RGBvals[6][2]);
  readingResults[7] = getNIT(RGBvals[7][0], RGBvals[7][1], RGBvals[7][2]);
  readingResults[8] = getLEU(RGBvals[8][0], RGBvals[8][1], RGBvals[8][2]);
  readingResults[9] = getGLU(RGBvals[9][0], RGBvals[9][1], RGBvals[9][2]);
  readingResults[10] = getSPE(RGBvals[10][0], RGBvals[10][1], RGBvals[10][2]);
  readingResults[11] = getPH(RGBvals[11][0], RGBvals[11][1], RGBvals[11][2]);
  readingResults[12] = getASC(RGBvals[12][0], RGBvals[12][1], RGBvals[12][2]);
  readingResults[13] = getCAL(RGBvals[13][0], RGBvals[13][1], RGBvals[13][2]);

}

void saveToSpreadSheet() {

  if(currentTesting == 0){
        Serial.println("Saving 4sg");
        // save 4sg feature
      }
      else if(currentTesting == 1){
        Serial.println("Saving 10t");
        //10t feature
      }
      else if(currentTesting == 2){
        Serial.println("Saving 11t");
        //11t feature
      }
      else if(currentTesting == 3){
        Serial.println("Saving 14");

        Serial.print("starting to save...");
        
        if (WiFi.status() == WL_CONNECTED) {

          Serial.println("Trying to save...");
          String spreadSheetSauce = "https://script.google.com/macros/s/AKfycbxj_2cMoQ0CiXyJVl2le4w7AsOMzt-AfFXFpj8liPRthBN2eEnYLsn6f93eKAwY05pYNw/exec?sts=write";

          spreadSheetSauce += "&Name=" + patientInfoInputValue[0];
          spreadSheetSauce += "&Age="+ patientInfoInputValue[1];
          spreadSheetSauce += "&Gend=" + patientInfoInputValue[2];
          spreadSheetSauce += "&MNum=" + patientInfoInputValue[3];

          spreadSheetSauce += "&Uro=" + readingResults[0];
          spreadSheetSauce += "&Bil=" + readingResults[1];
          spreadSheetSauce += "&Ket=" + readingResults[2];
          spreadSheetSauce += "&Cre=" + readingResults[3];
          spreadSheetSauce += "&Blo=" + readingResults[4];
          spreadSheetSauce += "&Pro=" + readingResults[5];
          spreadSheetSauce += "&Mic=" + readingResults[6];
          spreadSheetSauce += "&Nit=" + readingResults[7];
          spreadSheetSauce += "&Leu=" + readingResults[8];
          spreadSheetSauce += "&Glu=" + readingResults[9];
          spreadSheetSauce += "&Spe=" + readingResults[10];
          spreadSheetSauce += "&PH=" + readingResults[11];
          spreadSheetSauce += "&Asc=" + readingResults[12];
          spreadSheetSauce += "&Cal=" + readingResults[13];
          
          HTTPClient https;
          https.setInsecure();

          Serial.println(spreadSheetSauce);

          // Serial.println("WiFi Status: ");
          // Serial.print(WiFi.status());

          https.begin(spreadSheetSauce.c_str());  
          https.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
          uint16_t httpsCode = https.GET();

          if (httpsCode > 0) {
            Serial.printf("[HTTP] GET... code: %d\n", httpsCode);
            String payload = https.getString();
            Serial.println(payload);
          } else {
              Serial.printf("[HTTP] GET... failed, error: %s\n", https.errorToString(httpsCode).c_str());
          }
          https.end();

        } else {
          Serial.print("Wifi Error. Maybe not connected");
        }

      }
}

void pullData(){
  
  if (WiFi.status() == WL_CONNECTED) {
    isDoingHttpRequest = true;
    HTTPClient https;
    https.setInsecure();


    String DataFromSpreadSheet = "https://script.google.com/macros/s/AKfycbwmgh1AfqVb4SIIjmTSDB4eWfBUyKKaeWM9F9uUVn1V7diQe5VIXqcm3yuGE7ZwWHQq/exec?sts=read";

    https.begin(DataFromSpreadSheet.c_str());  
    https.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    int httpsCode = https.GET();

    if (httpsCode > 0) {
      Serial.printf("[HTTP] GET... code: %d\n", httpsCode);
      String payload = https.getString();
    
    uint32_t commaPlace = 0;
    for(int j=0; j < 150 ; j++){
      
      for(int i = 0; i <18; i++){

        PatientsRecordsInSpreadSheet[j][i]= String(extractValues(payload, ',', commaPlace));
        commaPlace++;
      }
    }
    
      // printing 
    for(int j=0; j < 150 ;j++){
      for(int i = 0; i < 18; i++){
        Serial.print(" ");
        Serial.print(PatientsRecordsInSpreadSheet[j][i]);
      }
      Serial.println();
    }

      Serial.println(payload);
    } else {
        Serial.printf("[HTTP] GET... failed, error: %s\n", https.errorToString(httpsCode).c_str());
    }

    https.end();
    // WiFi.disconnect();
    isDoingHttpRequest = false;

  } else {
    Serial.println("wifi error");
  }
}


String extractValues(String data, char separator, int index) {

  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;
  
  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}



// Urobilinogen
String getURU(uint16_t r, uint16_t g, uint16_t b) {  //(add minus 20)
  if ((r >= 240 && r <= 255) && (g >= 223 && g <= 263) && (b >= 181 && b <= 221)) {
    return "3.3";
  } else if ((r >= 235 && r <= 275) && (g >= 156 && g <= 196) && (b >= 131 && b <= 171)) {
    return "16";
  } else if ((r >= 232 && r <= 272) && (g >= 109 && g <= 149) && (b >= 94 && b <= 134)) {
    return "32";
  } else if ((r >= 234 && r <= 274) && (g >= 89 && g <= 129) && (b >= 72 && b <= 112)) {
    return "64";
  } else if ((r >= 227 && r <= 247) && (g >= 73 && g <= 113) && (b >= 49 && b <= 89)) {
    return "128";
  } else if (g >= 210) {  //(range of green)
    return "3.3-";
  } else if (g >= 152 && g <= 209) {
    return "16-";
  } else if (g >= 120 && g <= 151) {
    return "32-";
  } else if (g >= 101 && g <= 119) {
    return "64-";
  } else if (g <= 100) {
    return "128-";
  } else {
    return "3.3--'";
  }
}
//Bilirubin
//Negative - rgb(255, 255, 231)
//Small(17) - rgb(246, 214, 163)
//Moderate(50) - rgb(216, 188, 148)
//Large(100) - rgb(222, 186, 152)
String getBIL(uint16_t r, uint16_t g, uint16_t b) {  //(add minus 20)
  if ((r >= 235 && r <= 275) && (g >= 215 && g <= 255) && (b >= 211 && b <= 251)) {
    return "Negative";
  } else if ((r >= 223 && r <= 263) && (g >= 196 && g <= 236) && (b >= 143 && b <= 183)) {
    return "Small(17)";
  } else if ((r >= 197 && r <= 237) && (g >= 173 && g <= 213) && (b >= 128 && b <= 168)) {
    return "Moderate(50)";
  } else if ((r >= 209 && r <= 249) && (g >= 171 && g <= 211) && (b >= 132 && b <= 172)) {
    return "Large(100)";
  } else {
    return "Negative--";
  }
}
//Ketone
//Negative - rgb(251, 181, 132)
//Trace(0.5) - rgb(254, 153, 123)
//Small(1.5) - rgb(248, 105, 125)
//Moderate(4) - rgb(201, 49, 88)
//Large(8) - rgb(135, 20, 75)
//Large(16) - rgb(93, 0, 57)
String getKET(uint16_t r, uint16_t g, uint16_t b) {  //addminus 20
  if ((r >= 231 && r <= 271) && (g >= 161 && g <= 201) && (b >= 112 && b <= 152)) {
    return "Negative";
  } else if ((r >= 234 && r <= 274) && (g >= 133 && g <= 153) && (b >= 103 && b <= 143)) {
    return "Trace(0.5)";
  } else if ((r >= 228 && r <= 268) && (g >= 85 && g <= 105) && (b >= 105 && b <= 145)) {
    return "Small(1.5)";
  } else if ((r >= 281 && r <= 221) && (g >= 29 && g <= 69) && (b >= 68 && b <= 108)) {
    return "Moderate";
  } else if ((r >= 115 && r <= 155) && (g >= 0 && g <= 40) && (b >= 55 && b <= 95)) {
    return "Large(8)";
  } else if ((r >= 73 && r <= 113) && (g >= -20 && g <= 20) && (b >= 37 && b <= 77)) {
    return "Large(16)";
  } else {
    return "Negative--";
  }
}
//Creatinine
// 0.9 - rgb(255, 242, 187)
// 4.4 - rgb(228, 207, 152)
// 8.8 - rgb(217, 188, 148)
//17.7 - rgb(201, 174, 145)
//26.5 - rgb(158, 122, 124)
String getCRE(uint16_t r, uint16_t g, uint16_t b) {
  if ((r >= 235 && r <= 275) && (g >= 222 && g <= 262) && (b >= 167 && b <= 207)) {
    return "0.9";
  } else if ((r >= 208 && r <= 248) && (g >= 187 && g <= 227) && (b >= 132 && b <= 172)) {
    return "4.4";
  } else if ((r >= 197 && r <= 237) && (g >= 168 && g <= 208) && (b >= 128 && b <= 168)) {
    return "8.8";
  } else if ((r >= 181 && r <= 221) && (g >= 154 && g <= 194) && (b >= 125 && b <= 165)) {
    return "17.7";
  } else if ((r >= 138 && r <= 178) && (g >= 102 && g <= 142) && (b >= 104 && b <= 144)) {
    return "26.5";
  } else {
    return "0.9--";
  }
}
//Blood
// Negative - rgb(255, 178, 26)
// Trace(10-NonHemo) - rgb(228, 178, 53)
// Trace(10-Hemo) - rgb(205, 163, 27)
// Small(25) - rgb(153, 147, 33)
// Moderate(80) - rgb(89, 125, 38)
// Large(200) - rgb(24, 74, 23)
String getBLO(uint16_t r, uint16_t g, uint16_t b) {
  if ((r >= 235 && r <= 275) && (g >= 158 && g <= 198) && (b >= 6 && b <= 46)) {
    return "Negative";
  } else if ((r >= 208 && r <= 248) && (g >= 158 && g <= 198) && (b >= 33 && b <= 73)) {
    return "Trace(10-NonHemo)";
  } else if ((r >= 185 && r <= 225) && (g >= 143 && g <= 183) && (b >= 7 && b <= 47)) {
    return "Trace(10-Hemo)";
  } else if ((r >= 133 && r <= 173) && (g >= 127 && g <= 167) && (b >= 13 && b <= 53)) {
    return "Small(25)";
  } else if ((r >= 69 && r <= 109) && (g >= 105 && g <= 145) && (b >= 18 && b <= 58)) {
    return "Moderate(80)";
  } else if ((r >= 4 && r <= 44) && (g >= 54 && g <= 94) && (b >= 3 && b <= 43)) {
    return "Large(200)";
  } else {
    return "Negative--";
  }
}
//Protein
// Negative -rgb(230, 247, 115)
// Trace -rgb(206, 235, 129)
// 0.3+ -rgb(177, 213, 123)
// 1++ -rgb(140, 197, 142)
// 3+++ -rgb(133, 193, 165)
// >20++++ -rgb(117, 179, 140)
String getPRO(uint16_t r, uint16_t g, uint16_t b) {
  if ((r >= 210 && r <= 250) && (g >= 227 && g <= 267) && (b >= 95 && b <= 135)) {
    return "Negative";
  } else if ((r >= 186 && r <= 226) && (g >= 215 && g <= 255) && (b >= 109 && b <= 149)) {
    return "Trace";
  } else if ((r >= 157 && r <= 197) && (g >= 193 && g <= 233) && (b >= 103 && b <= 143)) {
    return "0.3+";
  } else if ((r >= 120 && r <= 160) && (g >= 177 && g <= 217) && (b >= 122 && b <= 162)) {
    return "1++";
  } else if ((r >= 113 && r <= 153) && (g >= 173 && g <= 213) && (b >= 145 && b <= 185)) {
    return "3+++";
  } else if ((r >= 97 && r <= 137) && (g >= 159 && g <= 199) && (b >= 120 && b <= 160)) {
    return ">20++++";
  } else {
    return "Negative--";
  }
}
//Micro Albumin
// 10 -rgb(243, 252, 195)
// 30 -rgb(223, 244, 203)
// 80 -rgb(207, 234, 201)
// 150 -rgb(176, 227, 194)
String getMIC(uint16_t r, uint16_t g, uint16_t b) {
  if ((r >= 223 && r <= 263) && (g >= 232 && g <= 272) && (b >= 175 && b <= 215)) {
    return "10";
  } else if ((r >= 203 && r <= 243) && (g >= 224 && g <= 264) && (b >= 183 && b <= 223)) {
    return "30";
  } else if ((r >= 187 && r <= 227) && (g >= 214 && g <= 254) && (b >= 181 && b <= 221)) {
    return "80";
  } else if ((r >= 156 && r <= 196) && (g >= 207 && g <= 247) && (b >= 174 && b <= 214)) {
    return "150";
  } else {
    return "10--";
  }
}
//Nitrite
// Negative - rgb(255, 254, 233) White
// Positive - Any degree of uniform pink color (rgb(255, 231, 218)) to
// rgb(252, 220, 225))
String getNIT(uint16_t r, uint16_t g, uint16_t b) {
  if ((r >= 240 && r <= 255) && (g >= 210 && g <= 255) && (b >= 180 && b <= 220)) {
    return "Negative";
  } else if ((r >= 240 && r <= 252) && (g >= 152 && g <= 231) && (b >= 180 && b <= 225)) {
    return "Positive";
  } else {
    return "Negative'";
  }
}
//Leukocytes
//Negative - rgb(253, 252, 187)
//Trace(15) - rgb(229, 213, 187)
//Small(70) - rgb(212, 185, 166)
//Moderate(125) - rgb(180, 137, 157)
//Large(500) - rgb(140, 96, 149)
String getLEU(uint16_t r, uint16_t g, uint16_t b) {
  if ((r >= 233 && r <= 273) && (g >= 232 && g <= 272) && (b >= 167 && b <= 207)) {
    return "Negative";
  } else if ((r >= 209 && r <= 249) && (g >= 193 && g <= 233) && (b >= 167 && b <= 207)) {
    return "Trace(15)";
  } else if ((r >= 192 && r <= 232) && (g >= 165 && g <= 205) && (b >= 146 && b <= 186)) {
    return "Small(70)";
  } else if ((r >= 160 && r <= 200) && (g >= 117 && g <= 157) && (b >= 137 && b <= 177)) {
    return "Moderate(125)";
  } else if ((r >= 120 && r <= 160) && (g >= 76 && g <= 116) && (b >= 129 && b <= 169)) {
    return "Large(500)";
  } else {
    return "Negative--";
  }
}
//Glucose
//Negative - rgb(145, 201, 176)
//Trace(5) - rgb(155, 213, 126)
//15+ rgb(132, 164, 41)
//30++ rgb(157, 133, 7)
//60+++ rgb(138, 75, 22)
//100++++ rgb(108, 42, 7)
String getGLU(uint16_t r, uint16_t g, uint16_t b) {
  if ((r >= 125 && r <= 165) && (g >= 181 && g <= 221) && (b >= 156 && b <= 196)) {
    return "Negative";
  } else if ((r >= 135 && r <= 175) && (g >= 193 && g <= 233) && (b >= 106 && b <= 146)) {
    return "Trace(5)";
  } else if ((r >= 112 && r <= 152) && (g >= 144 && g <= 184) && (b >= 21 && b <= 61)) {
    return "15+";
  } else if ((r >= 137 && r <= 177) && (g >= 113 && g <= 153) && (b >= -13 && b <= 27)) {
    return "30++";
  } else if ((r >= 118 && r <= 158) && (g >= 55 && g <= 95) && (b >= 2 && b <= 42)) {
    return "60+++";
  } else if ((r >= 88 && r <= 128) && (g >= 22 && g <= 62) && (b >= -13 && b <= 27)) {
    return "100++++";
  } else {
    return "Negative--";
  }
}
//Specific Gravity
//1 -rgb(3, 90, 84)
//1.005 -rgb(79, 106, 71)
//1.01 -rgb(102, 123, 56)
//1.015 -rgb(153, 147, 51)
//1.02 -rgb(178, 165, 51)
//1.025 -rgb(205, 176, 38)
//1.03 -rgb(229, 195, 27)
String getSPE(uint16_t r, uint16_t g, uint16_t b) {
  if ((r >= -17 && r <= 23) && (g >= 70 && g <= 110) && (b >= 64 && b <= 104)) {
    return "1";
  } else if ((r >= 59 && r <= 99) && (g >= 86 && g <= 126) && (b >= 51 && b <= 91)) {
    return "1.005";
  } else if ((r >= 82 && r <= 122) && (g >= 103 && g <= 143) && (b >= 36 && b <= 76)) {
    return "1.01";
  } else if ((r >= 133 && r <= 173) && (g >= 127 && g <= 167) && (b >= 31 && b <= 71)) {
    return "1.015";
  } else if ((r >= 158 && r <= 198) && (g >= 145 && g <= 185) && (b >= 31 && b <= 71)) {
    return "1.02";
  } else if ((r >= 185 && r <= 225) && (g >= 156 && g <= 196) && (b >= 18 && b <= 58)) {
    return "1.025";
  } else if ((r >= 209 && r <= 249) && (g >= 175 && g <= 215) && (b >= 7 && b <= 47)) {
    return "1.03";
  } else {
    return "1.015--";
  }
}
//PH Level
//5 -rgb(247, 197, 4)
//6 -rgb(173, 175, 14)
//6.5 -rgb(141, 163, 18)
//7 -rgb(114, 132, 20)
//7.5 -rgb(93, 125, 24)
//8 -rgb(63, 100, 30)
//8.5 -rgb(9, 62, 102)
String getPH(uint16_t r, uint16_t g, uint16_t b) {
  if ((r >= 227 && r <= 267) && (g >= 177 && g <= 217) && (b >= -16 && b <= 24)) {
    return "5";
  } else if ((r >= 153 && r <= 193) && (g >= 155 && g <= 195) && (b >= -6 && b <= 34)) {
    return "6";
  } else if ((r >= 121 && r <= 161) && (g >= 143 && g <= 183) && (b >= -2 && b <= 38)) {
    return "6.5";
  } else if ((r >= 94 && r <= 134) && (g >= 112 && g <= 152) && (b >= 0 && b <= 40)) {
    return "7";
  } else if ((r >= 73 && r <= 113) && (g >= 105 && g <= 145) && (b >= 4 && b <= 44)) {
    return "7.5";
  } else if ((r >= 43 && r <= 83) && (g >= 80 && g <= 120) && (b >= 10 && b <= 50)) {
    return "8";
  } else if ((r >= -11 && r <= 29) && (g >= 42 && g <= 82) && (b >= 82 && b <= 122)) {
    return "8.5";
  } else {
    return "5--";
  }
}
//Ascorbate
//0 -rgb(5, 64, 108)
//0.6 -rgb(90, 168, 144)
//1.4 -rgb(150, 188, 151)
//2.8 -rgb(213, 222, 141)
//5 -rgb(229, 230, 136)
String getASC(uint16_t r, uint16_t g, uint16_t b) {
  if ((r >= -15 && r <= 45) && (g >= 44 && g <= 84) && (b >= 88 && b <= 128)) {
    return "0";
  } else if ((r >= 70 && r <= 110) && (g >= 148 && g <= 188) && (b >= 124 && b <= 164)) {
    return "0.6";
  } else if ((r >= 130 && r <= 170) && (g >= 168 && g <= 208) && (b >= 131 && b <= 171)) {
    return "1.4";
  } else if ((r >= 193 && r <= 233) && (g >= 202 && g <= 242) && (b >= 121 && b <= 161)) {
    return "2.8";
  } else if ((r >= 209 && r <= 249) && (g >= 210 && g <= 250) && (b >= 116 && b <= 156)) {
    return "5";
  } else {
    return "Unknown";
  }
}
//Calcium
//<=1.0 -rgb(254, 243, 239)
//2.5 -rgb(248, 230, 230)
//5 -rgb(240, 204, 216)
//7.5 -rgb(231, 171, 205)
//(>=)10 -rgb(190, 140, 191)
String getCAL(uint16_t r, uint16_t g, uint16_t b) {
  if ((r >= 234 && r <= 274) && (g >= 223 && g <= 263) && (b >= 219 && b <= 259)) {
    return "<=1.0";
  } else if ((r >= 228 && r <= 268) && (g >= 210 && g <= 250) && (b >= 210 && b <= 250)) {
    return "2.5";
  } else if ((r >= 220 && r <= 260) && (g >= 184 && g <= 224) && (b >= 196 && b <= 236)) {
    return "5";
  } else if ((r >= 211 && r <= 251) && (g >= 151 && g <= 191) && (b >= 185 && b <= 225)) {
    return "7.5";
  } else if ((r >= 170 && r <= 210) && (g >= 120 && g <= 160) && (b >= 171 && b <= 211)) {
    return ">=10";
  } else {
    return "Unknown";
  }
}

// =========================




// dont edit this, it sets the orientation of touch for the prefered display
void touch_calibrate() {
  uint16_t calData[5];
  uint8_t calDataOK = 0;

  // check file system exists
  if (!FILESYSTEM.begin()) {
    Serial.println("Formating file system");
    FILESYSTEM.format();
    FILESYSTEM.begin();
  }

  // check if calibration file exists and size is correct
  if (FILESYSTEM.exists(CALIBRATION_FILE)) {
    if (REPEAT_CAL) {
      // Delete if we want to re-calibrate
      FILESYSTEM.remove(CALIBRATION_FILE);
    } else {
      File f = FILESYSTEM.open(CALIBRATION_FILE, "r");
      if (f) {
        if (f.readBytes((char *)calData, 14) == 14)
          calDataOK = 1;
        f.close();
      }
    }
  }

  if (calDataOK && !REPEAT_CAL) {
    // calibration data valid
    tft.setTouch(calData);
  } else {
    // data not valid so recalibrate
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(30, 150);
    tft.setTextFont(1);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    tft.println("Calibrating touch. Click the arrow.");

    tft.setTextFont(1);
    tft.println();

    if (REPEAT_CAL) {
      // tft.setTextColor(TFT_RED, TFT_BLACK);
      // tft.println("Set REPEAT_CAL to false to stop this running again!");
    }

    tft.calibrateTouch(calData, TFT_YELLOW, TFT_BLACK, 12);//15

    // tft.setTextColor(TFT_GREEN, TFT_BLACK);
    // tft.println("Calibration complete!");

    // store data
    File f = FILESYSTEM.open(CALIBRATION_FILE, "w");
    if (f) {
      f.write((const unsigned char *)calData, 14);
      f.close();
    }
  }
}