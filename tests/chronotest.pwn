#include <open.mp>
#include <chrono>

/**
 * Runtime test of the Chrono component. Its server runs with
 * `"chrono": { "time_zone": "Europe/London" }`, an hour ahead of UTC in
 * June 2018.
 */

#define TEST_TIME (Timestamp:1527929232)

static
    gPassed,
    gFailed
;

main() {
}

stock Check(bool:ok, const what[]) {
    if (ok) {
        ++gPassed;
        printf("[chronotest] PASS %s", what);
    } else {
        ++gFailed;
        printf("[chronotest] FAIL %s", what);
    }
}

stock CheckText(const got[], const expected[], const what[]) {
    new
        line[160]
    ;

    format(line, sizeof (line), "%s (\"%s\")", what, got);
    Check(strequal(got, expected), line);
}

stock TestFormat() {
    new
        text[64]
    ;

    TimeFormat(TEST_TIME, ISO8601_FULL_UTC, text, .zone = "UTC");
    CheckText(text, "2018-06-02T08:47:12Z", "a time in UTC");

    TimeFormat(TEST_TIME, "%Y-%m-%d %H:%M:%S", text);
    CheckText(text, "2018-06-02 09:47:12", "a time in the zone of the server");

    TimeFormat(TEST_TIME, "%A, %d %B %Y %z", text);
    CheckText(text, "Saturday, 02 June 2018 +0100", "names of days and months, and the offset");

    TimeFormat(TEST_TIME, "Year %Y, {%m} 100%%", text);
    CheckText(text, "Year 2018, {06} 100%", "text, braces and percent signs around the specifiers");

    TimeFormat(TEST_TIME, "%Y-%m-%d", text, 5);
    CheckText(text, "2018", "a format cut to the size of the output");

    Check(!TimeFormat(TEST_TIME, "%Y", text, .zone = "Nowhere/City"), "a zone that does not exist is refused");
    Check(!TimeFormat(TEST_TIME, "%Y %", text), "a format with a broken specifier is refused");
}

stock TestParse() {
    new
        Timestamp:time,
        line[96]
    ;

    Check(TimeParse("2018-06-02 09:47:12", "%Y-%m-%d %H:%M:%S", time) && time == TEST_TIME, "a time is read in the zone of the server");
    Check(TimeParse("2018-06-02T08:47:12+0000", "%Y-%m-%dT%H:%M:%S%z", time) && time == TEST_TIME, "an offset in the text wins over the zone");

    new const
        bool:dated = TimeParse("2018-06-02", "%Y-%m-%d", time, "UTC")
    ;

    format(line, sizeof (line), "a date alone is midnight (%i, %i)", dated, _:time);
    Check(dated && time == Timestamp:1527897600, line);

    time = Timestamp:7;
    Check(!TimeParse("not a date", "%Y-%m-%d", time) && time == Timestamp:7, "text that does not match is refused and leaves the output alone");

    Check(TimeParse("02/06/2018 09:47", "%d/%m/%Y %H:%M", time) && time == TEST_TIME - Seconds:12, "a day first date without seconds");
    Check(TimeParse("jun 2, 2018", "%b %e, %Y", time) && time == Timestamp:1527894000, "a month by name, in any case");
    Check(TimeParse("02/06/2018 09:47:12 AM", "%d/%m/%Y %I:%M:%S %p", time) && time == TEST_TIME, "a twelve hour clock");
    Check(TimeParse("2018-06-02T09:47:12+01:00", "%FT%T%Ez", time) && time == TEST_TIME, "an offset with a colon");
    Check(!TimeParse("2018-02-30", "%Y-%m-%d", time), "a day the month does not have is refused");
    Check(TimeParse("2018 153 09:47:12", "%Y %j %H:%M:%S", time) && time == TEST_TIME, "a day of the year");
    Check(TimeParse("2018-W22-6 09:47:12", "%G-W%V-%u %T", time) && time == TEST_TIME, "an ISO week and its day");
    Check(TimeParse("2018 21 6 09:47:12", "%Y %U %w %T", time) && time == TEST_TIME, "a week from Sunday and its day");
    Check(TimeParse("2018 22 Sat 09:47:12", "%Y %W %a %T", time) && time == TEST_TIME, "a week from Monday and its day");
    Check(TimeParse("Sat, 02 Jun 2018 09:47:12", "%a, %d %b %Y %T", time) && time == TEST_TIME, "a weekday that matches the date");
    Check(!TimeParse("Mon, 02 Jun 2018", "%a, %d %b %Y", time), "a weekday that does not match the date is refused");
    Check(TimeParse("2018-06-02 09:47:12 BST", "%F %T %Z", time) && time == TEST_TIME, "the abbreviation of a zone is read and left out");
    Check(TimeParse("20180602094712", "%4Y%2m%2d%2H%2M%2S", time) && time == TEST_TIME, "widths before the specifiers");
    Check(!TimeParse("09:47", "%H:%M", time), "a time with no date is refused");
    Check(!TimeParse("x", "%Q", time), "a specifier it does not know is refused");

    new
        text[32]
    ;

    TimeFormat(TEST_TIME, "%c", text);
    Check(TimeParse(text, "%c", time) && time == TEST_TIME, "%c reads back what it writes");
}

