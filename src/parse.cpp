#include "chrono.hpp"

#include <algorithm>
#include <cctype>
#include <initializer_list>
#include <iterator>
#include <sstream>

namespace chrono {

namespace {

/**
 * What `readFields` found in a text.
 */
struct Fields {
	int64_t year = 0;
	int64_t month = 0;
	int64_t day = 0;
	int64_t hour = 0;
	int64_t minute = 0;
	int64_t second = 0;
	bool dated = false;
	bool monthly = false;
	bool daily = false;
	bool twelve = false;
	int meridiem = -1;
	std::optional<seconds> offset;
};

enum class Reading {
	Read,
	Failed,
	Unknown
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
 * Reads a text with the specifiers `TimeParse` sees most, or answers `Unknown`
 * at any other for `std::chrono::parse` to read the text instead. Spaces in the
 * format match any spaces, and text after the format is left alone, as there.
 */
Reading readFields(std::string_view text, size_t& at, std::string_view format, Fields& fields) {
	for (size_t i = 0; i != format.size(); ++i) {
		if (std::isspace(uint8_t(format[i]))) {
			while (at != text.size() && std::isspace(uint8_t(text[at]))) {
				++at;
			}

			continue;
		}

		if (format[i] != '%') {
			if (at == text.size() || text[at] != format[i]) {
				return Reading::Failed;
			}

			++at;

			continue;
		}

		if (++i == format.size()) {
			return Reading::Unknown;
		}

		bool colon = (format[i] == 'E' || format[i] == 'O') && i + 1 != format.size() && format[i + 1] == 'z';

		if (colon) {
			++i;
		}

		int64_t value = 0;
		Reading part = Reading::Read;

		switch (format[i]) {
			case 'Y':
				fields.dated = readNumber(text, at, 4, fields.year);
				part = fields.dated ? Reading::Read : Reading::Failed;
				break;
			case 'y':
				fields.dated = readNumber(text, at, 2, value);
				fields.year = value < 69 ? 2000 + value : 1900 + value;
				part = fields.dated ? Reading::Read : Reading::Failed;
				break;
			case 'm':
				fields.monthly = readNumber(text, at, 2, fields.month);
				part = fields.monthly ? Reading::Read : Reading::Failed;
				break;
			case 'b':
			case 'B':
			case 'h':
				fields.monthly = readName(text, at, MonthNames, 12, value);
				fields.month = value + 1;
				part = fields.monthly ? Reading::Read : Reading::Failed;
				break;
			case 'e':
				if (at != text.size() && text[at] == ' ') {
					++at;
				}

				[[fallthrough]];
			case 'd':
				fields.daily = readNumber(text, at, 2, fields.day);
				part = fields.daily ? Reading::Read : Reading::Failed;
				break;
			case 'a':
			case 'A':
				part = readName(text, at, WeekdayNames, 7, value) ? Reading::Read : Reading::Failed;
				break;
			case 'H':
				part = readNumber(text, at, 2, fields.hour) ? Reading::Read : Reading::Failed;
				break;
			case 'I':
				fields.twelve = true;
				part = readNumber(text, at, 2, fields.hour) ? Reading::Read : Reading::Failed;
				break;
			case 'M':
				part = readNumber(text, at, 2, fields.minute) ? Reading::Read : Reading::Failed;
				break;
			case 'S':
				part = readNumber(text, at, 2, fields.second) ? Reading::Read : Reading::Failed;
				break;
			case 'p':
				if (text.size() - at >= 2 && std::tolower(uint8_t(text[at + 1])) == 'm' && (std::tolower(uint8_t(text[at])) == 'a' || std::tolower(uint8_t(text[at])) == 'p')) {
					fields.meridiem = std::tolower(uint8_t(text[at])) == 'p';
					at += 2;
				} else {
					part = Reading::Failed;
				}

				break;
			case 'z': {
				int64_t hours = 0;
				int64_t mins = 0;

				if (at == text.size() || (text[at] != '+' && text[at] != '-')) {
					part = Reading::Failed;
					break;
				}

				bool negative = text[at++] == '-';

				if (!readNumber(text, at, 2, hours)) {
					part = Reading::Failed;
					break;
				}

				if (colon && at != text.size() && text[at] == ':') {
					++at;

					if (!readNumber(text, at, 2, mins)) {
						part = Reading::Failed;
						break;
					}
				} else if (!colon) {
					readNumber(text, at, 2, mins);
				}

				fields.offset = seconds((negative ? -1 : 1) * (hours * 3600 + mins * 60));
				break;
			}
			case 'F':
				part = readFields(text, at, "%Y-%m-%d", fields);
				break;
			case 'T':
				part = readFields(text, at, "%H:%M:%S", fields);
				break;
			case 'R':
				part = readFields(text, at, "%H:%M", fields);
				break;
			case 'D':
				part = readFields(text, at, "%m/%d/%y", fields);
				break;
			case 'n':
				if (at != text.size() && std::isspace(uint8_t(text[at]))) {
					++at;
				} else {
					part = Reading::Failed;
				}

				break;
			case 't':
				if (at != text.size() && std::isspace(uint8_t(text[at]))) {
					++at;
				}

				break;
			case '%':
				if (at != text.size() && text[at] == '%') {
					++at;
				} else {
					part = Reading::Failed;
				}

				break;
			default:
				return Reading::Unknown;
		}

		if (part != Reading::Read) {
			return part;
		}
	}

	return Reading::Read;
}

/**
 * The time a text read by `readFields` names in a zone, or nothing for a date
 * left incomplete or out of the calendar.
 */
std::optional<sys_seconds> timeOf(const Fields& fields, const time_zone* zone) {
	if (!fields.dated || !fields.monthly || !fields.daily) {
		return std::nullopt;
	}

	int64_t hour = fields.twelve ? fields.hour % 12 + 12 * fields.meridiem : fields.hour;
	std::chrono::year_month_day date(std::chrono::year(int(fields.year)), std::chrono::month(unsigned(fields.month)), std::chrono::day(unsigned(fields.day)));

	if (!date.ok() || (fields.twelve && (fields.hour < 1 || fields.hour > 12)) || hour > 23 || fields.minute > 59 || fields.second > 59) {
		return std::nullopt;
	}

	local_seconds local = local_days(date) + seconds(hour * 3600 + fields.minute * 60 + fields.second);

	return fields.offset ? sys_seconds(local.time_since_epoch()) - *fields.offset : toSys(zone, local);
}

}

std::optional<sys_seconds> parseTime(std::string_view text, std::string_view format, const time_zone* zone) {
	Fields fields;
	size_t at = 0;
	Reading reading = readFields(text, at, format, fields);

	if (reading == Reading::Read && fields.twelve != (fields.meridiem != -1)) {
		reading = Reading::Unknown;
	}

	if (reading == Reading::Read) {
		return timeOf(fields, zone);
	}

	if (reading == Reading::Failed) {
		return std::nullopt;
	}

	std::istringstream input{ std::string(text) };
	local_seconds local{};
	std::chrono::minutes offset = std::chrono::minutes::min();
	input >> std::chrono::parse(std::string(format), local, offset);

	if (input.fail()) {
		return std::nullopt;
	}

	return offset != std::chrono::minutes::min() ? sys_seconds(local.time_since_epoch()) - offset : toSys(zone, local);
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
