#if defined _INC_CHRONO
    #endinput
#endif
#define _INC_CHRONO
#define _chrono_included

/**
 * Chrono for open.mp: timestamps, units of time with tags, the calendar and
 * time zones, in place of the pawn-chrono plugin. Scripts written for it keep
 * working, but `TimeParse` and `DurationParse` now return `true` when they
 * read the text, and `MonthInMilliseconds`, which did not fit a cell, is gone.
 *
 * A `Timestamp` is the seconds since 1970-01-01 UTC in a cell, so it reaches
 * 2038-01-19. The operators keep the tags apart: `Now() + Days:3`, and
 * `Timestamp - Timestamp` gives `Seconds`. Months and years follow the
 * calendar, so January 31st and one month is the last day of February.
 *
 * Dates are read and written in the time zone of the server, which
 * `chrono.time_zone` names in `config.json`, or the zone of the machine when it
 * names none. Each function that reads or writes a date also takes a zone, as
 * `"UTC"`, which the `*_FULL_UTC` formats need. The specifiers are those of
 * C++, as `%Y-%m-%d %H:%M:%S`.
 *
 * The tags, constants and specifier names come from pawn-chrono, Copyright (c)
 * 2018 Barnaby "Southclaws" Keene, under the MIT License.
 */

/**
 * # Header
 */

#define TIME_UNITS {Milliseconds,Seconds,Minutes,Hours,Days,Weeks,Months,Years}

#define INVALID_TIMESTAMP (Timestamp:cellmin)

// Each unit of time in the smaller ones. A month and a year are averages, of
// 30.44 and 365.24 days, but in days, where they are whole.

const
    Milliseconds:SecondInMilliseconds = Milliseconds:1_000,
    Milliseconds:MinuteInMilliseconds = Milliseconds:60_000,
    Milliseconds:HourInMilliseconds = Milliseconds:3_600_000,
    Milliseconds:DayInMilliseconds = Milliseconds:86_400_000,
    Milliseconds:WeekInMilliseconds = Milliseconds:604_800_000 // A month or a year of milliseconds does not fit a cell.
;

const
    Seconds:MinuteInSeconds = Seconds:60,
    Seconds:HourInSeconds = Seconds:3_600,
    Seconds:DayInSeconds = Seconds:86_400,
    Seconds:WeekInSeconds = Seconds:604_800,
    Seconds:MonthInSeconds = Seconds:2_629_746,
    Seconds:YearInSeconds = Seconds:31_556_952
;

const
    Minutes:HourInMinutes = Minutes:60,
    Minutes:DayInMinutes = Minutes:1_440,
    Minutes:WeekInMinutes = Minutes:10_080,
    Minutes:MonthInMinutes = Minutes:43_829,
    Minutes:YearInMinutes = Minutes:525_948
;

const
    Hours:DayInHours = Hours:24,
    Hours:WeekInHours = Hours:168,
    Hours:MonthInHours = Hours:730,
    Hours:YearInHours = Hours:8_760
;

const
    Days:WeekInDays = Days:7,
    Days:MonthInDays = Days:30, // 30.4375 on average.
    Days:YearInDays = Days:365 // Leap years left out.
;

const
    Weeks:YearInWeeks = Weeks:52
;

// The specifiers of `TimeFormat` and `TimeParse`. Names are always in English,
// and each example is Saturday, June 2nd 2018 at 08:47:12.

