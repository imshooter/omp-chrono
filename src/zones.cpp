#include "chrono.hpp"

#include <array>
#include <exception>

namespace chrono {

namespace {

/**
 * A stretch of time a zone keeps one offset. The zones of the machine are slow
 * to ask, so the last few stretches are kept.
 */
struct Period {
	const time_zone* zone = nullptr;
	sys_seconds begin{};
	sys_seconds end{};
	seconds offset{};
	std::string abbreviation;
};

struct Zones {
	const time_zone* server = nullptr;
	std::array<Period, 4> periods{};
	size_t next = 0;
	std::string lastName;
	const time_zone* last = nullptr;
};

Zones& zones() {
	static Zones instance;
	return instance;
}

std::optional<const time_zone*> locateZone(std::string_view name) {
	try {
		return std::chrono::locate_zone(name);
	} catch (const std::exception&) {
		return std::nullopt;
	}
}

const Period& periodOf(const time_zone* zone, sys_seconds time) {
	Zones& z = zones();

	for (const Period& period : z.periods) {
		if (period.zone == zone && period.begin <= time && time < period.end) {
			return period;
		}
	}

	std::chrono::sys_info info = zone->get_info(time);
	Period& period = z.periods[z.next];
	z.next = (z.next + 1) % z.periods.size();
	period.zone = zone;
	period.begin = info.begin;
	period.end = info.end;
	period.offset = info.offset;
	period.abbreviation = info.abbrev;

	return period;
}

}

void load(ICore& core, StringView name) {
	Zones& z = zones();
	std::string wanted(name.data(), name.size());

	try {
		static_cast<void>(std::chrono::get_tzdb());
	} catch (const std::exception& error) {
		core.logLn(LogLevel::Error, "Chrono " COMPONENT_VERSION " cannot read the time zones of this machine (%s), so it keeps time in UTC.", error.what());
		z.server = nullptr;

		return;
	}

	std::optional<const time_zone*> zone = wanted.empty() ? machineZone() : locateZone(wanted);

	if (!zone && wanted.empty()) {
		core.logLn(LogLevel::Warning, "Chrono " COMPONENT_VERSION " cannot tell the time zone of this machine, so it keeps time in UTC.");
		zone = locateZone("UTC");
	} else if (!zone) {
		core.logLn(LogLevel::Error, "Chrono " COMPONENT_VERSION " knows no time zone \"%s\", so it keeps time in UTC.", wanted.c_str());
		zone = locateZone("UTC");
	}

	z.server = zone ? *zone : nullptr;
	core.printLn("Chrono " COMPONENT_VERSION " keeps time in %s.", z.server != nullptr ? std::string(z.server->name()).c_str() : "UTC");
}

const time_zone* serverZone() {
	return zones().server;
}

void setServerZone(const time_zone* zone) {
	zones().server = zone;
}

std::optional<const time_zone*> machineZone() {
	try {
		return std::chrono::current_zone();
	} catch (const std::exception&) {
		return std::nullopt;
	}
}

std::optional<const time_zone*> findZone(std::string_view name) {
	Zones& z = zones();

	if (name.empty()) {
		return z.server;
	}

	if (z.last != nullptr && name == z.lastName) {
		return z.last;
	}

	std::optional<const time_zone*> zone = locateZone(name);

	if (zone) {
		z.lastName = name;
		z.last = *zone;
	}

	return zone;
}

Offset offsetOf(const time_zone* zone, sys_seconds time) {
	if (zone == nullptr) {
		return { seconds(0), "UTC" };
	}

	const Period& period = periodOf(zone, time);

	return { period.offset, period.abbreviation };
}

local_seconds toLocal(const time_zone* zone, sys_seconds time) {
	return local_seconds((time + offsetOf(zone, time).offset).time_since_epoch());
}

sys_seconds toSys(const time_zone* zone, local_seconds time) {
	if (zone == nullptr) {
		return sys_seconds(time.time_since_epoch());
	}

	for (const Period& period : zones().periods) {
		sys_seconds guess = sys_seconds(time.time_since_epoch()) - period.offset;

		if (period.zone == zone && period.begin + days(1) <= guess && guess + days(1) <= period.end) {
			return guess;
		}
	}

	sys_seconds result = zone->to_sys(time, std::chrono::choose::earliest);
	periodOf(zone, result);

	return result;
}

}
