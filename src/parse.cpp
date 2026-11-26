#include "chrono.hpp"

#include <algorithm>
#include <cctype>
#include <initializer_list>
#include <iterator>

namespace chrono {

namespace {

/**
 * What `readFields` found in a text. A weekday counts from `0` on Sunday.
 */
struct Fields {
	std::optional<int64_t> year;
	std::optional<int64_t> century;
	std::optional<int64_t> shortYear;
	std::optional<int64_t> month;
	std::optional<int64_t> day;
	std::optional<int64_t> yearDay;
	std::optional<int64_t> weekday;
	std::optional<int64_t> sundayWeek;
	std::optional<int64_t> mondayWeek;
	std::optional<int64_t> isoYear;
	std::optional<int64_t> isoWeek;
	int64_t hour = 0;
	int64_t minute = 0;
	int64_t second = 0;
	bool twelve = false;
	int meridiem = -1;
	std::optional<seconds> offset;
};

/**
 * Reads up to `width` digits, at least one.
 */
bool readNumber(std::string_view text, size_t& at, int width, int64_t& out) {
	size_t start = at;
	out = 0;

	while (at != text.size() && at - start != size_t(width) && std::isdigit(uint8_t(text[at]))) {
		out = out * 10 + (text[at++] - '0');
	}

	return at != start;
}

/**
 * Reads the English name of a day or a month, whole or its first three letters,
 * in any case, and gives its index.
 */
bool readName(std::string_view text, size_t& at, const char* const* names, int count, int64_t& out) {
	for (int index = 0; index != count; ++index) {
		std::string_view name(names[index]);

		for (size_t length : { name.size(), size_t(3) }) {
			if (text.size() - at < length) {
				continue;
			}

			bool same = true;

			for (size_t k = 0; k != length && same; ++k) {
				same = std::tolower(uint8_t(text[at + k])) == std::tolower(uint8_t(name[k]));
			}

			if (same) {
				at += length;
				out = index;

				return true;
			}
		}
	}

	return false;
}

/**
 * Reads a text with the specifiers of `TimeFormat`. A width before a specifier,
 * as `%4Y`, is the most digits it reads, spaces in the format match any spaces,
 * and text after the format is left alone.
 */
bool readFields(std::string_view text, size_t& at, std::string_view format, Fields& fields) {
	for (size_t i = 0; i != format.size(); ++i) {
		if (std::isspace(uint8_t(format[i]))) {
			while (at != text.size() && std::isspace(uint8_t(text[at]))) {
				++at;
			}

			continue;
		}

		if (format[i] != '%') {
			if (at == text.size() || text[at] != format[i]) {
				return false;
			}

			++at;

			continue;
		}

		int width = 0;

		while (++i != format.size() && std::isdigit(uint8_t(format[i]))) {
			width = width * 10 + (format[i] - '0');
		}

		bool modified = i != format.size() && (format[i] == 'E' || format[i] == 'O');

		if (modified) {
			++i;
		}

		if (i == format.size()) {
			return false;
		}

		auto number = [&](int digits, std::optional<int64_t>& out, int64_t low, int64_t high) {
			int64_t value = 0;

			if (!readNumber(text, at, width != 0 ? width : digits, value) || value < low || value > high) {
				return false;
			}

			out = value;

			return true;
		};

		auto clock = [&](int64_t& out, int64_t high) {
			return readNumber(text, at, width != 0 ? width : 2, out) && out <= high;
		};

		std::optional<int64_t> value;
		bool read = true;

		switch (format[i]) {
			case 'Y':
				read = number(4, fields.year, 0, 9999);
				break;
			case 'C':
				read = number(2, fields.century, 0, 99);
				break;
			case 'y':
				read = number(2, fields.shortYear, 0, 99);
				break;
			case 'G':
				read = number(4, fields.isoYear, 0, 9999);
				break;
			case 'g':
				read = number(2, value, 0, 99);
				fields.isoYear = value ? (*value < 69 ? 2000 + *value : 1900 + *value) : fields.isoYear;
				break;
			case 'm':
				read = number(2, fields.month, 1, 12);
				break;
			case 'b':
			case 'B':
			case 'h': {
				int64_t index = 0;
				read = readName(text, at, MonthNames, 12, index);
				fields.month = read ? index + 1 : fields.month;
				break;
			}
			case 'e':
				if (at != text.size() && text[at] == ' ') {
					++at;
				}

				[[fallthrough]];
			case 'd':
				read = number(2, fields.day, 1, 31);
				break;
			case 'j':
				read = number(3, fields.yearDay, 1, 366);
				break;
			case 'a':
			case 'A': {
				int64_t index = 0;
				read = readName(text, at, WeekdayNames, 7, index);
				fields.weekday = read ? index : fields.weekday;
				break;
			}
			case 'u':
				read = number(1, value, 1, 7);
				fields.weekday = value ? *value % 7 : fields.weekday;
				break;
			case 'w':
				read = number(1, fields.weekday, 0, 6);
				break;
			case 'U':
				read = number(2, fields.sundayWeek, 0, 53);
				break;
			case 'W':
				read = number(2, fields.mondayWeek, 0, 53);
				break;
			case 'V':
				read = number(2, fields.isoWeek, 1, 53);
				break;
			case 'H':
				read = clock(fields.hour, 23);
				break;
			case 'I':
				fields.twelve = true;
				read = clock(fields.hour, 12) && fields.hour != 0;
				break;
			case 'M':
				read = clock(fields.minute, 59);
				break;
			case 'S':
				read = clock(fields.second, 59);
				break;
			case 'p':
				read = text.size() - at >= 2 && std::tolower(uint8_t(text[at + 1])) == 'm' && (std::tolower(uint8_t(text[at])) == 'a' || std::tolower(uint8_t(text[at])) == 'p');

				if (read) {
					fields.meridiem = std::tolower(uint8_t(text[at])) == 'p';
					at += 2;
				}

				break;
			case 'z': {
				int64_t hours = 0;
				int64_t mins = 0;
				read = at != text.size() && (text[at] == '+' || text[at] == '-');

				if (!read) {
					break;
				}

				bool negative = text[at++] == '-';
				read = readNumber(text, at, 2, hours);

				if (read && modified && at != text.size() && text[at] == ':') {
					++at;
					read = readNumber(text, at, 2, mins);
				} else if (read && !modified) {
					readNumber(text, at, 2, mins);
				}

				fields.offset = seconds((negative ? -1 : 1) * (hours * 3600 + mins * 60));
				break;
			}
			case 'Z': {
				size_t start = at;

				while (at != text.size() && (std::isalnum(uint8_t(text[at])) || text[at] == '+' || text[at] == '-' || text[at] == '_' || text[at] == '/')) {
					++at;
				}

				read = at != start;
				break;
			}
			case 'c':
				read = readFields(text, at, "%a %b %e %H:%M:%S %Y", fields);
				break;
			case 'D':
			case 'x':
				read = readFields(text, at, "%m/%d/%y", fields);
				break;
			case 'F':
				read = readFields(text, at, "%Y-%m-%d", fields);
				break;
			case 'r':
				read = readFields(text, at, "%I:%M:%S %p", fields);
				break;
			case 'R':
				read = readFields(text, at, "%H:%M", fields);
				break;
			case 'T':
			case 'X':
				read = readFields(text, at, "%H:%M:%S", fields);
				break;
			case 'n':
				read = at != text.size() && std::isspace(uint8_t(text[at]));
				at += read ? 1 : 0;
				break;
			case 't':
				at += at != text.size() && std::isspace(uint8_t(text[at])) ? 1 : 0;
				break;
			case '%':
				read = at != text.size() && text[at] == '%';
				at += read ? 1 : 0;
				break;
			default:
				return false;
		}

		if (!read) {
			return false;
		}
	}

	return true;
}

/**
 * The day a text read by `readFields` names: by its month and day, its day of
 * the year, its ISO week or its week from Sunday or Monday, in that order. A
 * weekday read beside a date must be the weekday of that date.
 */
std::optional<local_days> dateOf(const Fields& fields) {
	using std::chrono::year_month_day;

	std::optional<int64_t> year = fields.year;

	if (!year && fields.century) {
		year = *fields.century * 100 + fields.shortYear.value_or(0);
	} else if (!year && fields.shortYear) {
		year = *fields.shortYear < 69 ? 2000 + *fields.shortYear : 1900 + *fields.shortYear;
	}

	auto first = [](int64_t of, unsigned month, unsigned day) {
		return local_days(year_month_day(std::chrono::year(int(of)), std::chrono::month(month), std::chrono::day(day)));
	};

	auto weekdayOf = [](local_days day) {
		return int64_t(std::chrono::weekday(day).c_encoding());
	};

	std::optional<local_days> date;

	if (year && fields.month && fields.day) {
		year_month_day ymd(std::chrono::year(int(*year)), std::chrono::month(unsigned(*fields.month)), std::chrono::day(unsigned(*fields.day)));

		if (!ymd.ok()) {
			return std::nullopt;
		}

		date = local_days(ymd);
	} else if (year && fields.yearDay) {
		date = first(*year, 1, 1) + days(*fields.yearDay - 1);
	} else if (fields.isoYear && fields.isoWeek && fields.weekday) {
		local_days january = first(*fields.isoYear, 1, 4);
		local_days monday = january - days((weekdayOf(january) + 6) % 7);
		date = monday + days((*fields.isoWeek - 1) * 7 + (*fields.weekday + 6) % 7);

		if (year_month_day(*date + days(3 - (*fields.weekday + 6) % 7)).year() != std::chrono::year(int(*fields.isoYear))) {
			return std::nullopt;
		}
	} else if (year && fields.weekday && (fields.sundayWeek || fields.mondayWeek)) {
		local_days january = first(*year, 1, 1);
		int64_t weekday = weekdayOf(january);

		if (fields.sundayWeek) {
			date = january + days((7 - weekday) % 7 + (*fields.sundayWeek - 1) * 7 + *fields.weekday);
		} else {
			date = january + days((8 - weekday) % 7 + (*fields.mondayWeek - 1) * 7 + (*fields.weekday + 6) % 7);
		}
	} else {
		return std::nullopt;
	}

	if (year && !fields.isoWeek && year_month_day(*date).year() != std::chrono::year(int(*year))) {
		return std::nullopt;
	}

	if (fields.weekday && weekdayOf(*date) != *fields.weekday) {
		return std::nullopt;
	}

	return date;
}

}

std::optional<sys_seconds> parseTime(std::string_view text, std::string_view format, const time_zone* zone) {
	Fields fields;
	size_t at = 0;

	if (!readFields(text, at, format, fields)) {
		return std::nullopt;
	}

	std::optional<local_days> date = dateOf(fields);

	if (!date || fields.twelve != (fields.meridiem != -1)) {
		return std::nullopt;
	}

	int64_t hour = fields.twelve ? fields.hour % 12 + 12 * fields.meridiem : fields.hour;
	local_seconds local = *date + seconds(hour * 3600 + fields.minute * 60 + fields.second);

	return fields.offset ? sys_seconds(local.time_since_epoch()) - *fields.offset : toSys(zone, local);
}

bool parseDuration(std::string_view text, int64_t& total) {
	struct Unit {
		const char* name;
		int64_t milliseconds;
	};

	static const Unit units[] = {
		{ "ms", 1 },
		{ "s", 1000 },
		{ "m", 60 * 1000 },
		{ "h", 3600 * 1000 },
		{ "d", 86400 * 1000 },
		{ "w", 7 * 86400 * 1000 },
		{ "y", YearInSeconds * 1000 }
	};

	constexpr int64_t limit = int64_t(1) << 53;
	size_t i = 0;
	bool read = false;
	total = 0;

	auto skipSpaces = [&] {
		while (i != text.size() && std::isspace(uint8_t(text[i]))) {
			++i;
		}
	};

	for (skipSpaces(); i != text.size(); skipSpaces()) {
		if (!std::isdigit(uint8_t(text[i]))) {
			return false;
		}

		int64_t number = 0;

		while (i != text.size() && std::isdigit(uint8_t(text[i]))) {
			number = number * 10 + (text[i++] - '0');

			if (number > limit) {
				return false;
			}
		}

		skipSpaces();
		char unit[3] = {};
		size_t length = 0;

		while (i != text.size() && std::isalpha(uint8_t(text[i]))) {
			if (length == 2) {
				return false;
			}

			unit[length++] = char(std::tolower(uint8_t(text[i++])));
		}

		const Unit* found = std::find_if(std::begin(units), std::end(units), [&](const Unit& candidate) {
			return std::string_view(unit, length) == candidate.name;
		});

		if (found == std::end(units) || number > (limit - total) / found->milliseconds) {
			return false;
		}

		total += number * found->milliseconds;
		read = true;
	}

	return read;
}

}