#define WEEKDAY_NAME_ABV  "%a"  // The name of the day, short: Sat.
#define WEEKDAY_NAME      "%A"  // The name of the day: Saturday.
#define MONTH_NAME_ABV    "%b"  // The name of the month, short: Jun.
#define MONTH_NAME        "%B"  // The name of the month: June.
#define YEAR_DIV          "%C"  // The year divided by 100, in two digits: 20.
#define DAY_OF_MONTH      "%d"  // The day of the month, in two digits: 02.
#define DATE_MDY          "%D"  // The same as %m/%d/%y, the American way: 06/02/18.
#define DAY_OF_MONTH_PAD  "%e"  // The day of the month, a space before one digit: " 2".
#define DATE_YMD          "%F"  // The same as %Y-%m-%d, the ISO 8601 way: 2018-06-02.
#define WEEK_NUMBER_ISO   "%g"  // The last two digits of the year of the ISO week: 18.
#define YEAR_PAD          "%G"  // The year of the ISO week, which the first days of January may not share: 2018.
#define HOUR_24           "%H"  // The hour of a 24 hour clock, in two digits: 08.
#define HOUR_12           "%I"  // The hour of a 12 hour clock, in two digits: 08.
#define DAY_OF_YEAR       "%j"  // The day of the year, in three digits: 153.
#define MONTH             "%m"  // The month, in two digits: 06.
#define MINUTE            "%M"  // The minute, in two digits: 47.
#define NEWLINE_CHAR      "%n"  // A new line.
#define MERIDIAN          "%p"  // AM or PM.
#define TIME_12           "%r"  // The time of a 12 hour clock: 08:47:12 AM.
#define HM_COLON          "%R"  // The same as %H:%M: 08:47.
#define SECOND            "%S"  // The second, in two digits: 12.
#define TAB_CHAR          "%t"  // A tab.
#define HMS_COLON         "%T"  // The same as %H:%M:%S: 08:47:12.
#define WEEK_DAY          "%u"  // The day of the week, from 1 on Monday to 7 on Sunday: 6.
#define WEEK_OF_YEAR      "%U"  // The week of the year, weeks from Sunday, the days before the first Sunday in week 00: 21.
#define WEEK_OF_YEAR_ZERO "%V"  // The ISO week of the year, from 01: 22.
#define WEEK_DAY_ZERO     "%w"  // The day of the week, from 0 on Sunday to 6 on Saturday: 6.
#define WEEK_OF_YEAR_ALT  "%W"  // The week of the year, weeks from Monday, the days before the first Monday in week 00: 22.
#define LOCALE_DATE       "%x"  // The date, as %m/%d/%y: 06/02/18.
#define LOCALE_TIME       "%X"  // The time, as %H:%M:%S: 08:47:12.
#define YEAR_ABV          "%y"  // The last two digits of the year: 18.
#define YEAR              "%Y"  // The year, in four digits: 2018.
#define UTC_OFFSET        "%z"  // The offset of the zone from UTC: +0100 in London. %Ez writes +01:00.
#define TIME_ZONE_ABV     "%Z"  // The abbreviation of the zone, as the zones of the machine name it.
#define PERCENTAGE        "%%"  // A % sign.

#define HUMAN_DATE         "%d/%m/%y"           // 02/06/18
#define ISO8601_TIME       "%H:%M:%S"           // 08:47:12
#define ISO8601_DATE       "%Y-%m-%d"           // 2018-06-02
#define ISO8601_FULL_UTC   "%Y-%m-%dT%H:%M:%SZ" // 2018-06-02T08:47:12Z, for a time written with the zone "UTC".
#define ISO8601_FULL_LOCAL "%Y-%m-%dT%H:%M:%S"  // 2018-06-02T08:47:12

#define ISO6801_TIME       ISO8601_TIME
#define ISO6801_DATE       ISO8601_DATE
#define ISO6801_FULL_UTC   ISO8601_FULL_UTC
#define ISO6801_FULL_LOCAL ISO8601_FULL_LOCAL

#define DURATION_CLOCK "%H:%M:%S" // 26:03:04 for 1 day, 2 hours, 3 minutes and 4 seconds.

/**
 * # Functions
 */

/**
 * @brief      The time now.
 *
 * @return     The seconds since 1970-01-01 00:00:00 UTC.
 */
native Timestamp:Now();

/**
 * @brief      Write a time as text.
 *
 * @param      timestamp  The time.
 * @param      format     The text with specifiers, as `%Y-%m-%d %H:%M:%S`.
 * @param      output     Where the text goes.
 * @param      size       The size of `output`.
 * @param      zone       The time zone, or the zone of the server when left
 *                        out.
 *
 * @return     - `true` it was written
 *             - `false` the format or the zone is not valid
 */
