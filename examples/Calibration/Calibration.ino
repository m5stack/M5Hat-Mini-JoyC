// ============================================================================
// M5Stick-C Plus MiniJoyC Calibration Program
// ============================================================================

#include "M5Unified.h"
#include "M5HatMiniJoyC.h"

#define MiniJoyC_SDA 0 
#define MiniJoyC_SCL 26

#define SAMPLE_TIMES 100    // Number of samples for center calibration
#define XY_CAL_TIMES 10     // Times to sweep all 4 edges

#define X_DEFAULT_MIN 500    // Default minimum X axis value
#define X_DEFAULT_MAX 3600  // Default maximum X axis value
#define Y_DEFAULT_MIN 500   // Default minimum Y axis value
#define Y_DEFAULT_MAX 3600  // Default maximum Y axis value
#define MID_DEFAULT 2000    // Default center/middle value

enum { 
  CAL_CENTER = 0,     // State 0: Center calibration
  CAL_CENTER_DONE,    // State 1: Center calibration done
  CAL_XY_RANGE,       // State 2: XY axis range calibration
  CAL_DONE,           // State 3: Calibration complete
  CAL_VERIFY          // State 4: Verify calibration results
};
uint8_t cal_state = CAL_CENTER;  // Current calibration state

M5HatMiniJoyC joyc;    // Joystick object

static void waitMiniJoyCReady() {
  while (!joyc.begin(&Wire, MiniJoyC_ADDR, MiniJoyC_SDA, MiniJoyC_SCL, 100000UL)) {
    delay(100);  // Retry every 100ms
  }
}

static void drawHeader(const char* title) {
  M5.Display.fillScreen(BLACK);                    // Fill black background
  M5.Display.setCursor(0, 0);                      // Set cursor position
  M5.Display.setTextColor(YELLOW, BLACK);          // Set text color to yellow
  M5.Display.printf("%s", title);                  // Display title
  M5.Display.drawLine(5, 30, 130, 30, ORANGE);    // Draw orange separator line
}

// Center Point Calibration
static void cal_center() {
  waitMiniJoyCReady();  // Wait for joystick ready

  // Center coordinates and extreme values
  int16_t adc_cen_x, adc_cen_y;       // Center X, Y values
  int16_t adc_max_x = 0, adc_max_y = 0;       // X, Y max values
  int16_t adc_min_x = 4095, adc_min_y = 4095; // X, Y min values
  
  // Sample arrays
  int16_t adc_x_set[SAMPLE_TIMES];    // X axis samples
  int16_t adc_y_set[SAMPLE_TIMES];    // Y axis samples
  int32_t sum_x = 0, sum_y = 0;       // X, Y sum

  drawHeader("CenterCal");                        // Draw header
  M5.Display.setCursor(0, 40);                   // Set cursor position
  M5.Display.setTextColor(WHITE, BLACK);         // White text
  M5.Display.printf("Sampling...");             // Display sampling message

  // Collect SAMPLE_TIMES samples
  for (int i = 0; i < SAMPLE_TIMES; i++) {
    adc_x_set[i] = joyc.getADCValue(ADC_X);    // Read X axis ADC value
    adc_y_set[i] = joyc.getADCValue(ADC_Y);    // Read Y axis ADC value
    delay(5);                                    // 5ms delay
  }

  // Calculate min, max and sum
  for (int i = 0; i < SAMPLE_TIMES; i++) {
    adc_min_x = min(adc_min_x, adc_x_set[i]);  // Update X min
    adc_max_x = max(adc_max_x, adc_x_set[i]);  // Update X max
    adc_min_y = min(adc_min_y, adc_y_set[i]);  // Update Y min
    adc_max_y = max(adc_max_y, adc_y_set[i]);  // Update Y max
    sum_x += adc_x_set[i];                      // Accumulate X value
    sum_y += adc_y_set[i];                      // Accumulate Y value
  }

  // Calculate center value (exclude min and max)
  // average = (sum - min - max) / (sample_count - 2)
  adc_cen_x = (sum_x - adc_min_x - adc_max_x) / (SAMPLE_TIMES - 2);
  adc_cen_y = (sum_y - adc_min_y - adc_max_y) / (SAMPLE_TIMES - 2);

  // Write center calibration values to joystick
  joyc.setOneCalValue(Cal_X_cen, adc_cen_x);  // Set X center value
  joyc.setOneCalValue(Cal_Y_cen, adc_cen_y);  // Set Y center value

  // Transition to center calibration done state
  cal_state = CAL_CENTER_DONE;
}

