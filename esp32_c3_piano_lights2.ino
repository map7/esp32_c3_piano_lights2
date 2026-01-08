// Test playing a files from the SD card triggering I/O for each note played.
// Example program to demonstrate the the MIDFile library playing through a 
// user instrument
//
// Hardware required:
//  SD card interface - change SD_SELECT for SPI comms
//  I/O defined for custom notes output - Change pin definitions for specific 
//      hardware or other hardware actuation - defined below.

// -----------------------------------------------------------------------
// OLED
// -----------------------------------------------------------------------
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// -----------------------------------------------------------------------
// Neopixel
// -----------------------------------------------------------------------
#include <Adafruit_NeoPixel.h>
#define PIN_NEO_PIXEL 2  // The ESP32 pin GPIO16 connected to NeoPixel
#define NUM_PIXELS 300     // The number of LEDs (pixels) on NeoPixel LED strip

Adafruit_NeoPixel NeoPixel(NUM_PIXELS, PIN_NEO_PIXEL, NEO_GRB + NEO_KHZ800);

// -----------------------------------------------------------------------
// SD Card
// -----------------------------------------------------------------------
#include <SdFat.h>
#include <MD_MIDIFile.h>

// Defining the printout
#define USE_DEBUG  1   // set to 1 to enable MIDI output, otherwise debug output

#if USE_DEBUG // set up for direct DEBUG output

#define DEBUGS(s)    do { Serial.print(F(s)); } while (false)
#define DEBUG(s, x)  do { Serial.print(F(s)); Serial.print(x); } while(false)
#define DEBUGX(s, x) do { Serial.print(F(s)); Serial.print(F("0x")); Serial.print(x, HEX); } while(false)

#else

#define DEBUGS(s)
#define DEBUG(s, x)
#define DEBUGX(s, x)

#endif // USE_DEBUG

#if SD_FAT_TYPE == 0
  SdFat sd;
  File file;
  File root;
#elif SD_FAT_TYPE == 1
  SdFat32 sd;
  File32 file;
  File32 root;
#elif SD_FAT_TYPE == 2
  SdExFat sd;
  ExFile file;
  ExFile root;
#elif SD_FAT_TYPE == 3
  SdFs sd;
  FsFile file;
  FsFile root;
#endif  // SD_FAT_TYPE

// -----------------------------------------------------------------------
// MIDI
// -----------------------------------------------------------------------

// SD chip select pin for SPI comms.
// Default SD chip select is the SPI SS pin (10 on Uno, 53 on Mega).
const uint8_t SD_SELECT = SS;

const uint16_t WAIT_DELAY = 2000; // ms

// Define constants for MIDI channel voice message IDs
const uint8_t NOTE_OFF = 0x80;    // note on
const uint8_t NOTE_ON = 0x90;     // note off. NOTE_ON with velocity 0 is same as NOTE_OFF

// The files in should be located on the SD card
//const char fileName[] = "pianosolo.mid";
//const char fileName[] = "furelise.mid";
const char fileName[] = "Let_It_Be.mid";
//const char fileName[] = "test.mid";
//const char fileName[] = "the_final_countdown.mid";
//const char fileName[] = "LOOPDEMO.MID";
//const char fileName[] = "TWINKLE.MID";
//const char fileName[] = "POPCORN.MID";

const uint8_t ACTIVE = HIGH;
const uint8_t SILENT = LOW;

SDFAT	SD;
MD_MIDIFile SMF;

void playNote(uint8_t note, bool state)
{
  if (note > 128) return;

  DEBUG("\nNOTE: ", note);
  DEBUG("\nSTATE: ", state);

  //NeoPixel.clear();  // set all pixel colors to 'off'. It only takes effect if pixels.show() is called

  if (state) {
    NeoPixel.setPixelColor(note - 21, NeoPixel.Color(0, 255, 0));
  }else{
    NeoPixel.setPixelColor(note - 21, NeoPixel.Color(0, 0, 0));
  }
  
  NeoPixel.show();
}