native bool:TimeFormat(Timestamp:timestamp, const format[], output[], size = sizeof (output), const zone[] = "");

/**
 * @brief      Read a time from text.
 *
 * A time of day left out is midnight, and with no offset (`%z`) the text is
 * read as a time of the zone.
 *
 * @param      string  The text.
 * @param      format  How it is written, as `%Y-%m-%d %H:%M`.
 * @param      output  The time read.
 * @param      zone    The time zone, or the zone of the server when left out.
 *
 * @return     - `true` it was read
 *             - `false` when:
 *                 + the text does not match the format
 *                 + the time does not fit a `Timestamp`
 *                 + the zone is not valid
 */
native bool:TimeParse(const string[], const format[], &Timestamp:output, const zone[] = "");

/**
 * @brief      Read a duration such as `1h 30m` or `2d12h`.
 *
 * The units are `ms`, `s`, `m`, `h`, `d`, `w` and `y`, a year of 365 days.
 *
 * @param      string  The text.
 * @param      output  The duration read.
 *
 * @return     - `true` it was read
 *             - `false` it is not a duration, or it is longer than 24 days
 */
native bool:DurationParse(const string[], &Milliseconds:output);

/**
 * @brief      Read a duration as `DurationParse` does, to the second.
 *
 * @param      string  The text.
 * @param      output  The duration read, its milliseconds left out.
 *
 * @return     - `true` it was read
 *             - `false` it is not a duration, or it is longer than 68 years
 */
native bool:DurationParseSeconds(const string[], &Seconds:output);

/**
 * @brief      Write a duration as text.
 *
 * With no format it is written in words, as `1 day, 2 hours, 3 minutes,
 * 4 seconds`, leaving out what is zero. A format takes `%y` years of 365 days,
 * `%d` days, `%h` hours, `%m` minutes and `%s` seconds, in two digits when in
 * capitals. Each holds what the larger ones in the format leave, so `%H:%M:%S`
 * writes `26:03:04` and `%dd %hh %mm %ss` writes `1d 2h 3m 4s`.
 *
 * @param      duration  The duration, with `-` before it below zero.
 * @param      output    Where the text goes.
 * @param      size      The size of `output`.
 * @param      format    The text with specifiers, or words when left out.
 *
 * @return     - `true` it was written
 *             - `false` the format has a specifier it does not know
 */
native bool:DurationFormat(Seconds:duration, output[], size = sizeof (output), const format[] = "");

/**
 * @brief      The date of a time.
 *
 * @param      timestamp  The time.
 * @param      year       The year.
 * @param      month      The month, `1` to `12`.
 * @param      day        The day of the month.
 * @param      zone       The time zone, or the zone of the server when left
 *                        out.
 *
 * @return     - `true` it was read
 *             - `false` the zone is not valid
 */
native bool:GetTimestampDate(Timestamp:timestamp, &year, &month, &day, const zone[] = "");

/**
 * @brief      The time of day of a time.
 *
 * @param      timestamp  The time.
 * @param      hour       The hour, `0` to `23`.
 * @param      minute     The minute.
 * @param      second     The second.
 * @param      zone       The time zone, or the zone of the server when left
 *                        out.
 *
 * @return     - `true` it was read
 *             - `false` the zone is not valid
 */
native bool:GetTimestampTime(Timestamp:timestamp, &hour, &minute, &second, const zone[] = "");

/**
 * @brief      The day of the week of a time.
 *
 * @param      timestamp  The time.
 * @param      zone       The time zone, or the zone of the server when left
 *                        out.
 *
 * @return     `0` for Sunday to `6` for Saturday, or `-1` when the zone is not
 *             valid.
 */
native GetTimestampWeekday(Timestamp:timestamp, const zone[] = "");

