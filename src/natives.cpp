#include "chrono.hpp"

#include <algorithm>
#include <ctime>
#include <iterator>

namespace chrono {

namespace {

using std::chrono::year_month;
using std::chrono::year_month_day;

struct Scripts {
	IPawnComponent* pawn = nullptr;

	/**
	 * Text read from and written to scripts, kept for its room.
	 */
	std::string input;
	std::string format;
	std::string output;
};

Scripts& scripts() {
	static Scripts instance;
	return instance;
}

IPawnScript* scriptOf(AMX* amx) {
	Scripts& s = scripts();

	return s.pawn != nullptr ? s.pawn->getScript(amx) : nullptr;
}

int arguments(const cell* params) {
	return int(params[0] / cell(sizeof(cell)));
}

/**
 * Reads a string of a script into `out`, which keeps its room between calls.
 */
std::string& readString(IPawnScript& script, cell address, std::string& out) {
	cell* text = nullptr;
	int length = 0;
	out.clear();

	if (script.GetAddr(address, &text) != AMX_ERR_NONE || script.StrLen(text, &length) != AMX_ERR_NONE) {
		return out;
	}

	out.resize(size_t(length) + 1);
	script.GetString(out.data(), text, false, out.size());
	out.resize(size_t(length));

	return out;
}

bool writeString(IPawnScript& script, cell address, cell size, const std::string& text) {
	cell* out = nullptr;

	if (size <= 0 || script.GetAddr(address, &out) != AMX_ERR_NONE) {
		return false;
	}

	return script.SetString(out, StringView(text.data(), text.size()), false, false, size_t(size)) == AMX_ERR_NONE;
}

bool writeCell(IPawnScript& script, cell address, cell value) {
	cell* out = nullptr;

	if (script.GetAddr(address, &out) != AMX_ERR_NONE) {
		return false;
	}

	*out = value;

	return true;
}

/**
 * The zone a script names at `index`, or the zone of the server when it names
 * none or leaves it out, as scripts built for pawn-chrono do.
 */
std::optional<const time_zone*> zoneOf(IPawnScript& script, const cell* params, int index) {
	if (arguments(params) < index) {
		return serverZone();
	}

	return findZone(readString(script, params[index], scripts().input));
}

cell timestampOf(sys_seconds time) {
	int64_t count = time.time_since_epoch().count();

	if (count <= std::numeric_limits<cell>::min() || count > std::numeric_limits<cell>::max()) {
		return InvalidTimestamp;
	}

	return cell(count);
}

/**
 * The month that many months after January of year 0, when the calendar has it.
 */
std::optional<year_month> monthOf(int64_t months) {
	int64_t year = months >= 0 ? months / 12 : (months - 11) / 12;

	if (year < int64_t(int(std::chrono::year::min())) || year > int64_t(int(std::chrono::year::max()))) {
		return std::nullopt;
	}

	return std::chrono::year(int(year)) / std::chrono::month(unsigned(months - year * 12 + 1));
}

cell AMX_NATIVE_CALL Now(AMX* amx, const cell* params) {
	return timestampOf(sys_seconds(seconds(std::time(nullptr))));
}

cell AMX_NATIVE_CALL TimeFormat(AMX* amx, const cell* params) {
	Scripts& s = scripts();
	IPawnScript* script = scriptOf(amx);

	if (script == nullptr || arguments(params) < 4) {
		return 0;
	}

	std::optional<const time_zone*> zone = zoneOf(*script, params, 5);

	if (!zone || !formatTime(*zone, sys_seconds(seconds(params[1])), readString(*script, params[2], s.format), s.output)) {
		return 0;
	}

	return writeString(*script, params[3], params[4], s.output);
}

cell AMX_NATIVE_CALL TimeParse(AMX* amx, const cell* params) {
	Scripts& s = scripts();
	IPawnScript* script = scriptOf(amx);

	if (script == nullptr || arguments(params) < 3) {
		return 0;
	}

	std::optional<const time_zone*> zone = zoneOf(*script, params, 4);

	if (!zone) {
		return 0;
	}

	std::optional<sys_seconds> time = parseTime(readString(*script, params[1], s.input), readString(*script, params[2], s.format), *zone);
	cell timestamp = time ? timestampOf(*time) : InvalidTimestamp;

	return timestamp != InvalidTimestamp && writeCell(*script, params[3], timestamp);
}

cell AMX_NATIVE_CALL DurationParse(AMX* amx, const cell* params) {
	Scripts& s = scripts();
	IPawnScript* script = scriptOf(amx);
	int64_t total = 0;

	if (script == nullptr || arguments(params) < 2 || !parseDuration(readString(*script, params[1], s.input), total) || total > std::numeric_limits<cell>::max()) {
		return 0;
	}

	return writeCell(*script, params[2], cell(total));
}

cell AMX_NATIVE_CALL DurationParseSeconds(AMX* amx, const cell* params) {
	Scripts& s = scripts();
	IPawnScript* script = scriptOf(amx);
	int64_t total = 0;

	if (script == nullptr || arguments(params) < 2 || !parseDuration(readString(*script, params[1], s.input), total) || total / 1000 > std::numeric_limits<cell>::max()) {
		return 0;
	}

	return writeCell(*script, params[2], cell(total / 1000));
}

cell AMX_NATIVE_CALL DurationFormat(AMX* amx, const cell* params) {
	Scripts& s = scripts();
	IPawnScript* script = scriptOf(amx);

	if (script == nullptr || arguments(params) < 3) {
		return 0;
	}

	std::string_view format = arguments(params) >= 4 ? std::string_view(readString(*script, params[4], s.format)) : std::string_view();

	if (!formatDuration(params[1], format, s.output)) {
		return 0;
	}

	return writeString(*script, params[2], params[3], s.output);
}

cell AMX_NATIVE_CALL GetTimestampDate(AMX* amx, const cell* params) {
	IPawnScript* script = scriptOf(amx);

	if (script == nullptr || arguments(params) < 4) {
		return 0;
	}

	std::optional<const time_zone*> zone = zoneOf(*script, params, 5);

	if (!zone) {
		return 0;
	}

	year_month_day date(std::chrono::floor<days>(toLocal(*zone, sys_seconds(seconds(params[1])))));
	bool year = writeCell(*script, params[2], cell(int(date.year())));
	bool month = writeCell(*script, params[3], cell(unsigned(date.month())));
	bool day = writeCell(*script, params[4], cell(unsigned(date.day())));

	return year && month && day;
}

cell AMX_NATIVE_CALL GetTimestampTime(AMX* amx, const cell* params) {
	IPawnScript* script = scriptOf(amx);

	if (script == nullptr || arguments(params) < 4) {
		return 0;
	}

	std::optional<const time_zone*> zone = zoneOf(*script, params, 5);

	if (!zone) {
		return 0;
	}

	local_seconds local = toLocal(*zone, sys_seconds(seconds(params[1])));
	std::chrono::hh_mm_ss<seconds> time(local - std::chrono::floor<days>(local));
	bool hour = writeCell(*script, params[2], cell(time.hours().count()));
	bool minute = writeCell(*script, params[3], cell(time.minutes().count()));
	bool second = writeCell(*script, params[4], cell(time.seconds().count()));

	return hour && minute && second;
}

cell AMX_NATIVE_CALL GetTimestampWeekday(AMX* amx, const cell* params) {
	IPawnScript* script = scriptOf(amx);

	if (script == nullptr || arguments(params) < 1) {
		return -1;
	}

	std::optional<const time_zone*> zone = zoneOf(*script, params, 2);

	if (!zone) {
		return -1;
	}

	return cell(std::chrono::weekday(std::chrono::floor<days>(toLocal(*zone, sys_seconds(seconds(params[1]))))).c_encoding());
}

cell AMX_NATIVE_CALL MakeTimestamp(AMX* amx, const cell* params) {
	IPawnScript* script = scriptOf(amx);

	if (script == nullptr || arguments(params) < 3) {
		return InvalidTimestamp;
	}

	std::optional<const time_zone*> zone = zoneOf(*script, params, 7);
	std::optional<year_month> month = monthOf(int64_t(params[1]) * 12 + int64_t(params[2]) - 1);

	if (!zone || !month) {
		return InvalidTimestamp;
	}

	auto optional = [&](int index) {
		return arguments(params) >= index ? int64_t(params[index]) : int64_t(0);
	};

	seconds offset((int64_t(params[3]) - 1) * 86400 + optional(4) * 3600 + optional(5) * 60 + optional(6));

	return timestampOf(toSys(*zone, local_days(*month / 1) + offset));
}

cell AMX_NATIVE_CALL AddTimestampMonths(AMX* amx, const cell* params) {
	IPawnScript* script = scriptOf(amx);

	if (script == nullptr || arguments(params) < 2) {
		return InvalidTimestamp;
	}

	std::optional<const time_zone*> zone = zoneOf(*script, params, 3);

	if (!zone) {
		return InvalidTimestamp;
	}

	local_seconds local = toLocal(*zone, sys_seconds(seconds(params[1])));
	local_days day = std::chrono::floor<days>(local);
	year_month_day date(day);
	std::optional<year_month> month = monthOf(int64_t(int(date.year())) * 12 + int64_t(unsigned(date.month())) - 1 + int64_t(params[2]));

	if (!month) {
		return InvalidTimestamp;
	}

	local_days target(*month / std::min(date.day(), (*month / std::chrono::last).day()));

	return timestampOf(toSys(*zone, target + (local - day)));
}

cell AMX_NATIVE_CALL SetChronoTimeZone(AMX* amx, const cell* params) {
	IPawnScript* script = scriptOf(amx);

	if (script == nullptr || arguments(params) < 1) {
		return 0;
	}

	std::string& name = readString(*script, params[1], scripts().input);
	std::optional<const time_zone*> zone = name.empty() ? machineZone() : findZone(name);

	if (!zone) {
		return 0;
	}

	setServerZone(*zone);

	return 1;
}

cell AMX_NATIVE_CALL GetChronoTimeZone(AMX* amx, const cell* params) {
	Scripts& s = scripts();
	IPawnScript* script = scriptOf(amx);
	const time_zone* zone = serverZone();

	if (script == nullptr || arguments(params) < 2) {
		return 0;
	}

	s.output.assign(zone != nullptr ? zone->name() : std::string_view("UTC"));

	return writeString(*script, params[1], params[2], s.output);
}

const AMX_NATIVE_INFO natives[] = {
	{ "Now", Now },
	{ "TimeFormat", TimeFormat },
	{ "TimeParse", TimeParse },
	{ "DurationParse", DurationParse },
	{ "DurationParseSeconds", DurationParseSeconds },
	{ "DurationFormat", DurationFormat },
	{ "GetTimestampDate", GetTimestampDate },
	{ "GetTimestampTime", GetTimestampTime },
	{ "GetTimestampWeekday", GetTimestampWeekday },
	{ "MakeTimestamp", MakeTimestamp },
	{ "AddTimestampMonths", AddTimestampMonths },
	{ "SetChronoTimeZone", SetChronoTimeZone },
	{ "GetChronoTimeZone", GetChronoTimeZone }
};

}

void attach(IPawnComponent* pawn) {
	scripts().pawn = pawn;
}

void registerNatives(IPawnScript& script) {
	script.Register(natives, int(std::size(natives)));
}

}