void midiCallback(midi_event *pev)
// Called by the MIDIFile library when a MIDI event needs to be processed.
// This callback is set up in the setup() function.
// Note: MIDI Channel 10 (pev->channel == 9) is for percussion instruments
{
  // if(pev->channel+1 == 1){
    DEBUG("\n", millis());
    DEBUG("\tM T", pev->track);
    DEBUG(":  Ch ", pev->channel+1);
    DEBUGS(" Data");

    for (uint8_t i=0; i<pev->size; i++)
      DEBUGX(" ", pev->data[i]);

    // Handle the event through our I/O interface
    switch (pev->data[0])
    {
    case NOTE_OFF:    // [1]=note no, [2]=velocity
      DEBUGS(" NOTE_OFF");
      playNote(pev->data[1], SILENT);
      break;

    case NOTE_ON:     // [1]=note_no, [2]=velocity
      DEBUGS(" NOTE_ON");
      // Note ON with velocity 0 is the same as off
      playNote(pev->data[1], (pev->data[2] == 0) ? SILENT : ACTIVE);
      break;
    }

  // }
}

void setup(void)
{
#if USE_DEBUG
  Serial.begin(57600);
#endif
  DEBUGS("\n[MidiFile Play I/O]");

  // OLED
  Wire.begin(8,9);
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Clear the buffer
  display.clearDisplay();

  // Draw a single pixel in white
  display.drawPixel(10, 10, SSD1306_WHITE);

  display.setTextWrap(false);

  // Show the display buffer on the screen. You MUST call display() after
  // drawing commands to make them visible on screen!
  display.display();

  delay(2000);

  NeoPixel.begin();  // initialize NeoPixel strip object (REQUIRED)

  // Initialize SD
  if (!SD.begin(SD_SELECT, SPI_FULL_SPEED))
  {
    DEBUGS("\nSD init fail!");
    while (true) ;
  }

  // Initialize MIDIFile
  SMF.begin(&SD);
  SMF.setMidiHandler(midiCallback);
}

void loop(void)
{
  static enum { S_IDLE, S_PLAYING, S_END, S_PAUSE } state = S_IDLE;
  static uint32_t timeStart;

  Serial.println(F("Listing files on SD card:"));
  //SD.ls(&Serial);

  if (!root.open("/")) {
    Serial.println("ERROR Opening Root");
  }

  // Step through each file
  int index = 0;
  int selected = 1;
  display.clearDisplay();

  while (file.openNext(&root, O_RDONLY)) {
    char buf[255];
    
    file.getName(buf, 255);

    if (strstr(buf, ".mid")){
      String sbuf(buf);
      Serial.println(sbuf);

      display.setTextSize(1);                 // Normal 1:1 pixel scale
      display.setTextColor(SSD1306_WHITE);    // Draw white text

      if (index == selected){
        sbuf = "* " + sbuf;
      }
      
      display.setCursor(0,10 * index);                // Start at top-left corner
      display.println(sbuf);
      display.display();   

      index++;

      delay(1000);
    }
  }

  switch (state)
  {
  case S_IDLE:    // now idle, set up the next tune
    {
      int err;

      DEBUGS("\nS_IDLE");

      // play the file name



      display.clearDisplay();

      display.setTextSize(1);                 // Normal 1:1 pixel scale
      display.setTextColor(SSD1306_WHITE);    // Draw white text
      display.setCursor(0,0);                 // Start at top-left corner
      display.println(F("Filename"));
      display.setCursor(0,10);                // Start at top-left corner
      display.println(F(fileName));
      display.display();      

      DEBUG("\nFile: ", fileName);
      err = SMF.load(fileName);
      if (err != MD_MIDIFile::E_OK)
      {
        DEBUG(" - SMF load Error ", err);
        timeStart = millis();
        state = S_PAUSE;
        DEBUGS("\nWAIT_BETWEEN");
      }
      else
      {
        //DEBUGS("\nS_PLAYING");
        state = S_PLAYING;
      }
    }
    break;

  case S_PLAYING: // play the file
    //DEBUGS("\nS_PLAYING");
    SMF.getNextEvent();
    if (SMF.isEOF())
      state = S_END;
    break;

  case S_END:   // done with this one
    DEBUGS("\nS_END");
    SMF.close();
    NeoPixel.clear();
    timeStart = millis();
    state = S_PAUSE;
    DEBUGS("\nPAUSE");
    break;

  case S_PAUSE:    // signal finished with a dignified pause
    if (millis() - timeStart >= WAIT_DELAY)
      state = S_IDLE;
    break;

  default:
    state = S_IDLE;
    break;
  }
}