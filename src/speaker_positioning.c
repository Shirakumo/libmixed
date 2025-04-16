#include "internal.h"

const float default_speaker_positions[][3] =
  { 
    {-2.0, +0.0, +1.0}, // Left front bottom
    {+2.0, +0.0, +1.0}, // Right front bottom
    {-2.0, +0.0, -1.0}, // Left rear bottom
    {+2.0, +0.0, -1.0}, // Right rear bottom
    {+0.0, +0.0, +1.0}, // Center front
    {+0.0, +0.0, +0.0}, // LFE
    {-2.0, +0.0, +0.0}, // Left side
    {+2.0, +0.0, +0.0}, // Right side
    {-2.0, +1.0, +1.0}, // Left front top
    {+2.0, +1.0, +1.0}, // Right front top
    {-2.0, +1.0, -1.0}, // Left rear top
    {+2.0, +1.0, -1.0}, // Right rear top
    {+0.0, +0.0, -1.0}, // Center rear
    {-2.0, +1.0, +0.0}, // Left side Top
    {+2.0, +1.0, +0.0}, // Right side top
    {+0.0, +0.0, +0.0}, // LFE2
    {+4.0, +0.0, +4.0}, // Left front wide
    {-4.0, +0.0, +4.0}, // Right front wide
    {+0.0, +1.0, +0.0}, // Center top
    {+0.0, +1.0, +1.0}, // Center front top
    {+0.0, +1.0, -1.0}, // Center rear top
    {-2.0, +0.0, -1.0}, // Left rear center
    {+2.0, +0.0, -1.0}, // Right rear center
    {-2.0, +0.0, +0.0}, // LFE Left
    {+2.0, +0.0, +0.0}, // LFE Right
    {+1.0, +2.0, +1.0}, // Left front high
    {-1.0, +2.0, +1.0}, // Right front high
    {+0.0, +2.0, +1.0}, // Center front high
    {+0.0, -1.0, +0.0}, // Center bottom
    {-2.0, -1.0, +0.0}, // Left center bottom
    {+2.0, -1.0, +0.0}, // Right center bottom
    {+1.0, +1.0, +0.0}, // Left front center top
    {-1.0, -1.0, +0.0}, // Right front center bottom
  };

MIXED_EXPORT int mixed_default_speaker_position(float position[3], mixed_channel_t channel){
  if(channel < MIXED_MAX_SPEAKER_COUNT){
    memcpy(position, default_speaker_positions[channel], sizeof(float)*3);
    return 1;
  }
  mixed_err(MIXED_INVALID_VALUE);
  return 0;
}

const struct mixed_channel_configuration default_configurations[] =
  {
    /* 0.0 */ {0, {0}},
    /* 1.0 */ {1, {MIXED_MONO}},
    /* 2.0 */ {2, {MIXED_LEFT, MIXED_RIGHT}},
    /* 3.0 */ {3, {MIXED_LEFT, MIXED_RIGHT, MIXED_CENTER}},
    /* 4.0 */ {4, {MIXED_LEFT_FRONT, MIXED_RIGHT_FRONT, MIXED_LEFT_REAR, MIXED_RIGHT_REAR}},
    /* 5.0 */ {5, {MIXED_LEFT_FRONT, MIXED_RIGHT_FRONT, MIXED_CENTER, MIXED_LEFT_REAR, MIXED_RIGHT_REAR}},
    /* 5.1 */ {6, {MIXED_LEFT_FRONT, MIXED_RIGHT_FRONT, MIXED_CENTER, MIXED_SUBWOOFER, MIXED_LEFT_REAR, MIXED_RIGHT_REAR}},
    /* 6.1 */ {7, {MIXED_LEFT_FRONT, MIXED_RIGHT_FRONT, MIXED_CENTER, MIXED_SUBWOOFER, MIXED_CENTER_REAR, MIXED_LEFT_SIDE, MIXED_RIGHT_SIDE}},
    /* 7.1 */ {8, {MIXED_LEFT_FRONT, MIXED_RIGHT_FRONT, MIXED_CENTER, MIXED_SUBWOOFER, MIXED_LEFT_REAR, MIXED_RIGHT_REAR, MIXED_LEFT_SIDE, MIXED_RIGHT_SIDE}},
    /* 9.0 */ {9, {MIXED_LEFT_FRONT, MIXED_RIGHT_FRONT, MIXED_CENTER, MIXED_LEFT_REAR, MIXED_RIGHT_REAR, MIXED_LEFT_SIDE, MIXED_RIGHT_SIDE, MIXED_LEFT_FRONT_TOP, MIXED_RIGHT_FRONT_TOP}},
    /* 9.1 */ {10, {MIXED_LEFT_FRONT, MIXED_RIGHT_FRONT, MIXED_CENTER, MIXED_SUBWOOFER, MIXED_LEFT_REAR, MIXED_RIGHT_REAR, MIXED_LEFT_SIDE, MIXED_RIGHT_SIDE, MIXED_LEFT_FRONT_TOP, MIXED_RIGHT_FRONT_TOP}},
  };

MIXED_EXPORT struct mixed_channel_configuration const* mixed_default_channel_configuration(mixed_channel_t channel_count){
  if(0 < channel_count && channel_count < 8){
    return &default_configurations[channel_count];
  }
  mixed_err(MIXED_INVALID_VALUE);
  return 0;
}