stock TestDurations() {
    new
        Milliseconds:milliseconds,
        Seconds:seconds,
        text[64]
    ;

    Check(DurationParse("8m5s", milliseconds) && milliseconds == Milliseconds:485000, "8m5s");
    Check(DurationParse("1d3h10m5s5ms", milliseconds) && milliseconds == Milliseconds:97805005, "1d3h10m5s5ms");
    Check(DurationParse("1h 30m", milliseconds) && milliseconds == Milliseconds:5400000, "spaces between the parts");
    Check(DurationParse("1W", milliseconds) && milliseconds == Milliseconds:604800000, "weeks, in capitals");
    Check(!DurationParse("25d", milliseconds), "milliseconds past 24 days are refused");
    Check(!DurationParse("5x", milliseconds) && !DurationParse("10", milliseconds) && !DurationParse("", milliseconds), "unknown units, a number alone and nothing are refused");

    Check(DurationParseSeconds("1y", seconds) && seconds == Seconds:31536000, "a year is 365 days");
    Check(DurationParseSeconds("2w 3d", seconds) && seconds == Seconds:1468800, "weeks and days in seconds");
    Check(DurationParseSeconds("1500ms", seconds) && seconds == Seconds:1, "milliseconds below a second are left out");

    text = "left over";
    DurationFormat(Seconds:93784, text);
    CheckText(text, "1 day, 2 hours, 3 minutes, 4 seconds", "a duration in words, over old text");

    DurationFormat(Seconds:3600, text);
    CheckText(text, "1 hour", "exactly one hour");

    DurationFormat(Seconds:0, text);
    CheckText(text, "0 seconds", "no time at all");

    DurationFormat(Seconds:93784, text, .format = "%dd %hh %mm %ss");
    CheckText(text, "1d 2h 3m 4s", "a format of days, hours, minutes and seconds");

    DurationFormat(Seconds:93784, text, .format = DURATION_CLOCK);
    CheckText(text, "26:03:04", "a clock holds the days in its hours");

    DurationFormat(Seconds:93784, text, .format = "%d days and %s seconds");
    CheckText(text, "1 days and 7384 seconds", "a unit holds what the larger ones leave");

    DurationFormat(Seconds:245, text, .format = "%m:%S");
    CheckText(text, "4:05", "two digits in capitals");

    DurationFormat(Seconds:-61, text, .format = "%m:%S");
    CheckText(text, "-1:01", "a duration below zero");

    Check(!DurationFormat(Seconds:1, text, .format = "%x"), "a specifier it does not know is refused");
}

