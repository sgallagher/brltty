/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2015 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://brltty.com/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <string.h>
#include <errno.h>

#include "log.h"
#include "ascii.h"

#define BRL_STATUS_FIELDS sfWindowCoordinates
#define BRL_HAVE_STATUS_CELLS

#include "brl_driver.h"
#include "brldefs-md.h"

#define PROBE_RETRY_LIMIT 2
#define PROBE_INPUT_TIMEOUT 1000

#define MAXIMUM_TEXT_CELLS 80
#define MAXIMUM_STATUS_CELLS 2

BEGIN_KEY_NAME_TABLE(navigation)
  BRL_KEY_NAME_ENTRY(MD, NAV, F1, "F1"),
  BRL_KEY_NAME_ENTRY(MD, NAV, F2, "F2"),
  BRL_KEY_NAME_ENTRY(MD, NAV, F3, "F3"),
  BRL_KEY_NAME_ENTRY(MD, NAV, F4, "F4"),
  BRL_KEY_NAME_ENTRY(MD, NAV, F5, "F5"),
  BRL_KEY_NAME_ENTRY(MD, NAV, F6, "F6"),
  BRL_KEY_NAME_ENTRY(MD, NAV, F7, "F7"),
  BRL_KEY_NAME_ENTRY(MD, NAV, F8, "F8"),
  BRL_KEY_NAME_ENTRY(MD, NAV, F9, "F9"),
  BRL_KEY_NAME_ENTRY(MD, NAV, F10, "F10"),

  BRL_KEY_NAME_ENTRY(MD, NAV, LEFT, "Left"),
  BRL_KEY_NAME_ENTRY(MD, NAV, UP, "Up"),
  BRL_KEY_NAME_ENTRY(MD, NAV, RIGHT, "Right"),
  BRL_KEY_NAME_ENTRY(MD, NAV, DOWN, "Down"),

  BRL_KEY_NAME_ENTRY(MD, NAV, SHIFT, "Shift"),
  BRL_KEY_NAME_ENTRY(MD, NAV, LONG, "Long"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(braille)
  BRL_KEY_NAME_ENTRY(MD, BRL, DOT1, "Dot1"),
  BRL_KEY_NAME_ENTRY(MD, BRL, DOT2, "Dot2"),
  BRL_KEY_NAME_ENTRY(MD, BRL, DOT3, "Dot3"),
  BRL_KEY_NAME_ENTRY(MD, BRL, DOT4, "Dot4"),
  BRL_KEY_NAME_ENTRY(MD, BRL, DOT5, "Dot5"),
  BRL_KEY_NAME_ENTRY(MD, BRL, DOT6, "Dot6"),
  BRL_KEY_NAME_ENTRY(MD, BRL, DOT7, "Dot7"),
  BRL_KEY_NAME_ENTRY(MD, BRL, DOT8, "Dot8"),
  BRL_KEY_NAME_ENTRY(MD, BRL, SPACE, "Space"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(routing)
  BRL_KEY_GROUP_ENTRY(MD, RK, "RoutingKey"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(status)
  BRL_KEY_GROUP_ENTRY(MD, SK, "StatusKey"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLES(all)
  KEY_NAME_TABLE(navigation),
  KEY_NAME_TABLE(braille),
  KEY_NAME_TABLE(routing),
  KEY_NAME_TABLE(status),
END_KEY_NAME_TABLES

DEFINE_KEY_TABLE(all)

BEGIN_KEY_TABLE_LIST
  &KEY_TABLE_DEFINITION(all),
END_KEY_TABLE_LIST

struct BrailleDataStruct {
  struct {
    unsigned char cells[MAXIMUM_TEXT_CELLS];
    int rewrite;
  } text;

  struct {
    unsigned char cells[MAXIMUM_STATUS_CELLS];
    int rewrite;
  } status;
};

static uint16_t
calculateChecksum (const unsigned char *from, const unsigned char *to) {
  uint16_t checksum = 0;

  while (from < to) {
    checksum += *from++;
  }

  return checksum ^ 0XAA55;
}

static int
writeBytes (BrailleDisplay *brl, const unsigned char *bytes, size_t count) {
  return writeBraillePacket(brl, NULL, bytes, count);
}

static int
writePacket (BrailleDisplay *brl, unsigned char code, const void *data, unsigned char length) {
  MD_Packet packet;
  unsigned char *byte = packet.fields.data.bytes;

  packet.fields.soh = SOH;
  packet.fields.stx = STX;
  packet.fields.etx = ETX;

  packet.fields.code = code;
  packet.fields.length = length;
  byte = mempcpy(byte, data, length);

  uint16_t checksum = calculateChecksum(&packet.fields.stx, byte);
  *byte++ = checksum & 0XFF;
  *byte++ = checksum >> 8;

  return writeBytes(brl, packet.bytes, byte-packet.bytes);
}

static BraillePacketVerifierResult
verifyPacket (
  BrailleDisplay *brl,
  const unsigned char *bytes, size_t size,
  size_t *length, void *data
) {
  unsigned char byte = bytes[size-1];

  switch (size) {
    case 1:
      if (byte != SOH) return BRL_PVR_INVALID;
      *length = 5;
      break;

    case 2:
      if (byte != STX) return BRL_PVR_INVALID;
      break;

    case 4:
      *length += byte + 2;
      break;

    case 5:
      if (byte != ETX) return BRL_PVR_INVALID;
      break;

    default:
      if (size == *length) {
        const unsigned char *from = &bytes[1];
        const unsigned char *to = &bytes[size-2];
        uint16_t checksum = (to[1] << 8) | to[0];

        if (checksum != calculateChecksum(from, to)) {
          return BRL_PVR_INVALID;
        }
      }

      break;
  }

  return BRL_PVR_INCLUDE;
}

static size_t
readBytes (BrailleDisplay *brl, void *packet, size_t size) {
  int ok = readBraillePacket(brl, NULL, packet, size, verifyPacket, NULL);

  if (ok) {
    if (!writePacket(brl, MD_CODE_ACKNOWLEDGE, NULL, 0)) {
      brl->hasFailed = 1;
    }
  }

  return ok;
}

static size_t
readPacket (BrailleDisplay *brl, MD_Packet *packet) {
  return readBytes(brl, packet, sizeof(*packet));
}

static int
connectResource (BrailleDisplay *brl, const char *identifier) {
  static const SerialParameters serialParameters = {
    SERIAL_DEFAULT_PARAMETERS,
    .baud = 19200
  };

  BEGIN_USB_CHANNEL_DEFINITIONS
  END_USB_CHANNEL_DEFINITIONS

  GioDescriptor descriptor;
  gioInitializeDescriptor(&descriptor);

  descriptor.serial.parameters = &serialParameters;

  descriptor.usb.channelDefinitions = usbChannelDefinitions;

  descriptor.bluetooth.discoverChannel = 1;

  if (connectBrailleResource(brl, identifier, &descriptor, NULL)) {
    return 1;
  }

  return 0;
}

static int
writeIdentityRequest (BrailleDisplay *brl) {
  return writePacket(brl, MD_CODE_IDENTIFY, NULL, 0);
}

static BrailleResponseResult
isIdentityResponse (BrailleDisplay *brl, const void *packet, size_t size) {
  const MD_Packet *response = packet;
  unsigned char code = response->fields.code;

  if (code == MD_CODE_IDENTITY) return BRL_RSP_DONE;
  if (code == MD_CODE_ACKNOWLEDGE) return BRL_RSP_CONTINUE;
  return BRL_RSP_UNEXPECTED;
}

static int
brl_construct (BrailleDisplay *brl, char **parameters, const char *device) {
  if ((brl->data = malloc(sizeof(*brl->data)))) {
    memset(brl->data, 0, sizeof(*brl->data));

    if (connectResource(brl, device)) {
      MD_Packet response;

      if (probeBrailleDisplay(brl, PROBE_RETRY_LIMIT, NULL, PROBE_INPUT_TIMEOUT,
                              writeIdentityRequest,
                              readBytes, &response, sizeof(response),
                              isIdentityResponse)) {
        logMessage(LOG_INFO,
          "MDV Model Description:"
          " Version:%u.%u Text:%u Status:%u Dots:%u Routing:%s",
          response.fields.data.identity.majorVersion,
          response.fields.data.identity.minorVersion,
          response.fields.data.identity.textCellCount,
          response.fields.data.identity.statusCellCount,
          response.fields.data.identity.dotsPerCell,
          (response.fields.data.identity.haveRoutingKeys? "yes": "no")
        );

        brl->textColumns = response.fields.data.identity.textCellCount;
        brl->statusColumns = response.fields.data.identity.statusCellCount;

        setBrailleKeyTable(brl, &KEY_TABLE_DEFINITION(all));
        MAKE_OUTPUT_TABLE(0X08, 0X04, 0X02, 0X80, 0X40, 0X20, 0X01, 0X10);

        brl->data->text.rewrite = 1;
        brl->data->status.rewrite = 1;

        return 1;
      }

      disconnectBrailleResource(brl, NULL);
    }

    free(brl->data);
  } else {
    logMallocError();
  }

  return 0;
}

static void
brl_destruct (BrailleDisplay *brl) {
  disconnectBrailleResource(brl, NULL);

  if (brl->data) {
    free(brl->data);
    brl->data = NULL;
  }
}

static int
brl_writeStatus (BrailleDisplay *brl, const unsigned char *cells) {
  if (cellsHaveChanged(brl->data->status.cells, cells, brl->statusColumns, NULL, NULL, &brl->data->status.rewrite)) {
    brl->data->text.rewrite = 1;
  }

  return 1;
}

static int
brl_writeWindow (BrailleDisplay *brl, const wchar_t *text) {
  if (cellsHaveChanged(brl->data->text.cells, brl->buffer, brl->textColumns, NULL, NULL, &brl->data->text.rewrite)) {
    unsigned char cells[brl->statusColumns + brl->textColumns];
    unsigned char *cell = cells;

    cell = mempcpy(cell, brl->data->status.cells, brl->statusColumns);
    cell = translateOutputCells(cell, brl->data->text.cells, brl->textColumns);

    if (!writePacket(brl, MD_CODE_WRITE_ALL, cells, (cell - cells))) return 0;
  }

  return 1;
}

static int
brl_readCommand (BrailleDisplay *brl, KeyTableCommandContext context) {
  MD_Packet packet;
  size_t size;

  while ((size = readPacket(brl, &packet))) {
    switch (packet.fields.code) {
      case MD_CODE_NAVIGATION_KEY: {
        unsigned char key = packet.fields.data.navigationKey.key;

        if (key == MD_NAV_SHIFT_PRESS) break;
        if (key == MD_NAV_SHIFT_RELEASE) break;

        int shiftPressed = (key & MD_NAV_SHIFT) != 0;
        int longPressed = (key & MD_NAV_LONG) != 0;

        key &= MD_NAV_MASK_KEY;
        MD_KeyGroup group = MD_GRP_NAV;

        if (shiftPressed) enqueueKeyEvent(brl, group, MD_NAV_SHIFT, 1);
        if (longPressed) enqueueKeyEvent(brl, group, MD_NAV_LONG, 1);
        enqueueKey(brl, group, key);
        if (longPressed) enqueueKeyEvent(brl, group, MD_NAV_LONG, 0);
        if (shiftPressed) enqueueKeyEvent(brl, group, MD_NAV_SHIFT, 0);

        break;
      }

      case MD_CODE_BRAILLE_KEY: {
        MD_KeyGroup group = MD_GRP_BRL;
        unsigned char spacePressed = packet.fields.data.brailleKey.isChord != 0;

        if (spacePressed) enqueueKeyEvent(brl, group, MD_BRL_SPACE, 1);
        enqueueKeys(brl, packet.fields.data.brailleKey.dots, group, 0);
        if (spacePressed) enqueueKeyEvent(brl, group, MD_BRL_SPACE, 0);

        break;
      }

      {
        unsigned char key;
        int press;

      case MD_CODE_ROUTING_PRESS:
        key = packet.fields.data.routingPress.key;
        press = 1;
        goto doRoutingKey;

      case MD_CODE_ROUTING_RELEASE:
        key = packet.fields.data.routingRelease.key;
        press = 0;
        goto doRoutingKey;

      doRoutingKey:
        key &= ~MD_ROUTING_SHIFT;

        if (key >= MD_ROUTING_FIRST) {
          key -= MD_ROUTING_FIRST;
          MD_KeyGroup group;

          if (key < brl->statusColumns) {
            group = MD_GRP_SK;
          } else if ((key -= brl->statusColumns) < brl->textColumns) {
            group = MD_GRP_RK;
          } else {
            break;
          }

          enqueueKeyEvent(brl, group, key, press);
        }

        break;
      }

      case MD_CODE_ACKNOWLEDGE:
        break;

      default:
        break;
    }

    logUnexpectedPacket(packet.bytes, size);
  }

  return (errno == EAGAIN)? EOF: BRL_CMD_RESTARTBRL;
}
