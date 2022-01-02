#include <Adafruit_GFX.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_ILI9341.h>
#include <TSC2004.h>
#include <Adafruit_NeoPixel.h>
#include <BBQ10Keyboard.h>
#include <SD.h>

#define TFT_CS 9
#define TFT_DC 10
#define TFT_MOSI 11
#define TFT_CLK 13
#define TFT_RST 8
#define TFT_MISO 12
#define SD_CS 5
#define NEOPIXEL_PIN 11

#define TS_MINX 150
#define TS_MINY 940
#define TS_MAXX 920
#define TS_MAXY 120

// The touch controller is interfaced over I2C
TSC2004 ts;

Adafruit_ILI9341 tft(TFT_CS, TFT_DC);
Adafruit_NeoPixel pixels(1, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
BBQ10Keyboard keyboard;
File root;
int touch;
int tftWidth, tftHeight;
bool isSdInserted = false;

// Test if the NeoPixel is working.
bool testNeoPixel() {
  pixels.setPixelColor(0, pixels.Color(0, 255, 0));
  pixels.show();
  delay(500);
  pixels.clear();
  delay(500);
  pixels.show();
  delay(500);
  pixels.clear();
  return true;
}

void hardwareTests()
{
  tft.print("  --- HARDWARE CHECKS ---\n\n");
  tft.print("[1] Detecting TFT Screen\n");
  tft.setTextColor(ILI9341_GREEN);
  tft.print("    Passed\n");
  
  tft.setTextColor(ILI9341_WHITE);
  delay(1000);
  
  tft.print("[2] Reading MicroSD\n");
  const bool sd = SD.begin(SD_CS);
  // Test check if a SD card is inserted.
  if (sd){
         isSdInserted = true;
         tft.setTextColor(ILI9341_GREEN);
         tft.print("    Passed\n");
  } else {
     tft.setTextColor(ILI9341_RED);
     tft.print("    Failed\n"); 
  }
  
  tft.setTextColor(ILI9341_WHITE);
  delay(1000);

  // Test front NeoPixel
  tft.print("[3] Flashing NeoPixel\n");

  if (testNeoPixel()) {
    tft.setTextColor(ILI9341_GREEN);
    tft.print("    Passed\n");
  } else {
    tft.setTextColor(ILI9341_RED);
    tft.print("    Failed\n");
  }

  tft.setTextColor(ILI9341_WHITE);
  delay(1000);
  
}

void loadMenu(){
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.fillScreen(ILI9341_BLACK);
  tft.fillRect(0,0,100,100, ILI9341_GREEN);
  tft.fillRect(100,0,100,100, ILI9341_RED);
  tft.fillRect(100,0,100,100, ILI9341_BLUE);
  tft.setCursor(8,45);
  tft.println("LED ON");
  tft.setCursor(128,45);
  tft.println("LEDOFF");
}

void listSDCard(){
  // List SD card files if available
    tft.print("  --- MicroSD Files ---\n\n");
    root = SD.open("/");
    while (true) {
      File entry =  root.openNextFile();
      if (!entry)
        break;
      tft.println(entry.name());
      entry.close();
    }
  }

  void touchManager(){
    // See if there's any  touch data for us
    if (ts.bufferEmpty()) {
      return;
    }
    
    TS_Point p = ts.getPoint();
    Serial.print("X = "); Serial.print(p.x);
    Serial.print("\tY = "); Serial.print(p.y);
    Serial.print("\tPressure = "); Serial.println(p.z);
    
    // Scale from ~0->4000 to tft.width using the calibration #'s
    p.x = map(p.x, TS_MINX, TS_MAXX, tft.width(), 0);
    p.y = map(p.y, TS_MINY, TS_MAXY, tft.height(), 0);
    
    if (p.x > 0 && p.x < 120) {
      if (p.y > 0 && p.y < 120) {
        touch = 1;
      }
    }
    if (p.x > 120 && p.x < 240) {
      if (p.y > 0 && p.y < 120) {
        touch = 2;
      }
    }
  
    if (touch == 1) {
        pixels.setPixelColor(0, pixels.Color(0, 255, 0));
        pixels.show();
        delay(500);
    }
  
    if (touch == 2) {
        pixels.clear();
        pixels.show();
        delay(500);
    }
  }



  class BitmapHandler {
  private:
    bool fileOK = false;
    File bmpFile;
    String bmpFilename;

    uint8_t read8Bit(){
      if (!this->bmpFile) {
       return 0;
      }
      else {
        return this->bmpFile.read();
      }
    }

    uint16_t read16Bit(){
      uint16_t lsb, msb;
      if (!this->bmpFile) {
       return 0;
      }
      else {
        lsb = this->bmpFile.read();
        msb = this->bmpFile.read();
        return (msb<<8) + lsb;
      }
    }

    uint32_t read32Bit(){
      uint32_t lsb, b2, b3, msb;
      if (!this->bmpFile) {
       return 0;
      }
      else {
        lsb = this->bmpFile.read();
        b2 = this->bmpFile.read();
        b3 = this->bmpFile.read();
        msb = this->bmpFile.read();
        return (msb<<24) + (b3<<16) + (b2<<8) + lsb;
      }
    }
    
  public:

    // BMP header fields
    uint16_t headerField;
    uint32_t fileSize;
    uint32_t imageOffset;
    // DIB header
    uint32_t headerSize;
    uint32_t imageWidth;
    uint32_t imageHeight;
    uint16_t colourPlanes;
    uint16_t bitsPerPixel;
    uint32_t compression;
    uint32_t imageSize;
    uint32_t xPixelsPerMeter;
    uint32_t yPixelsPerMeter;
    uint32_t totalColors;
    uint32_t importantColors;

    BitmapHandler(String filename){
      this->fileOK = false;
      this->bmpFilename = filename;
      this->bmpFile = SD.open(this->bmpFilename, FILE_READ);
      if (!this->bmpFile) {
        Serial.print(F("BitmapHandler : Unable to open file "));
        Serial.println(this->bmpFilename);
        this->fileOK = false;
      }
      else {
        if (!this->readFileHeaders()){
          Serial.println(F("Unable to read file headers"));
          this->fileOK = false;
        }
        else {
          if (!this->checkFileHeaders()){
            Serial.println(F("Not compatible file"));
            this->fileOK = false;
          }
          else {
            // all OK
            Serial.println(F("BMP file all OK"));
            this->fileOK = true;
          }       
        }
        // close file
        this->bmpFile.close();
      }
    }
    
    bool readFileHeaders(){
      if (this->bmpFile) {
        // reset to start of file
        this->bmpFile.seek(0);
        
        // BMP Header
        this->headerField = this->read16Bit();
        this->fileSize = this->read32Bit();
        this->read16Bit(); // reserved
        this->read16Bit(); // reserved
        this->imageOffset = this->read32Bit();

        // DIB Header
        this->headerSize = this->read32Bit();
        this->imageWidth = this->read32Bit();
        this->imageHeight = this->read32Bit();
        this->colourPlanes = this->read16Bit();
        this->bitsPerPixel = this->read16Bit();
        this->compression = this->read32Bit();
        this->imageSize = this->read32Bit();
        this->xPixelsPerMeter = this->read32Bit();
        this->yPixelsPerMeter = this->read32Bit();
        this->totalColors = this->read32Bit();
        this->importantColors = this->read32Bit();

        // close file
        return true;
      }
      else {
        return false;
      }
    }

    bool checkFileHeaders(){

      // BMP file id
      if (this->headerField != 0x4D42){
        return false;
      }
      // must be single colour plane
      if (this->colourPlanes != 1){
        return false;
      }
      // only working with 24 bit bitmaps
      if (this->bitsPerPixel != 24){
        return false;
      }
      // no compression
      if (this->compression != 0){
        return false;
      }
      // all ok
      return true;
    }

    void serialPrintHeaders() {
      Serial.print(F("filename : "));
      Serial.println(this->bmpFilename);
      // BMP Header
      Serial.print(F("headerField : "));
      Serial.println(this->headerField, HEX);
      Serial.print(F("fileSize : "));
      Serial.println(this->fileSize);
      Serial.print(F("imageOffset : "));
      Serial.println(this->imageOffset);
      Serial.print(F("headerSize : "));
      Serial.println(this->headerSize);
      Serial.print(F("imageWidth : "));
      Serial.println(this->imageWidth);
      Serial.print(F("imageHeight : "));
      Serial.println(this->imageHeight);
      Serial.print(F("colourPlanes : "));
      Serial.println(this->colourPlanes);
      Serial.print(F("bitsPerPixel : "));
      Serial.println(this->bitsPerPixel);
      Serial.print(F("compression : "));
      Serial.println(this->compression);
      Serial.print(F("imageSize : "));
      Serial.println(this->imageSize);
      Serial.print(F("xPixelsPerMeter : "));
      Serial.println(this->xPixelsPerMeter);
      Serial.print(F("yPixelsPerMeter : "));
      Serial.println(this->yPixelsPerMeter);
      Serial.print(F("totalColors : "));
      Serial.println(this->totalColors);
      Serial.print(F("importantColors : "));
      Serial.println(this->importantColors);
    }

    void renderImage(Adafruit_ILI9341 screen, int screenX, int screenY){
      
      // read from sd card in blocks
      uint8_t pixelBuffer[3 * 10];
      uint8_t pixelBufferCounter = sizeof(pixelBuffer);
      
      int bytesPerRow;
      int displayedWidth, displayedHeight;
      int pixelRow, pixelCol;
      uint32_t pixelRowFileOffset;
      uint8_t r,g,b;


      // make sure screenX, screenY is on screen
      if((screenX < 0) || (screenX >= screen.width()) 
        || (screenY < 0) || (screenY >= screen.height())){
      }

      // get dimensions of displayed image - crop if needed
      displayedWidth = this->imageWidth;
      displayedHeight = this->imageHeight;
      if (displayedWidth > (screen.width() - screenX)){
        displayedWidth = screen.width() - screenX;
      }
      if (displayedHeight > (screen.height() - screenY)){
        displayedHeight = screen.height() - screenY;
      }

      // bytes per row rounded up to next 4 byte boundary
      bytesPerRow = (3 * this->imageWidth) + ((4 - ((3 * this->imageWidth) % 4)) % 4);

      // open file
      this->bmpFile = SD.open(this->bmpFilename, FILE_READ);

      // set up tft byte write area
      screen.startWrite();
      screen.setAddrWindow(screenX, screenY, displayedWidth, displayedHeight);
      screen.endWrite();
      Serial.print(F("screenX : "));
      Serial.println(screenX);
      Serial.print(F("screenY : "));
      Serial.println(screenY);
      Serial.print(F("displayedWidth : "));
      Serial.println(displayedWidth);
      Serial.print(F("displayedHeight : "));
      Serial.println(displayedHeight);
      
      for (pixelRow = 0; pixelRow < displayedHeight; pixelRow ++) {
        // image stored bottom to top, screen top to bottom
        pixelRowFileOffset = this->imageOffset + ((this->imageHeight - pixelRow - 1) * bytesPerRow);

        // move file pointer to start of row pixel data
        screen.endWrite(); // turn off pixel stream while acxcessing sd card
        this->bmpFile.seek(pixelRowFileOffset);

        // reset buffer
        pixelBufferCounter = sizeof(pixelBuffer);

        // output pixels in row
        for (pixelCol = 0; pixelCol < displayedWidth; pixelCol ++) {
          if (pixelBufferCounter >= sizeof(pixelBuffer)){
            // need to read more from sd card
            screen.endWrite(); // turn of pixel stream while acxcessing sd card
            this->bmpFile.read(pixelBuffer, sizeof(pixelBuffer));
            pixelBufferCounter = 0;
            screen.startWrite(); // turn on pixel stream
          }

          // get next pixel colours
          b = pixelBuffer[pixelBufferCounter++];
          g = pixelBuffer[pixelBufferCounter++];
          r = pixelBuffer[pixelBufferCounter++];

          // send pixel to tft
          screen.writeColor(screen.color565(r,g,b),1);
          
        } // pixelCol
        
      } // pixelRow
      screen.endWrite(); // turn off pixel stream
      this->bmpFile.close();
    }
};

void loadImage(String fileName){

  if (isSdInserted){
   // get list of bmp files in root directory
    File rootFolder = SD.open("/");
    rootFolder.close();
    if (fileName != "") {
      BitmapHandler bmh = BitmapHandler(fileName);
      bmh.serialPrintHeaders();
      bmh.renderImage(tft,0,0);
      delay(3000);
    } 
  }
}


void setup(){
  Wire.begin();

  // Initialise Featherwing TFT Screen
  tft.begin();
  ts.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextSize(2);
  // Initialise Featherwing NEO Pixels
  pixels.begin();
  pixels.setBrightness(30);
  // Initialise Featherwing Keypad
  keyboard.begin();
  keyboard.setBacklight(0.5f);

  hardwareTests();
  //listSDCard();
  loadImage("logo.bmp");
  loadMenu();
}

void loop()
{
  
}