// Display Center Calibration Results
static void cal_center_done() {
  drawHeader("CenterCal");  // Draw header

  M5.Display.setTextColor(WHITE, BLACK); // White text

  // Display real-time raw ADC values
  M5.Display.setCursor(0, 40);
  M5.Display.printf("RawX:%d", joyc.getADCValue(ADC_X));  // X raw value
  M5.Display.setCursor(0, 70);
  M5.Display.printf("RawY:%d", joyc.getADCValue(ADC_Y));  // Y raw value

  M5.Display.drawLine(5, 105, 130, 105, ORANGE);  // Separator line

  // Display calibrated center values
  M5.Display.setTextColor(GREEN, BLACK);  // Green text
  M5.Display.setCursor(0, 115);
  M5.Display.printf("CalX:%d", joyc.getCalValue(Cal_X_cen));  // X calibrated value
  M5.Display.setCursor(0, 145);
  M5.Display.printf("CalY:%d", joyc.getCalValue(Cal_Y_cen));  // Y calibrated value

  M5.Display.drawLine(5, 175, 130, 175, ORANGE);  // Separator line

  // Display operation menu
  M5.Display.setTextColor(WHITE, BLACK);  // White text
  M5.Display.setCursor(0, 185);
  M5.Display.printf("BtnA:CalXY");  // Button A: Go to XY calibration
  M5.Display.setCursor(0, 210);
  M5.Display.printf("BtnB:Recal");  // Button B: Recalibrate center
}

// XY Axis Range Calibration
static void cal_xy_range() {
  waitMiniJoyCReady();  // Wait for joystick ready

  int16_t adc_min_x = joyc.getCalValue(Cal_X_min);  // X minimum
  int16_t adc_max_x = joyc.getCalValue(Cal_X_max);  // X maximum
  int16_t adc_min_y = joyc.getCalValue(Cal_Y_min);  // Y minimum
  int16_t adc_max_y = joyc.getCalValue(Cal_Y_max);  // Y maximum
  int16_t adc_cen_x = joyc.getCalValue(Cal_X_cen);  // X center
  int16_t adc_cen_y = joyc.getCalValue(Cal_Y_cen);  // Y center

  uint16_t cal_times = 0;  // Number of complete sweeps
  uint8_t cal_flag_x_min = 0, cal_flag_x_max = 0, cal_flag_y_min = 0, cal_flag_y_max = 0;

  while (true) {
    M5.update();  // Update M5 input status

    int16_t adc_x = joyc.getADCValue(ADC_X);  // X axis value
    int16_t adc_y = joyc.getADCValue(ADC_Y);  // Y axis value

    if (adc_x < adc_min_x) adc_min_x = adc_x;  // Update X min
    if (adc_x > adc_max_x) adc_max_x = adc_x;  // Update X max
    if (adc_y < adc_min_y) adc_min_y = adc_y;  // Update Y min
    if (adc_y > adc_max_y) adc_max_y = adc_y;  // Update Y max

    if (adc_x < X_DEFAULT_MIN) cal_flag_x_min = 1;  // Reached X minimum
    if (adc_x > X_DEFAULT_MAX) cal_flag_x_max = 1;  // Reached X maximum
    if (adc_y < Y_DEFAULT_MIN) cal_flag_y_min = 1;  // Reached Y minimum
    if (adc_y > Y_DEFAULT_MAX) cal_flag_y_max = 1;  // Reached Y maximum

    if (cal_flag_x_max && cal_flag_x_min && cal_flag_y_min && cal_flag_y_max) {
      cal_times++;  // Increment completion count
      cal_flag_x_min = cal_flag_x_max = cal_flag_y_min = cal_flag_y_max = 0;
      if (cal_times > XY_CAL_TIMES) cal_times = XY_CAL_TIMES;
    }

    drawHeader("XYCal");                         // Draw header
    M5.Display.setTextColor(WHITE, BLACK);       // White text
    M5.Display.setCursor(0, 50);
    M5.Display.printf("Remain:");               // Display "Remain"

    M5.Display.setTextColor(GREEN, BLACK);       // Green text
    M5.Display.setCursor(0, 80);
    M5.Display.printf("%d", (int)(XY_CAL_TIMES - cal_times));  // Show remaining times

    M5.Display.setTextColor(WHITE, BLACK);       // White text
    M5.Display.setCursor(0, 120);
    M5.Display.printf("Push to 4 edges repeatedly");  // Show instruction

    if (cal_times >= XY_CAL_TIMES) break;
    delay(30);  // 30ms delay
  }

  uint16_t adc_cal_data[6];
  adc_cal_data[0] = adc_min_x;    // X minimum
  adc_cal_data[1] = adc_max_x;    // X maximum
  adc_cal_data[2] = adc_min_y;    // Y minimum
  adc_cal_data[3] = adc_max_y;    // Y maximum
  adc_cal_data[4] = adc_cen_x;    // X center
  adc_cal_data[5] = adc_cen_y;    // Y center

  joyc.setAllCalValue(adc_cal_data);
  delay(300);  // Wait for joystick to process

  cal_state = CAL_DONE;
}