stock TestCalendar() {
    new
        year,
        month,
        day,
        hour,
        minute,
        second,
        line[96]
    ;

    Check(GetTimestampDate(TEST_TIME, year, month, day, "UTC") && year == 2018 && month == 6 && day == 2, "the date of a time");
    Check(GetTimestampTime(TEST_TIME, hour, minute, second, "UTC") && hour == 8 && minute == 47 && second == 12, "the time of day in UTC");
    Check(GetTimestampTime(TEST_TIME, hour, minute, second) && hour == 9, "the time of day in the zone of the server");
    Check(GetTimestampWeekday(TEST_TIME) == 6, "the 2nd of June 2018 was a Saturday");
    Check(GetTimestampWeekday(TEST_TIME, "Nowhere/City") == -1, "a weekday in a zone that does not exist");

    Check(MakeTimestamp(2018, 6, 2, 8, 47, 12, "UTC") == TEST_TIME, "a date in UTC");
    Check(MakeTimestamp(2018, 6, 2, 9, 47, 12) == TEST_TIME, "a date in the zone of the server");
    Check(MakeTimestamp(2018, 1, 32, .zone = "UTC") == MakeTimestamp(2018, 2, 1, .zone = "UTC"), "the 32nd of January is the 1st of February");
    Check(MakeTimestamp(2018, 13, 1, .zone = "UTC") == MakeTimestamp(2019, 1, 1, .zone = "UTC"), "month 13 is January of the next year");
    Check(MakeTimestamp(2100, 1, 1) == INVALID_TIMESTAMP, "a date past 2038 does not fit");
    Check(MakeTimestamp(2018, 1, 1, .zone = "Nowhere/City") == INVALID_TIMESTAMP, "a date in a zone that does not exist");

    new const
        Timestamp:january = MakeTimestamp(2026, 1, 31, 10)
    ;

    Check(AddTimestampMonths(january, 1) == MakeTimestamp(2026, 2, 28, 10), "January 31st and a month is February 28th");
    Check(AddTimestampMonths(january, 13) == MakeTimestamp(2027, 2, 28, 10), "and thirteen months is February 28th of the next year");
    Check(AddTimestampMonths(MakeTimestamp(2026, 3, 31, 10), -1) == MakeTimestamp(2026, 2, 28, 10), "March 31st less a month is February 28th");
    Check(MakeTimestamp(2024, 1, 31, 10) + Months:1 == MakeTimestamp(2024, 2, 29, 10), "a month after January 31st of a leap year is February 29th");
    Check(MakeTimestamp(2024, 2, 29, 12) + Years:1 == MakeTimestamp(2025, 2, 28, 12), "a year after February 29th is February 28th");
    Check(january + Weeks:2 - january == Seconds:1209600, "two weeks are 1209600 seconds");
    Check(january - Days:1 == MakeTimestamp(2026, 1, 30, 10), "a day back");
    Check(DayInHours == Hours:24 && WeekInHours == Hours:168, "a day has 24 hours");

    new const
        Seconds:drift = Now() - Timestamp:gettime()
    ;

    format(line, sizeof (line), "Now is the time of the server (%i)", _:drift);
    Check(Seconds:-1 <= drift <= Seconds:1, line);
}

stock TestZone() {
    new
        text[48]
    ;

    Check(GetChronoTimeZone(text) && strequal(text, "Europe/London"), "the zone comes from config.json");
    Check(!SetChronoTimeZone("Nowhere/City") && GetChronoTimeZone(text) && strequal(text, "Europe/London"), "a zone that does not exist changes nothing");
    Check(SetChronoTimeZone("UTC"), "the zone of the server changes");

    TimeFormat(TEST_TIME, "%H", text);
    CheckText(text, "08", "times then follow the new zone");
    SetChronoTimeZone("Europe/London");
}

public OnGameModeInit() {
    TestFormat();
    TestParse();
    TestDurations();
    TestCalendar();
    TestZone();
    printf("[chronotest] DONE %d passed, %d failed", gPassed, gFailed);
    SendRconCommand("exit");

    return 1;
}
