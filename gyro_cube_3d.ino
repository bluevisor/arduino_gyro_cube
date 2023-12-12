#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

// OLED display width and height for 128x32
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1

// Create display and MPU6050 objects
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_MPU6050 mpu;

  struct Point2D {
    float x, y;
  };

  struct Point3D {
    float x, y, z;
  };

float zoomFactor = 6;  // Adjust as needed, >1 for zoom in, <1 for zoom out

int frameCount = 0;

Point2D project(Point3D point3D) {
  float zFactor = 1 / (point3D.z + 8);  // Perspective projection
  Point2D point2D;
  point2D.x = point3D.x * zFactor * zoomFactor;
  point2D.y = point3D.y * zFactor * zoomFactor;
  return point2D;
}

Point3D cubeVertices[8] = {
  { -1, -1, -1 }, { 1, -1, -1 }, { 1, 1, -1 }, { -1, 1, -1 }, { -1, -1, 1 }, { 1, -1, 1 }, { 1, 1, 1 }, { -1, 1, 1 }
};

Point3D rotateX(Point3D point, float angle) {
  Point3D rotated;
  rotated.x = point.x;
  rotated.y = point.y * cos(angle) - point.z * sin(angle);
  rotated.z = point.y * sin(angle) + point.z * cos(angle);
  return rotated;
}

Point3D rotateY(Point3D point, float angle) {
  Point3D rotated;
  rotated.x = point.x * cos(angle) + point.z * sin(angle);
  rotated.y = point.y;
  rotated.z = -point.x * sin(angle) + point.z * cos(angle);
  return rotated;
}

Point3D rotateZ(Point3D point, float angle) {
  Point3D rotated;
  rotated.x = point.x * cos(angle) - point.y * sin(angle);
  rotated.y = point.x * sin(angle) + point.y * cos(angle);
  rotated.z = point.z;
  return rotated;
}

int cubeEdges[12][2] = {
  { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 }, { 4, 5 }, { 5, 6 }, { 6, 7 }, { 7, 4 }, { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 }
};

void drawCube() {
  float aspectRatio = (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT;

  for (int i = 0; i < 12; i++) {
    Point2D p1 = project(cubeVertices[cubeEdges[i][0]]);
    Point2D p2 = project(cubeVertices[cubeEdges[i][1]]);

    // Correct aspect ratio
    p1 = correctAspectRatio(p1, aspectRatio);
    p2 = correctAspectRatio(p2, aspectRatio);

    // Scale and translate the points to fit the OLED screen
    int x1 = (p1.x + 1) * (SCREEN_WIDTH / 2);
    int y1 = (1 - p1.y) * (SCREEN_HEIGHT / 2);
    int x2 = (p2.x + 1) * (SCREEN_WIDTH / 2);
    int y2 = (1 - p2.y) * (SCREEN_HEIGHT / 2);

    display.drawLine(x1, y1, x2, y2, WHITE);
  }
}

Point2D correctAspectRatio(Point2D point, float aspectRatio) {
  Point2D correctedPoint;
  correctedPoint.x = point.x / aspectRatio;
  correctedPoint.y = point.y;
  return correctedPoint;
}

void calculatePitchAndRoll(float ax, float ay, float az, float& pitch, float& roll) {
  pitch = atan2(ay, az) * 180.0 / M_PI;
  roll = atan2(-ax, sqrt(ay * ay + az * az)) * 180.0 / M_PI;
}

void setup() {
  // Start the I2C communication
  Wire.begin();

  // Initialize the MPU6050
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }

  // Initialize the OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 allocation failed");
    for (;;)
      ;  // Don't proceed, loop forever
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
}

void loop() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  float pitch, roll;
  calculatePitchAndRoll(a.acceleration.x, a.acceleration.y, a.acceleration.z, pitch, roll);

  // Apply pitch and roll to reset cube orientation
  // Note: This resets the cube's orientation based on accelerometer
  // Adjust the rotations to fit your cube's coordinate system
  for (int i = 0; i < 8; i++) {
    cubeVertices[i] = rotateX(cubeVertices[i], pitch * DEG_TO_RAD * -.5);
    cubeVertices[i] = rotateY(cubeVertices[i], roll * DEG_TO_RAD * -.5);
  }
  // Use gyroscope data for continuous rotation
  float gyroX = g.gyro.x;
  float gyroY = g.gyro.y;
  float gyroZ = g.gyro.z;

  // Convert gyro data to rotation angles (example, might need tuning)
  float angleX = gyroX * 0.03;  // Adjust scaling factor as needed
  float angleY = gyroY * 0.03;
  float angleZ = gyroZ * 0.03;

  // Rotate cube vertices
  for (int i = 0; i < 8; i++) {
    cubeVertices[i] = rotateX(cubeVertices[i], angleX);
    cubeVertices[i] = rotateY(cubeVertices[i], angleY);
    cubeVertices[i] = rotateZ(cubeVertices[i], angleZ);
  }
  // }

  // Draw the cube
  display.clearDisplay();
  drawCube();
  display.display();
  delay(10);
}