// Display Calibration Complete
static void cal_done() {
  drawHeader("CalDone");  // Draw header

  M5.Display.setTextColor(GREEN, BLACK);  // Green text

  M5.Display.setCursor(0, 40);
  M5.Display.printf("MinX:%d", joyc.getCalValue(Cal_X_min));  // X min
  M5.Display.setCursor(0, 65);
  M5.Display.printf("MaxX:%d", joyc.getCalValue(Cal_X_max));  // X max
  M5.Display.setCursor(0, 90);
  M5.Display.printf("MinY:%d", joyc.getCalValue(Cal_Y_min));  // Y min
  M5.Display.setCursor(0, 115);
  M5.Display.printf("MaxY:%d", joyc.getCalValue(Cal_Y_max));  // Y max

  M5.Display.drawLine(5, 165, 130, 165, ORANGE);  // Separator line

  M5.Display.setTextColor(WHITE, BLACK);  // White text
  M5.Display.setCursor(0, 180);
  M5.Display.printf("BtnA:Verify");         // Button A: Verify
  M5.Display.setCursor(0, 205);
  M5.Display.printf("BtnB:ReCalXY");        // Button B: Recalibrate XY
}

// Calibration Verification
static void cal_verify() {
  int8_t pos_x = joyc.getPOSValue(POS_X, _8bit);  // X axis position
  int8_t pos_y = joyc.getPOSValue(POS_Y, _8bit);  // Y axis position

  drawHeader("Verify");  // Draw header

  M5.Display.setTextColor(GREEN, BLACK);  // Green text

  M5.Display.setCursor(0, 50);
  M5.Display.printf("PosX:%d", pos_x);  // X position (-128~127)
  M5.Display.setCursor(0, 80);
  M5.Display.printf("PosY:%d", pos_y);  // Y position (-128~127)

  M5.Display.drawLine(5, 165, 130, 165, ORANGE);  // Separator line

  M5.Display.setTextColor(WHITE, BLACK);  // White text
  M5.Display.setCursor(0, 180);
  M5.Display.printf("BtnA:CalCen");           // Button A: Next
  M5.Display.setCursor(0, 205);
  M5.Display.printf("BtnB:seeVal");          // Button B: See Calibration Values
}

// Restore Default Calibration Values
static void recal_defaults() {
  uint16_t adc_cal_data[6];
  adc_cal_data[0] = X_DEFAULT_MIN;  // X min default
  adc_cal_data[1] = X_DEFAULT_MAX;  // X max default
  adc_cal_data[2] = Y_DEFAULT_MIN;  // Y min default
  adc_cal_data[3] = Y_DEFAULT_MAX;  // Y max default
  adc_cal_data[4] = MID_DEFAULT;    // X center default
  adc_cal_data[5] = MID_DEFAULT;    // Y center default
  
  joyc.setAllCalValue(adc_cal_data);
  delay(300);
}

void setup() {
  M5.begin();  
  waitMiniJoyCReady();// Wait for MiniJoyC to be ready

  M5.Display.setRotation(0);
  M5.Display.setFont(&fonts::FreeMonoBold9pt7b);

  cal_state = CAL_VERIFY;
}

void loop() {
  M5.update();  // Update M5 input status

  // Button A advances calibration flow
  if (M5.BtnA.wasPressed()) {
    if (cal_state == CAL_CENTER_DONE) 
      cal_state = CAL_XY_RANGE;        // Go to XY calibration
    else if (cal_state == CAL_DONE) 
      cal_state = CAL_VERIFY;    // Go to verification
    else if (cal_state == CAL_VERIFY) 
      cal_state = CAL_CENTER;    // Return to center calibration
  } 
  // Button B goes back or performs special operations
  else if (M5.BtnB.wasPressed()) {
    if (cal_state == CAL_CENTER_DONE) 
      cal_state = CAL_CENTER;    // Restart center calibration
    else if (cal_state == CAL_DONE) 
      cal_state = CAL_XY_RANGE;        // Return to XY calibration
    else if (cal_state == CAL_VERIFY) {
      cal_state = CAL_DONE;    // Return to center calibration
    }
  }

  if (!joyc.getButtonStatus())  { // MiniJoyC button pressed
    recal_defaults();          // Restore defaults
    cal_state = CAL_VERIFY;    // Return to center calibration
  }

  // Execute corresponding function based on state
  switch (cal_state) {
    case CAL_CENTER:      
      cal_center();                      // Perform center calibration
      break;
    case CAL_CENTER_DONE: 
      cal_center_done();
      joyc.setLEDColor(0xFFFF00);  // Yellow LED
      delay(50);
      break;
    case CAL_XY_RANGE:          
      cal_xy_range();                    // Perform XY range calibration
      joyc.setLEDColor(0x0000FF);  // BLUE LED
      break;
    case CAL_DONE:        
      cal_done();
      joyc.setLEDColor(0x00FF00);  // GREEN LED
      delay(80);
      break;
    case CAL_VERIFY:      
      cal_verify();
      joyc.setLEDColor(0xFFFFFF);  // GREEN LED
      delay(30);
      break;
    default:              
      cal_state = CAL_VERIFY;            // Reset on invalid state
      break;
  }
}
