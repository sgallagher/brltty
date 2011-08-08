/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2011 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

/* Voyager/braille.c - Braille display driver for Tieman Voyager displays.
 *
 * Written by St�phane Doyon  <s.doyon@videotron.ca>
 *
 * It is being tested on Voyager 44, should also support Voyager 70.
 * It is designed to be compiled in BRLTTY version 4.1.
 *
 * History:
 * 0.21, January 2005:
 *       Remove gcc4 signedness/unsignedness incompatibilities.
 * 0.20, June 2004:
 *       Add statuscells parameter.
 *       Rename brlinput parameter to inputmode.
 *       Change default inputmode to no.
 *       Chorded functions work without chording when inputmode is no.
 *       Move complex routing key combinations to front/dot keys.
 *       Duplicate status key bindings on front/dot keys.
 *       Execute on first release rather than on all released.
 *       Add support for the part232 serial adapter.
 * 0.10, March 2004: Use BRLTTY core repeat functions. Add brlinput parameter
 *   and toggle to disallow braille typing.
 * 0.01, January 2004: fork from the original driver which relied on an
 *   in-kernel USB driver.
 */

#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "log.h"
#include "timing.h"
#include "ascii.h"

#include "brl_driver.h"
#include "brldefs-vo.h"


BEGIN_KEY_NAME_TABLE(all)
  KEY_SET_ENTRY(VO_SET_RoutingKeys, "RoutingKey"),

  KEY_NAME_ENTRY(VO_KEY_Dot1, "Dot1"),
  KEY_NAME_ENTRY(VO_KEY_Dot2, "Dot2"),
  KEY_NAME_ENTRY(VO_KEY_Dot3, "Dot3"),
  KEY_NAME_ENTRY(VO_KEY_Dot4, "Dot4"),
  KEY_NAME_ENTRY(VO_KEY_Dot5, "Dot5"),
  KEY_NAME_ENTRY(VO_KEY_Dot6, "Dot6"),
  KEY_NAME_ENTRY(VO_KEY_Dot7, "Dot7"),
  KEY_NAME_ENTRY(VO_KEY_Dot8, "Dot8"),

  KEY_NAME_ENTRY(VO_KEY_Thumb1, "Thumb1"),
  KEY_NAME_ENTRY(VO_KEY_Thumb2, "Thumb2"),
  KEY_NAME_ENTRY(VO_KEY_Left, "Left"),
  KEY_NAME_ENTRY(VO_KEY_Up, "Up"),
  KEY_NAME_ENTRY(VO_KEY_Down, "Down"),
  KEY_NAME_ENTRY(VO_KEY_Right, "Right"),
  KEY_NAME_ENTRY(VO_KEY_Thumb3, "Thumb3"),
  KEY_NAME_ENTRY(VO_KEY_Thumb4, "Thumb4"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(bp)
  KEY_NAME_ENTRY(BP_KEY_Dot1, "Dot1"),
  KEY_NAME_ENTRY(BP_KEY_Dot2, "Dot2"),
  KEY_NAME_ENTRY(BP_KEY_Dot3, "Dot3"),
  KEY_NAME_ENTRY(BP_KEY_Dot4, "Dot4"),
  KEY_NAME_ENTRY(BP_KEY_Dot5, "Dot5"),
  KEY_NAME_ENTRY(BP_KEY_Dot6, "Dot6"),

  KEY_NAME_ENTRY(BP_KEY_Shift, "Shift"),
  KEY_NAME_ENTRY(BP_KEY_Space, "Space"),
  KEY_NAME_ENTRY(BP_KEY_Control, "Control"),

  KEY_NAME_ENTRY(BP_KEY_JoystickEnter, "JoystickEnter"),
  KEY_NAME_ENTRY(BP_KEY_JoystickLeft, "JoystickLeft"),
  KEY_NAME_ENTRY(BP_KEY_JoystickRight, "JoystickRight"),
  KEY_NAME_ENTRY(BP_KEY_JoystickUp, "JoystickUp"),
  KEY_NAME_ENTRY(BP_KEY_JoystickDown, "JoystickDown"),

  KEY_NAME_ENTRY(BP_KEY_ScrollLeft, "ScrollLeft"),
  KEY_NAME_ENTRY(BP_KEY_ScrollRight, "ScrollRight"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLES(all)
  KEY_NAME_TABLE(all),
END_KEY_NAME_TABLES

BEGIN_KEY_NAME_TABLES(bp)
  KEY_NAME_TABLE(bp),
END_KEY_NAME_TABLES

DEFINE_KEY_TABLE(all)
DEFINE_KEY_TABLE(bp)

BEGIN_KEY_TABLE_LIST
  &KEY_TABLE_DEFINITION(all),
  &KEY_TABLE_DEFINITION(bp),
END_KEY_TABLE_LIST


#define READY_BEEP_DURATION 200
#define MAXIMUM_CELL_COUNT 70 /* arbitrary max for allocations */

static unsigned char cellsInitialized;
static unsigned char cellCount;
#define IS_TEXT_RANGE(key1,key2) (((key1) <= (key2)) && ((key2) < cellCount))
#define IS_TEXT_KEY(key) IS_TEXT_RANGE((key), (key))

/* Structure to remember which keys are pressed */
typedef struct {
  uint16_t navigation;
  unsigned char routing[MAXIMUM_CELL_COUNT];
} Keys;

static Keys pressedKeys;
static char keysInitialized;

static void
initializeKeys (void) {
  if (!keysInitialized) {
    memset(&pressedKeys, 0, sizeof(pressedKeys));
    keysInitialized = 1;
  }
}

static void
updateKeys (const unsigned char *packet) {
  Keys currentKeys;

  unsigned char navigationPresses[0X10];
  int navigationPressCount = 0;

  unsigned char routingPresses[6];
  int routingPressCount = 0;

  initializeKeys();
  memset(&currentKeys, 0, sizeof(currentKeys));
  currentKeys.navigation = (packet[1] << 8) | packet[0];

  {
    unsigned char key = 0;
    uint16_t bit = 0X1;

    while (key < 0X10) {
      if ((pressedKeys.navigation & bit) && !(currentKeys.navigation & bit)) {
        enqueueKeyEvent(VO_SET_NavigationKeys, key, 0);
      } else if (!(pressedKeys.navigation & bit) && (currentKeys.navigation & bit)) {
        navigationPresses[navigationPressCount++] = key;
      }

      bit <<= 1;
      key += 1;
    }
  }
  
  {
    int i;

    for (i=2; i<8; i+=1) {
      unsigned char key = packet[i];
      if (!key) break;

      if ((key < 1) || (key > cellCount)) {
        logMessage(LOG_NOTICE, "invalid routing key number: %u", key);
        continue;
      }
      key -= 1;

      currentKeys.routing[key] = 1;
      if (!pressedKeys.routing[key]) routingPresses[routingPressCount++] = key;
    }
  }

  {
    unsigned char key;

    for (key=0; key<cellCount; key+=1)
      if (pressedKeys.routing[key] && !currentKeys.routing[key])
        enqueueKeyEvent(VO_SET_RoutingKeys, key, 0);
  }

  while (navigationPressCount)
    enqueueKeyEvent(VO_SET_NavigationKeys, navigationPresses[--navigationPressCount], 1);

  while (routingPressCount)
    enqueueKeyEvent(VO_SET_RoutingKeys, routingPresses[--routingPressCount], 1);

  pressedKeys = currentKeys;
}


typedef struct {
  int (*getCellCount) (BrailleDisplay *brl, unsigned char *length);
  int (*logSerialNumber) (BrailleDisplay *brl);
  int (*logHardwareVersion) (BrailleDisplay *brl);
  int (*logFirmwareVersion) (BrailleDisplay *brl);
  int (*setDisplayVoltage) (BrailleDisplay *brl, unsigned char voltage);
  int (*getDisplayVoltage) (BrailleDisplay *brl, unsigned char *voltage);
  int (*getDisplayCurrent) (BrailleDisplay *brl, unsigned char *current);
  int (*setDisplayState) (BrailleDisplay *brl, unsigned char state);
  int (*writeBraille) (BrailleDisplay *brl, const unsigned char *cells, unsigned char count, unsigned char start);
  int (*updateKeys) (BrailleDisplay *brl);
  int (*soundBeep) (BrailleDisplay *brl, unsigned char duration);
} ProtocolOperations;

typedef struct {
  const ProtocolOperations *protocol;
  int (*openPort) (char **parameters, const char *device);
  void (*closePort) (void);
  int (*awaitInput) (int timeout);
  ssize_t (*readData) (void *buffer, size_t size, int wait);
  ssize_t (*writeData) (const void *data, size_t size);
} InputOutputOperations;

static const InputOutputOperations *io;


#define SERIAL_BAUD 38400
#define SERIAL_INITIAL_TIMEOUT 200
#define SERIAL_SUBSEQUENT_TIMEOUT 100

static const char *serialDeviceNames[] = {"Serial Adapter", "Base Unit"};
static unsigned int serialCharactersPerSecond;

static inline void
setSerialCharactersPerSecond (int bits) {
  serialCharactersPerSecond = SERIAL_BAUD / bits;
}

static int
writeSerialPacket (BrailleDisplay *brl, unsigned char code, unsigned char *data, unsigned char count) {
  unsigned char buffer[2 + (count * 2)];
  unsigned char size = 0;
  unsigned char index;

  buffer[size++] = ESC;
  buffer[size++] = code;

  for (index=0; index<count; ++index)
    if ((buffer[size++] = data[index]) == buffer[0])
      buffer[size++] = buffer[0];

  logOutputPacket(buffer, size);
  brl->writeDelay += (size * 1000 / serialCharactersPerSecond) + 1;
  return io->writeData(buffer, size) != -1;
}

static int
readSerialPacket (unsigned char *packet, int size) {
  int started = 0;
  int escape = 0;
  int offset = 0;
  int length = 0;

  while (1) {
    unsigned char byte;

    {
      ssize_t result = io->readData(&byte, 1, (started || escape));

      if (result == 0) {
        errno = EAGAIN;
        result = -1;
      }

      if (result == -1) {
        if (started) logPartialPacket(packet, offset);
        return 0;
      }
    }

    if (byte == ESC) {
      if ((escape = !escape)) continue;
    } else if (escape) {
      escape = 0;

      if (offset > 0) {
        logShortPacket(packet, offset);
        offset = 0;
        length = 0;
      } else {
        started = 1;
      }
    }

    if (!started) {
      logIgnoredByte(byte);
      continue;
    }

    if (offset < size) {
      if (offset == 0) {
        switch (byte) {
          case 0X43:
          case 0X47:
            length = 2;
            break;

          case 0X4C:
            length = 3;
            break;

          case 0X46:
          case 0X48:
            length = 5;
            break;

          case 0X4B:
            length = 9;
            break;

          case 0X53:
            length = 10;
            break;

          default:
            logUnknownPacket(byte);
            started = 0;
            continue;
        }
      }

      packet[offset] = byte;
    } else {
      if (offset == size) logTruncatedPacket(packet, offset);
      logDiscardedByte(byte);
    }

    if (++offset == length) {
      if (offset > size) {
        offset = 0;
        length = 0;
        started = 0;
        continue;
      }

      logInputPacket(packet, offset);
      return length;
    }
  }
}

static int
nextSerialPacket (unsigned char code, unsigned char *buffer, int size, int wait) {
  int length;

  if (wait)
    if (!io->awaitInput(SERIAL_INITIAL_TIMEOUT))
      return 0;

  while ((length = readSerialPacket(buffer, size))) {
    if (buffer[0] == code) return length;
    logUnexpectedPacket(buffer, length);
  }

  return 0;
}

static int
getSerialCellCount (BrailleDisplay *brl, unsigned char *count) {
  const unsigned int code = 0X4C;
  if (writeSerialPacket(brl, code, NULL, 0)) {
    unsigned char buffer[3];
    if (nextSerialPacket(code, buffer, sizeof(buffer), 1)) {
      *count = buffer[2];
      return 1;
    }
  }
  return 0;
}

static int
logSerialSerialNumber (BrailleDisplay *brl) {
  unsigned char device;

  for (device=0; device<ARRAY_COUNT(serialDeviceNames); ++device) {
    const unsigned char code = 0X53;
    unsigned char buffer[10];

    if (!writeSerialPacket(brl, code, &device, 1)) return 0;
    if (!nextSerialPacket(code, buffer, sizeof(buffer), 1)) return 0;
    logMessage(LOG_INFO, "%s Serial Number: %02X%02X%02X%02X%02X%02X%02X%02X",
               serialDeviceNames[buffer[1]],
               buffer[2], buffer[3], buffer[4], buffer[5],
               buffer[6], buffer[7], buffer[8], buffer[9]);
  }

  return 1;
}

static int
logSerialHardwareVersion (BrailleDisplay *brl) {
  unsigned char device;

  for (device=0; device<ARRAY_COUNT(serialDeviceNames); ++device) {
    const unsigned char code = 0X48;
    unsigned char buffer[5];

    if (!writeSerialPacket(brl, code, &device, 1)) return 0;
    if (!nextSerialPacket(code, buffer, sizeof(buffer), 1)) return 0;
    logMessage(LOG_INFO, "%s Hardware Version: %c.%c.%c", 
               serialDeviceNames[buffer[1]],
               buffer[2], buffer[3], buffer[4]);
  }

  return 1;
}

static int
logSerialFirmwareVersion (BrailleDisplay *brl) {
  unsigned char device;

  for (device=0; device<ARRAY_COUNT(serialDeviceNames); ++device) {
    const unsigned char code = 0X46;
    unsigned char buffer[5];

    if (!writeSerialPacket(brl, code, &device, 1)) return 0;
    if (!nextSerialPacket(code, buffer, sizeof(buffer), 1)) return 0;
    logMessage(LOG_INFO, "%s Firmware Version: %c.%c.%c", 
               serialDeviceNames[buffer[1]],
               buffer[2], buffer[3], buffer[4]);
  }

  return 1;
}

static int
setSerialDisplayVoltage (BrailleDisplay *brl, unsigned char voltage) {
  return writeSerialPacket(brl, 0X56, &voltage, 1);
}

static int
getSerialDisplayVoltage (BrailleDisplay *brl, unsigned char *voltage) {
  const unsigned char code = 0X47;
  if (writeSerialPacket(brl, code, NULL, 0)) {
    unsigned char buffer[2];
    if (nextSerialPacket(code, buffer, sizeof(buffer), 1)) {
      *voltage = buffer[1];
      return 1;
    }
  }
  return 0;
}

static int
getSerialDisplayCurrent (BrailleDisplay *brl, unsigned char *current) {
  const unsigned int code = 0X43;
  if (writeSerialPacket(brl, code, NULL, 0)) {
    unsigned char buffer[2];
    if (nextSerialPacket(code, buffer, sizeof(buffer), 1)) {
      *current = buffer[1];
      return 1;
    }
  }
  return 0;
}

static int
setSerialDisplayState (BrailleDisplay *brl, unsigned char state) {
  return writeSerialPacket(brl, 0X44, &state, 1);
}

static int
writeSerialBraille (BrailleDisplay *brl, const unsigned char *cells, unsigned char count, unsigned char start) {
  unsigned char buffer[2 + count];
  unsigned char size = 0;
  buffer[size++] = start;
  buffer[size++] = count;
  memcpy(&buffer[size], cells, count);
  size += count;
  return writeSerialPacket(brl, 0X42, buffer, size);
}

static int
updateSerialKeys (BrailleDisplay *brl) {
  const unsigned char code = 0X4B;
  unsigned char packet[9];

  while (nextSerialPacket(code, packet, sizeof(packet), 0)) {
    updateKeys(&packet[1]);
  }

  return errno == EAGAIN;
}

static int
soundSerialBeep (BrailleDisplay *brl, unsigned char duration) {
  return writeSerialPacket(brl, 0X41, &duration, 1);
}

static const ProtocolOperations serialProtocolOperations = {
  .getCellCount = getSerialCellCount,
  .logSerialNumber = logSerialSerialNumber,
  .logHardwareVersion = logSerialHardwareVersion,
  .logFirmwareVersion = logSerialFirmwareVersion,
  .setDisplayVoltage = setSerialDisplayVoltage,
  .getDisplayVoltage = getSerialDisplayVoltage,
  .getDisplayCurrent = getSerialDisplayCurrent,
  .setDisplayState = setSerialDisplayState,
  .writeBraille = writeSerialBraille,
  .updateKeys = updateSerialKeys,
  .soundBeep = soundSerialBeep
};


#include "io_serial.h"

#define SERIAL_OPEN_DELAY 4000

static SerialDevice *serialDevice = NULL;

static int
openSerialPort (char **parameters, const char *device) {
  if ((serialDevice = serialOpenDevice(device))) {
    if (serialRestartDevice(serialDevice, SERIAL_BAUD)) {
      if (serialSetFlowControl(serialDevice, SERIAL_FLOW_HARDWARE)) {
        setSerialCharactersPerSecond(serialGetCharacterBits(serialDevice));
        approximateDelay(SERIAL_OPEN_DELAY);
        return 1;
      }
    }

    serialCloseDevice(serialDevice);
    serialDevice = NULL;
  }

  return 0;
}

static void
closeSerialPort (void) {
  if (serialDevice) {
    serialCloseDevice(serialDevice);
    serialDevice = NULL;
  }
}

static int
awaitSerialInput (int timeout) {
  return serialAwaitInput(serialDevice, timeout);
}

static ssize_t
readSerialData (void *buffer, size_t size, int wait) {
  const int timeout = SERIAL_SUBSEQUENT_TIMEOUT;
  return serialReadData(serialDevice, buffer, size,
                        (wait? timeout: 0), timeout);
}

static ssize_t
writeSerialData (const void *data, size_t size) {
  return serialWriteData(serialDevice, data, size);
}

static const InputOutputOperations serialInputOutputOperations = {
  .protocol = &serialProtocolOperations,
  .openPort = openSerialPort,
  .closePort = closeSerialPort,
  .awaitInput = awaitSerialInput,
  .readData = readSerialData,
  .writeData = writeSerialData
};


#include "io_bluetooth.h"

#define BLUETOOTH_CHANNEL_NUMBER 1
#define BLUETOOTH_CHARACTER_BITS 8
#define BLUETOOTH_OPEN_DELAY 800

static BluetoothConnection *bluetoothConnection = NULL;

static int
openBluetoothPort (char **parameters, const char *device) {
  if (!(bluetoothConnection = bthOpenConnection(device, BLUETOOTH_CHANNEL_NUMBER, 0))) return 0;
  setSerialCharactersPerSecond(BLUETOOTH_CHARACTER_BITS);
  approximateDelay(BLUETOOTH_OPEN_DELAY);
  return 1;
}

static void
closeBluetoothPort (void) {
  if (bluetoothConnection) {
    bthCloseConnection(bluetoothConnection);
    bluetoothConnection = NULL;
  }
}

static int
awaitBluetoothInput (int timeout) {
  return bthAwaitInput(bluetoothConnection, timeout);
}

static ssize_t
readBluetoothData (void *buffer, size_t size, int wait) {
  const int timeout = SERIAL_SUBSEQUENT_TIMEOUT;
  return bthReadData(bluetoothConnection, buffer, size,
                     (wait? timeout: 0), timeout);
}

static ssize_t
writeBluetoothData (const void *data, size_t size) {
  return bthWriteData(bluetoothConnection, data, size);
}

static const InputOutputOperations bluetoothInputOutputOperations = {
  .protocol = &serialProtocolOperations,
  .openPort = openBluetoothPort,
  .closePort = closeBluetoothPort,
  .awaitInput = awaitBluetoothInput,
  .readData = readBluetoothData,
  .writeData = writeBluetoothData
};


#include "io_usb.h"

/* Workarounds for control transfer flakiness (at least in this demo model) */
#define USB_RETRIES 6

static UsbChannel *usbChannel = NULL;

static int
putUsbData (
  uint8_t request, uint16_t value, uint16_t index,
  const unsigned char *buffer, uint16_t size
) {
  int retry = 0;
  while (1) {
    int ret = usbControlWrite(usbChannel->device, UsbControlRecipient_Endpoint, UsbControlType_Vendor,
                              request, value, index, buffer, size, 100);
    if (ret != -1) return 1;
    if ((errno != EPIPE) || (retry == USB_RETRIES)) return 0;
    logMessage(LOG_WARNING, "USB request 0X%X retry #%d", request, ++retry);
  }
}

static int
getUsbData (
  uint8_t request, uint16_t value, uint16_t index,
  unsigned char *buffer, uint16_t size
) {
  int retry = 0;

  while (1) {
    int ret = usbControlRead(usbChannel->device, UsbControlRecipient_Endpoint, UsbControlType_Vendor,
                             request, value, index, buffer, size, 100);
    if ((ret != -1) || (errno != EPIPE) || (retry == USB_RETRIES)) return ret;
    logMessage(LOG_WARNING, "USB request 0X%X retry #%d", request, ++retry);
  }
}

static int
getUsbCellCount (BrailleDisplay *brl, unsigned char *count) {
  unsigned char buffer[2];
  int size = getUsbData(0X06, 0, 0, buffer, sizeof(buffer));
  if (size == -1) return 0;

  *count = buffer[1];
  return 1;
}

static int
logUsbString (uint8_t request, const char *description) {
  UsbDescriptor descriptor;
  if (getUsbData(request, 0, 0, descriptor.bytes, sizeof(descriptor.bytes)) != -1) {
    char *string = usbDecodeString(&descriptor.string);
    if (string) {
      logMessage(LOG_INFO, "%s: %s", description, string);
      free(string);
      return 1;
    }
  }
  return 0;
}

static int
logUsbSerialNumber (BrailleDisplay *brl) {
  return logUsbString(0X03, "Serial Number");
}

static int
logUsbHardwareVersion (BrailleDisplay *brl) {
  unsigned char buffer[2];
  int size = getUsbData(0X04, 0, 0, buffer, sizeof(buffer));
  if (size == -1) return 0;

  logMessage(LOG_INFO, "Hardware Version: %u.%u",
             buffer[0], buffer[1]);
  return 1;
}

static int
logUsbFirmwareVersion (BrailleDisplay *brl) {
  return logUsbString(0X05, "Firmware Version");
}

static int
setUsbDisplayVoltage (BrailleDisplay *brl, unsigned char voltage) {
  return putUsbData(0X01, voltage, 0, NULL, 0);
}

static int
getUsbDisplayVoltage (BrailleDisplay *brl, unsigned char *voltage) {
  unsigned char buffer[1];
  int size = getUsbData(0X02, 0, 0, buffer, sizeof(buffer));
  if (size == -1) return 0;

  *voltage = buffer[0];
  return 1;
}

static int
getUsbDisplayCurrent (BrailleDisplay *brl, unsigned char *current) {
  unsigned char buffer[1];
  int size = getUsbData(0X08, 0, 0, buffer, sizeof(buffer));
  if (size == -1) return 0;

  *current = buffer[0];
  return 1;
}

static int
setUsbDisplayState (BrailleDisplay *brl, unsigned char state) {
  return putUsbData(0X00, state, 0, NULL, 0);
}

static int
writeUsbBraille (BrailleDisplay *brl, const unsigned char *cells, unsigned char count, unsigned char start) {
  return putUsbData(0X07, 0, start, cells, count);
}

static int
updateUsbKeys (BrailleDisplay *brl) {
  while (1) {
    unsigned char packet[8];

    {
      ssize_t result = io->readData(packet, sizeof(packet), 0);

      if (result < 0) {
        if (errno == EAGAIN) {
          /* no input */
          return 1;
        }

        if (errno == ENODEV) {
          /* Display was disconnected */
          return 0;
        }

        logMessage(LOG_ERR, "USB read error: %s", strerror(errno));
        keysInitialized = 0;
        return 1;
      }

      if ((result > 0) && (result < sizeof(packet))) {
        /* The display handles read requests of only and exactly 8 bytes */
        logMessage(LOG_WARNING, "short USB read: %ld", (long int)result);
        keysInitialized = 0;
        return 1;
      }

      if (result == 0) {
        /* no new key */
        return 1;
      }
    }

    updateKeys(packet);
  }
}

static int
soundUsbBeep (BrailleDisplay *brl, unsigned char duration) {
  return putUsbData(0X09, duration, 0, NULL, 0);
}

static const ProtocolOperations usbProtocolOperations = {
  .getCellCount = getUsbCellCount,
  .logSerialNumber = logUsbSerialNumber,
  .logHardwareVersion = logUsbHardwareVersion,
  .logFirmwareVersion = logUsbFirmwareVersion,
  .setDisplayVoltage = setUsbDisplayVoltage,
  .getDisplayVoltage = getUsbDisplayVoltage,
  .getDisplayCurrent = getUsbDisplayCurrent,
  .setDisplayState = setUsbDisplayState,
  .writeBraille = writeUsbBraille,
  .updateKeys = updateUsbKeys,
  .soundBeep = soundUsbBeep
};


static int
openUsbPort (char **parameters, const char *device) {
  static const UsbChannelDefinition definitions[] = {
    { .vendor=0X0798, .product=0X0001, 
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1
    }
    ,
    { .vendor=0 }
  };

  if ((usbChannel = usbFindChannel(definitions, (void *)device))) {
    return 1;
  }

  return 0;
}

static void
closeUsbPort (void) {
  if (usbChannel) {
    usbCloseChannel(usbChannel);
    usbChannel = NULL;
  }
}

static int
awaitUsbInput (int timeout) {
  return usbAwaitInput(usbChannel->device,
                       usbChannel->definition.inputEndpoint,
                       timeout);
}

static ssize_t
readUsbData (void *buffer, size_t size, int wait) {
  const int timeout = 100;
  ssize_t result = usbReapInput(usbChannel->device,
                                usbChannel->definition.inputEndpoint,
                                buffer, size,
                                (wait? timeout: 0), timeout);

  if (result != -1) return result;
  if (errno == EAGAIN) return 0;
  return -1;
}

static const InputOutputOperations usbInputOutputOperations = {
  .protocol = &usbProtocolOperations,
  .openPort = openUsbPort,
  .closePort = closeUsbPort,
  .awaitInput = awaitUsbInput,
  .readData = readUsbData
};


typedef struct {
  const char *productName;
  const KeyTableDefinition *keyTableDefinition;
} DeviceType;

static const DeviceType deviceType_Voyager = {
  .productName = "Voyager",
  .keyTableDefinition = &KEY_TABLE_DEFINITION(all)
};

static const DeviceType deviceType_BraillePen = {
  .productName = "Braille Pen",
  .keyTableDefinition = &KEY_TABLE_DEFINITION(bp)
};


typedef struct {
  const DeviceType *deviceType;
  int (*writeBraille) (BrailleDisplay *brl, const unsigned char *cells, unsigned char count, unsigned char start);
  unsigned char reportedCellCount;
  unsigned char actualCellCount;
  unsigned partialUpdates:1;
} DeviceModel;

static const DeviceModel *deviceModel;

typedef struct {
  struct {
    const unsigned char *cells;
    unsigned char offset;
    unsigned char count;
  } from;

  struct {
    unsigned char *cells;
    unsigned char offset;
  } to;
} WriteBrailleData;

static void
addHiddenCells (WriteBrailleData *wbd, unsigned char size) {
  while (size) {
    wbd->to.cells[wbd->to.offset++] = 0;
    size -= 1;
  }
}

static void
addActualCells (WriteBrailleData *wbd, unsigned char size) {
  unsigned char count;

  if (!size) size = wbd->from.count;
  if ((count = size) > wbd->from.count) count = wbd->from.count;

  if (count) {
    memcpy(&wbd->to.cells[wbd->to.offset], &wbd->from.cells[wbd->from.offset], count);
    wbd->from.count -= count;
    wbd->from.offset += count;
    wbd->to.offset += count;
  }

  addHiddenCells(wbd, size-count);
}

static int
writeBraille0 (BrailleDisplay *brl, const unsigned char *cells, unsigned char count, unsigned char start) {
  return io->protocol->writeBraille(brl, cells, count, start);
}

static int
writeBraille2 (BrailleDisplay *brl, const unsigned char *cells, unsigned char count, unsigned char start) {
  if (!deviceModel->partialUpdates) {
    unsigned char buffer[count + 2];
    WriteBrailleData wbd = {
      .from = {
        .cells = cells,
        .offset = 0,
        .count = cellCount
      },

      .to = {
        .cells = buffer,
        .offset = 0
      }
    };

    addHiddenCells(&wbd, 2);
    addActualCells(&wbd, 0);
    return io->protocol->writeBraille(brl, buffer, sizeof(buffer), 0);
  }

  return io->protocol->writeBraille(brl, cells, count, start+2);
}

static int
writeBraille4 (BrailleDisplay *brl, const unsigned char *cells, unsigned char count, unsigned char start) {
  if (!deviceModel->partialUpdates) {
    unsigned char buffer[count + 4];
    WriteBrailleData wbd = {
      .from = {
        .cells = cells,
        .offset = 0,
        .count = cellCount
      },

      .to = {
        .cells = buffer,
        .offset = 0
      }
    };

    addHiddenCells(&wbd, 2);
    addActualCells(&wbd, 6);
    addHiddenCells(&wbd, 2);
    addActualCells(&wbd, 0);
    return io->protocol->writeBraille(brl, buffer, sizeof(buffer), 0);
  }

  if (start >= 6) {
    return io->protocol->writeBraille(brl, &cells[start], count, start+4);
  }

  if ((start + count) <= 6) {
    return io->protocol->writeBraille(brl, &cells[start], count, start+2);
  }

  {
    unsigned char buffer[count + 2];
    WriteBrailleData wbd = {
      .from = {
        .cells = cells,
        .offset = start,
        .count = count
      },

      .to = {
        .cells = buffer,
        .offset = 0
      }
    };

    addActualCells(&wbd, 6-start);
    addHiddenCells(&wbd, 2);
    addActualCells(&wbd, 0);
    return io->protocol->writeBraille(brl, buffer, sizeof(buffer), start+2);
  }
}

static const DeviceModel deviceModels[] = {
  { .reportedCellCount = 48,
    .actualCellCount = 44,
    .writeBraille = writeBraille4,
    .partialUpdates = 1,
    .deviceType = &deviceType_Voyager
  }
  ,
  { .reportedCellCount = 72,
    .actualCellCount = 70,
    .writeBraille = writeBraille2,
    .partialUpdates = 1,
    .deviceType = &deviceType_Voyager
  }
  ,
  { .reportedCellCount = 12,
    .actualCellCount = 12,
    .writeBraille = writeBraille0,
    .deviceType = &deviceType_BraillePen
  }
  ,
  { .deviceType = NULL }
};


/* Global variables */
static unsigned char *previousCells = NULL; /* previous pattern displayed */
static unsigned char *translatedCells = NULL; /* buffer to prepare new pattern */

/* Voltage: from 0->300V to 255->200V.
 * Presumably this is voltage for dot firmness.
 * Presumably 0 makes dots hardest, 255 makes them softest.
 * We are told 265V is normal operating voltage but we don't know the scale.
 */
static int
setFirmness (BrailleDisplay *brl, BrailleFirmness setting) {
  unsigned char voltage = 0XFF - (setting * 0XFF / BRL_FIRMNESS_MAXIMUM);
  logMessage(LOG_DEBUG, "setting display voltage: %02X", voltage);
  return io->protocol->setDisplayVoltage(brl, voltage);
}

static int
soundBeep (BrailleDisplay *brl, unsigned char duration) {
  if (!io->protocol->soundBeep(brl, duration)) return 0;
  approximateDelay(duration);
  return 1;
}

static int
brl_construct (BrailleDisplay *brl, char **parameters, const char *device) {
  if (isSerialDevice(&device)) {
    io = &serialInputOutputOperations;
  } else if (isUsbDevice(&device)) {
    io = &usbInputOutputOperations;
  } else if (isBluetoothDevice(&device)) {
    io = &bluetoothInputOutputOperations;
  } else {
    unsupportedDevice(device);
    return 0;
  }

  if (io->openPort(parameters, device)) {
    if (io->protocol->getCellCount(brl, &cellCount)) {
      deviceModel = deviceModels;

      while (deviceModel->deviceType) {
        if (deviceModel->reportedCellCount == cellCount) {
          const DeviceType *deviceType = deviceModel->deviceType;

          cellCount = deviceModel->actualCellCount;
          logMessage(LOG_INFO, "Device Type: %s", deviceType->productName);
          logMessage(LOG_INFO, "Cell Count: %u", cellCount);

          io->protocol->logSerialNumber(brl);
          io->protocol->logHardwareVersion(brl);
          io->protocol->logFirmwareVersion(brl);

          /* translatedCells holds the status cells and the text cells.
           * We export directly to BRLTTY only the text cells.
           */
          brl->textColumns = cellCount;		/* initialize size of display */
          brl->textRows = 1;		/* always 1 */

          {
            const KeyTableDefinition *ktd = deviceType->keyTableDefinition;
            brl->keyBindings = ktd->bindings;
            brl->keyNameTables = ktd->names;
          }

          brl->setFirmness = setFirmness;

          if ((previousCells = malloc(cellCount))) {
            if ((translatedCells = malloc(cellCount))) {
              if (io->protocol->setDisplayState(brl, 1)) {
                makeOutputTable(dotsTable_ISO11548_1);
                keysInitialized = 0;
                cellsInitialized = 0;

                soundBeep(brl, READY_BEEP_DURATION);
                return 1;
              }

              free(translatedCells);
              translatedCells = NULL;
            } else {
              logMallocError();
            }

            free(previousCells);
            previousCells = NULL;
          } else {
            logMallocError();
          }

          break;
        }

        deviceModel += 1;
      }

      if (!deviceModel->reportedCellCount) {
        logMessage(LOG_ERR, "unsupported cell count: %u", cellCount);
        deviceModel = NULL;
      }
    }

    io->closePort();
  }

  return 0;
}

static void
brl_destruct (BrailleDisplay *brl) {
  io->closePort();

  if (translatedCells) {
    free(translatedCells);
    translatedCells = NULL;
  }

  if (previousCells) {
    free(previousCells);
    previousCells = NULL;
  }
}

static int
brl_writeWindow (BrailleDisplay *brl, const wchar_t *text) {
  unsigned int from = 0;
  unsigned int to = cellCount;
  int changed;

  if (!cellsInitialized) {
    memcpy(previousCells, brl->buffer, cellCount);
    changed = 1;
    cellsInitialized = 1;
  } else if (deviceModel->partialUpdates) {
    changed = cellsHaveChanged(previousCells, brl->buffer, cellCount, &from, &to);
  } else {
    changed = cellsHaveChanged(previousCells, brl->buffer, cellCount, NULL, NULL);
  }

  if (changed) {
    translateOutputCells(&translatedCells[from], &brl->buffer[from], to-from);
    if (!deviceModel->writeBraille(brl, translatedCells, to-from, from)) return 0;
  }

  return 1;
}

static int
brl_readCommand (BrailleDisplay *brl, BRL_DriverCommandContext context) {
  return io->protocol->updateKeys(brl)? EOF: BRL_CMD_RESTARTBRL;
}
