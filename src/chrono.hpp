#pragma once

#include <sdk.hpp>
#include <Server/Components/Pawn/pawn.hpp>

#include <chrono>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>

namespace chrono {

using std::chrono::days;
using std::chrono::local_days;
using std::chrono::local_seconds;
using std::chrono::seconds;
using std::chrono::sys_seconds;
using std::chrono::time_zone;

/**
 * @brief      `INVALID_TIMESTAMP` of the include, for a time a cell cannot hold.
 */
constexpr cell InvalidTimestamp = std::numeric_limits<cell>::min();

/**
 * @brief      A year in a duration, of 365 days.
 */
constexpr int64_t YearInSeconds = 365 * 86400;

/**
 * @brief      The offset of a zone at a time and its abbreviation, which lasts
 *             until the next zone is read.
 */
struct Offset {
	seconds offset{};
	std::string_view abbreviation;
};

/**
 * @brief      The English names of the days, from Sunday.
 */
extern const char* const WeekdayNames[7];

/**
 * @brief      The English names of the months.
 */
extern const char* const MonthNames[12];

/**
 * @brief      Gives the Pawn component, whose scripts the natives read.
 */
void attach(IPawnComponent* pawn);

/**
 * @brief      Adds the natives to a script.
 */
void registerNatives(IPawnScript& script);

/**
 * @brief      Reads the time zones and keeps the one the config names, the zone of
 *             the machine for an empty name, or UTC when neither is found.
 */
void load(ICore& core, StringView name);

/**
 * @brief      The zone of the server, `nullptr` for UTC when the machine has no
 *             zones.
 */
const time_zone* serverZone();

/**
 * @brief      Makes a zone the zone of the server.
 */
void setServerZone(const time_zone* zone);

/**
 * @brief      The zone of the machine, or nothing when it cannot be told.
 */
std::optional<const time_zone*> machineZone();

/**
 * @brief      The zone of a name, the zone of the server for an empty name, or
 *             nothing when no zone has that name.
 */
std::optional<const time_zone*> findZone(std::string_view name);

/**
 * @brief      The offset of a zone at a time. A zone of `nullptr` is UTC.
 */
Offset offsetOf(const time_zone* zone, sys_seconds time);

/**
 * @brief      A time in UTC as the clocks of a zone show it.
 */
local_seconds toLocal(const time_zone* zone, sys_seconds time);

/**
 * @brief      A time the clocks of a zone show, in UTC.
 *
 * A time the zone skips or repeats when its clocks change is taken as the
 * earliest it can be.
 */
sys_seconds toSys(const time_zone* zone, local_seconds time);

/**
 * @brief      Writes a time with the specifiers of `strftime` and C++, in English
 *             whatever the locale.
 *
 * @return     `false` at a specifier it does not know.
 */
bool formatTime(const time_zone* zone, sys_seconds time, std::string_view format, std::string& out);

/**
 * @brief      Writes a duration of seconds in words, or with the specifiers `%y`,
 *             `%d`, `%h`, `%m` and `%s`, in two digits when in capitals.
 *
 * @return     `false` at a specifier it does not know.
 */
bool formatDuration(int64_t total, std::string_view format, std::string& out);

/**
 * @brief      Reads a time of a zone, or of the offset the text gives.
 *
 * @return     Nothing when the text does not match the format or names no
 *             whole date.
 */
std::optional<sys_seconds> parseTime(std::string_view text, std::string_view format, const time_zone* zone);

/**
 * @brief      Reads a duration such as `1d 3h 10m 5s 5ms` in milliseconds.
 */
bool parseDuration(std::string_view text, int64_t& total);

}
