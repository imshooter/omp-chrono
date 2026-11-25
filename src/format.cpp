#include "chrono.hpp"

#include <algorithm>
#include <cctype>
#include <iterator>

namespace chrono {

const char* const WeekdayNames[7] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
const char* const MonthNames[12] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };

namespace {

/**
 * Appends a number of at least `width` digits, filled with `fill`.
 */
void appendNumber(std::string& out, int64_t value, int width, char fill = '0') {
	char digits[24];
	uint64_t rest = value < 0 ? uint64_t(-value) : uint64_t(value);
	int count = 0;

	do {
		digits[count++] = char('0' + rest % 10);
		rest /= 10;
	} while (rest != 0);

	if (value < 0) {
		out += '-';
	}

	out.append(size_t(std::max(width - count, 0)), fill);

	while (count != 0) {
		out += digits[--count];
	}
}

}

bool formatTime(const time_zone* zone, sys_seconds time, std::string_view format, std::string& out) {
	Offset at = offsetOf(zone, time);
	local_seconds local((time + at.offset).time_since_epoch());
	local_days day = std::chrono::floor<days>(local);
	std::chrono::year_month_day date(day);
	std::chrono::hh_mm_ss<seconds> clock(local - day);
	int year = int(date.year());
	int month = int(unsigned(date.month()));
	int monthDay = int(unsigned(date.day()));
	int weekday = int(std::chrono::weekday(day).c_encoding());
	int mondayDay = (weekday + 6) % 7;
	int yearDay = int((day - local_days(date.year() / std::chrono::January / 1)).count());
	int hour = int(clock.hours().count());
	int minute = int(clock.minutes().count());
	int second = int(clock.seconds().count());
	out.clear();

	for (size_t i = 0; i != format.size(); ++i) {
		if (format[i] != '%') {
			out += format[i];

			continue;
		}

		if (++i == format.size()) {
			return false;
		}

		bool modified = format[i] == 'E' || format[i] == 'O';

		if (modified && ++i == format.size()) {
			return false;
		}

		switch (format[i]) {
			case 'a':
				out.append(WeekdayNames[weekday], 3);
				break;
			case 'A':
				out += WeekdayNames[weekday];
				break;
			case 'b':
			case 'h':
				out.append(MonthNames[month - 1], 3);
				break;
			case 'B':
				out += MonthNames[month - 1];
				break;
			case 'c':
				out.append(WeekdayNames[weekday], 3);
				out += ' ';
				out.append(MonthNames[month - 1], 3);
				out += ' ';
				appendNumber(out, monthDay, 2, ' ');
				out += ' ';
				appendNumber(out, hour, 2);
				out += ':';
				appendNumber(out, minute, 2);
				out += ':';
				appendNumber(out, second, 2);
				out += ' ';
				appendNumber(out, year, 4);
				break;
			case 'C':
				appendNumber(out, year / 100, 2);
				break;
			case 'd':
				appendNumber(out, monthDay, 2);
				break;
			case 'D':
			case 'x':
				appendNumber(out, month, 2);
				out += '/';
				appendNumber(out, monthDay, 2);
				out += '/';
				appendNumber(out, year % 100, 2);
				break;
			case 'e':
				appendNumber(out, monthDay, 2, ' ');
				break;
			case 'F':
				appendNumber(out, year, 4);
				out += '-';
				appendNumber(out, month, 2);
				out += '-';
				appendNumber(out, monthDay, 2);
				break;
			case 'g':
			case 'G':
			case 'V': {
				local_days thursday = day + days(3 - mondayDay);
				std::chrono::year_month_day week(thursday);

				if (format[i] == 'V') {
					appendNumber(out, (thursday - local_days(week.year() / std::chrono::January / 1)).count() / 7 + 1, 2);
				} else {
					appendNumber(out, format[i] == 'G' ? int(week.year()) : int(week.year()) % 100, format[i] == 'G' ? 4 : 2);
				}

				break;
			}
			case 'H':
				appendNumber(out, hour, 2);
				break;
			case 'I':
				appendNumber(out, hour % 12 == 0 ? 12 : hour % 12, 2);
				break;
			case 'j':
				appendNumber(out, yearDay + 1, 3);
				break;
			case 'm':
				appendNumber(out, month, 2);
				break;
			case 'M':
				appendNumber(out, minute, 2);
				break;
			case 'n':
				out += '\n';
				break;
			case 'p':
				out += hour < 12 ? "AM" : "PM";
				break;
			case 'r':
				appendNumber(out, hour % 12 == 0 ? 12 : hour % 12, 2);
				out += ':';
				appendNumber(out, minute, 2);
				out += ':';
				appendNumber(out, second, 2);
				out += hour < 12 ? " AM" : " PM";
				break;
			case 'R':
				appendNumber(out, hour, 2);
				out += ':';
				appendNumber(out, minute, 2);
				break;
			case 'S':
				appendNumber(out, second, 2);
				break;
			case 't':
				out += '\t';
				break;
			case 'T':
			case 'X':
				appendNumber(out, hour, 2);
				out += ':';
				appendNumber(out, minute, 2);
				out += ':';
				appendNumber(out, second, 2);
				break;
			case 'u':
				appendNumber(out, mondayDay + 1, 1);
				break;
			case 'U':
				appendNumber(out, (yearDay + 7 - weekday) / 7, 2);
				break;
			case 'w':
				appendNumber(out, weekday, 1);
				break;
			case 'W':
				appendNumber(out, (yearDay + 7 - mondayDay) / 7, 2);
				break;
			case 'y':
				appendNumber(out, year % 100, 2);
				break;
			case 'Y':
				appendNumber(out, year, 4);
				break;
			case 'z': {
				int64_t total = at.offset.count();
				out += total < 0 ? '-' : '+';
				total = total < 0 ? -total : total;
				appendNumber(out, total / 3600, 2);

				if (modified) {
					out += ':';
				}

				appendNumber(out, total / 60 % 60, 2);
				break;
			}
			case 'Z':
				out += at.abbreviation;
				break;
			case '%':
				out += '%';
				break;
			default:
				return false;
		}
	}

	return true;
}

