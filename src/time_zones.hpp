#pragma once

#include <stdint.h>
#include <string.h>

enum class DaylightSaving : uint8_t {
  Automatic = 0,
  Standard = 1,
  Daylight = 2
};

struct TimeZoneTransition {
  uint32_t utc;
  int16_t offsetMinutes;
  bool daylight;
};

struct TimeZoneSchedule {
  uint16_t start;
  uint16_t count;
};

struct TimeZone {
  const char* name;
  uint16_t schedule;
};

#include "time_zones_data.inc"

inline const TimeZone* findTimeZone(const char* name) {
  size_t low = 0;
  size_t high = sizeof(TIME_ZONES) / sizeof(TIME_ZONES[0]);
  while (low < high) {
    size_t middle = low + (high - low) / 2;
    int comparison = strcmp(name, TIME_ZONES[middle].name);
    if (comparison == 0) {
      return &TIME_ZONES[middle];
    }
    if (comparison < 0) {
      high = middle;
    } else {
      low = middle + 1;
    }
  }
  return nullptr;
}

inline int16_t timeZoneOffset(const TimeZone& zone, uint64_t utc, DaylightSaving mode) {
  const TimeZoneSchedule& schedule = TIME_ZONE_SCHEDULES[zone.schedule];
  const TimeZoneTransition* transitions = TIME_ZONE_TRANSITIONS + schedule.start;
  uint16_t low = 0;
  uint16_t high = schedule.count;
  while (low + 1 < high) {
    uint16_t middle = low + (high - low) / 2;
    if (utc < transitions[middle].utc) {
      high = middle;
    } else {
      low = middle;
    }
  }

  const TimeZoneTransition& current = transitions[low];
  if (mode == DaylightSaving::Automatic || current.daylight == (mode == DaylightSaving::Daylight)) {
    return current.offsetMinutes;
  }

  // Use the nearest occurrence of the requested seasonal offset. A zone without
  // a nearby season (including fixed-offset zones) keeps its current offset.
  constexpr uint32_t seasonalWindow = 366u * 24u * 60u * 60u;
  uint32_t nearest = seasonalWindow;
  int16_t offset = current.offsetMinutes;
  for (uint16_t index = low; index > 0; --index) {
    uint64_t distance = utc - transitions[index].utc + 1;
    if (distance > nearest) {
      break;
    }
    if (transitions[index - 1].daylight != current.daylight) {
      nearest = distance;
      offset = transitions[index - 1].offsetMinutes;
      break;
    }
  }
  for (uint16_t index = high; index < schedule.count; ++index) {
    if (transitions[index].utc - utc > nearest) {
      break;
    }
    if (transitions[index].daylight != current.daylight) {
      return transitions[index].offsetMinutes;
    }
  }
  return offset;
}