/**
 * @brief      The time of a date.
 *
 * A value past its range carries over, so the 32nd of January is the 1st of
 * February.
 *
 * @param      year    The year.
 * @param      month   The month.
 * @param      day     The day of the month.
 * @param      hour    The hour.
 * @param      minute  The minute.
 * @param      second  The second.
 * @param      zone    The time zone, or the zone of the server when left out.
 *
 * @return     The time, or `INVALID_TIMESTAMP` when it does not fit a
 *             `Timestamp` or the zone is not valid.
 */
native Timestamp:MakeTimestamp(year, month, day, hour = 0, minute = 0, second = 0, const zone[] = "");

/**
 * @brief      Move a time by months of the calendar.
 *
 * A day the month does not have becomes its last, so January 31st and one
 * month is the last day of February.
 *
 * @param      timestamp  The time.
 * @param      months     How many months, below zero to go back.
 * @param      zone       The time zone, or the zone of the server when left
 *                        out.
 *
 * @return     The time, or `INVALID_TIMESTAMP` when it does not fit a
 *             `Timestamp` or the zone is not valid.
 */
native Timestamp:AddTimestampMonths(Timestamp:timestamp, months, const zone[] = "");

/**
 * @brief      Change the time zone of the server.
 *
 * @param      zone  The zone, as `"Europe/London"`, or the zone of the machine
 *                   when empty.
 *
 * @return     - `true` it was changed
 *             - `false` there is no such zone
 */
native bool:SetChronoTimeZone(const zone[]);

/**
 * @brief      The time zone of the server.
 *
 * @param      output  Where its name goes.
 * @param      size    The size of `output`.
 *
 * @return     - `true` it was written
 *             - `false` `output` has no room
 */
native bool:GetChronoTimeZone(output[], size = sizeof (output));

/**
 * # External
 */

stock Seconds:operator-(Timestamp:future, Timestamp:past) {
    return Seconds:(_:future - _:past);
}

stock Timestamp:operator+(Timestamp:timestamp, Seconds:duration) {
    return Timestamp:(_:timestamp + _:duration);
}

stock Timestamp:operator-(Timestamp:timestamp, Seconds:duration) {
    return Timestamp:(_:timestamp - _:duration);
}

stock Timestamp:operator+(Timestamp:timestamp, Minutes:duration) {
    return Timestamp:(_:timestamp + _:MinuteInSeconds * _:duration);
}

stock Timestamp:operator-(Timestamp:timestamp, Minutes:duration) {
    return Timestamp:(_:timestamp - _:MinuteInSeconds * _:duration);
}

stock Timestamp:operator+(Timestamp:timestamp, Hours:duration) {
    return Timestamp:(_:timestamp + _:HourInSeconds * _:duration);
}

stock Timestamp:operator-(Timestamp:timestamp, Hours:duration) {
    return Timestamp:(_:timestamp - _:HourInSeconds * _:duration);
}

stock Timestamp:operator+(Timestamp:timestamp, Days:duration) {
    return Timestamp:(_:timestamp + _:DayInSeconds * _:duration);
}

stock Timestamp:operator-(Timestamp:timestamp, Days:duration) {
    return Timestamp:(_:timestamp - _:DayInSeconds * _:duration);
}

stock Timestamp:operator+(Timestamp:timestamp, Weeks:duration) {
    return Timestamp:(_:timestamp + _:WeekInSeconds * _:duration);
}

stock Timestamp:operator-(Timestamp:timestamp, Weeks:duration) {
    return Timestamp:(_:timestamp - _:WeekInSeconds * _:duration);
}

stock Timestamp:operator+(Timestamp:timestamp, Months:duration) {
    return AddTimestampMonths(timestamp, _:duration);
}

stock Timestamp:operator-(Timestamp:timestamp, Months:duration) {
    return AddTimestampMonths(timestamp, -_:duration);
}

stock Timestamp:operator+(Timestamp:timestamp, Years:duration) {
    return AddTimestampMonths(timestamp, _:duration * 12);
}

stock Timestamp:operator-(Timestamp:timestamp, Years:duration) {
    return AddTimestampMonths(timestamp, -_:duration * 12);
}