bool formatDuration(int64_t total, std::string_view format, std::string& out) {
	struct Unit {
		char specifier;
		uint64_t seconds;
		const char* name;
	};

	static const Unit units[] = {
		{ 'y', uint64_t(YearInSeconds), "year" },
		{ 'd', 86400, "day" },
		{ 'h', 3600, "hour" },
		{ 'm', 60, "minute" },
		{ 's', 1, "second" }
	};

	constexpr size_t count = std::size(units);
	uint64_t value = total < 0 ? uint64_t(-total) : uint64_t(total);
	out.assign(total < 0 ? "-" : "");

	if (format.empty()) {
		size_t start = out.size();

		for (const Unit& unit : units) {
			uint64_t amount = value / unit.seconds;
			value %= unit.seconds;

			if (amount == 0 && (unit.seconds != 1 || out.size() != start)) {
				continue;
			}

			if (out.size() != start) {
				out += ", ";
			}

			appendNumber(out, int64_t(amount), 1);
			out += ' ';
			out += unit.name;

			if (amount != 1) {
				out += 's';
			}
		}

		return true;
	}

	auto unitOf = [&](char specifier) {
		char lower = char(std::tolower(uint8_t(specifier)));

		return size_t(std::find_if(std::begin(units), std::end(units), [&](const Unit& unit) {
			return unit.specifier == lower;
		}) - std::begin(units));
	};

	bool used[count] = {};

	for (size_t i = 0; i != format.size(); ++i) {
		if (format[i] != '%') {
			continue;
		}

		if (++i == format.size()) {
			return false;
		}

		if (format[i] == '%') {
			continue;
		}

		size_t unit = unitOf(format[i]);

		if (unit == count) {
			return false;
		}

		used[unit] = true;
	}

	uint64_t parts[count] = {};

	for (size_t unit = 0; unit != count; ++unit) {
		if (used[unit]) {
			parts[unit] = value / units[unit].seconds;
			value %= units[unit].seconds;
		}
	}

	for (size_t i = 0; i != format.size(); ++i) {
		if (format[i] != '%') {
			out += format[i];

			continue;
		}

		char specifier = format[++i];

		if (specifier == '%') {
			out += '%';

			continue;
		}

		appendNumber(out, int64_t(parts[unitOf(specifier)]), std::isupper(uint8_t(specifier)) ? 2 : 1);
	}

	return true;
}

}
