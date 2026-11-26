# Chrono

> [!NOTE]
> Chrono is experimental and already runs in production. Its natives may still change from one version to the next. It was made with Claude Opus 5.5, a model by [Anthropic](https://www.anthropic.com).

An open.mp component for dates and times: timestamps and durations with a tag for each unit, the calendar and time zones. It takes the place of the [pawn-chrono](https://github.com/Southclaws/pawn-chrono) plugin with the same names, and every call is faster.

## Features

- **Timestamps** with tags, so `Now() + Days:3` is a `Timestamp` and `Timestamp - Timestamp` gives `Seconds`.
- **Time zones** with daylight saving time. The server keeps one, named in `config.json`, and every function that reads or writes a date takes another, as `"UTC"`.
- **Writing and reading times** with the specifiers of C++, as `%Y-%m-%d %H:%M:%S`, with names always in English.
- **Durations** read from text such as `1h 30m`, and written in words or with a format of their own, as `%H:%M:%S`.
- **The calendar**: the date, time and day of the week of a timestamp, a timestamp from a date, and months and years counted as the calendar does, so January 31st and one month is the last day of February.

## Installation

1. Download the latest release from [Releases](https://github.com/imshooter/chrono-omp/releases), the `windows-x86` or the `linux-x86` zip.
2. Place `Chrono.dll` or `Chrono.so` in the `components` folder of the server.
3. Place `chrono.inc` in the include folder of the compiler.
4. Remove `chrono` from `pawn.legacy_plugins`, as both give the same natives.

No plugin is needed. Tested with open.mp 1.6.7 on Windows, Ubuntu 20.04, Debian 12 and Ubuntu 24.04. The time zones come from Windows 10 version 1903 or later, and on Linux from the `tzdata` of the system. The Linux build needs glibc 2.25 or later.

## Configuration

```json
"chrono": {
    "time_zone": "Europe/London"
}
```

| Key | Description |
| --- | --- |
| `time_zone` | The zone of the server, by its name in the time zone database. Empty, the zone of the machine. |

## A first script

```pawn
#include <open.mp>
#include <chrono>

public OnPlayerConnect(playerid) {
    new const
        Timestamp:expiry = Now() + Days:30
    ;

    new
        date[32],
        left[48]
    ;

    TimeFormat(expiry, "%d %B %Y at %H:%M", date);
    DurationFormat(expiry - Now(), left);
    SendClientMessage(playerid, -1, "Your membership ends on %s, in %s.", date, left);

    return 1;
}
```

## Coming from pawn-chrono

Scripts compile as they are, and scripts built against pawn-chrono run on the component. What changes:

- `TimeParse` and `DurationParse` return `true` when they read the text, where pawn-chrono returned `0`.
- `TimeFormat` writes in the zone of the server, where pawn-chrono wrote UTC. Pass `"UTC"` to keep the old text.
- `DayInHours` is `24`, not `12`, and `DurationFormat` no longer writes before its array for a duration of zero.
- `MonthInMilliseconds`, which did not fit a cell, is gone.

## Performance

Microseconds per call, measured on one computer with pawn-chrono and chrono-omp on the same server and script:

| Call | pawn-chrono | chrono-omp |
| --- | --- | --- |
| `Now` | 0.03 | 0.03 |
| `TimeFormat` | 3.4 | 0.33 to 0.46 |
| `TimeParse` | 2.4 | 0.38 |
| `DurationParse` | 0.66 | 0.32 |
| `DurationFormat` | 0.77 | 0.25 |

## Tests

- [`tests/chronotest.pwn`](tests/chronotest.pwn): writing and reading times, durations, the calendar and time zones, on a server in `Europe/London`. GitHub Actions runs it on open.mp for Linux at every push.

## Building

CMake 3.19 or later fetches the open.mp SDK and the Pawn headers. The server is 32 bit, so the component must be too.

On Windows, with Visual Studio 2022 or its Build Tools:

```
cmake -S . -B build -A Win32
cmake --build build --config Release
```

On Linux, with GCC 13 or later and its 32 bit libraries:

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS=-m32 -DCMAKE_CXX_FLAGS=-m32 -DCMAKE_SHARED_LINKER_FLAGS=-m32
cmake --build build
```

The component is written to `build/Release/Chrono.dll` or `build/Chrono.so`. The Linux release is built on Ubuntu 20.04, as [`.github/workflows/main.yml`](.github/workflows/main.yml) does.

## Thanks to

- [@Southclaws](https://github.com/Southclaws) for [pawn-chrono](https://github.com/Southclaws/pawn-chrono), whose names, tags and constants Chrono keeps.
- [@openmultiplayer](https://github.com/openmultiplayer) for the [SDK](https://github.com/openmultiplayer/open.mp-sdk) and the [Pawn compiler](https://github.com/openmultiplayer/compiler).

## License

Released under the [MIT License](LICENSE). The names, tags and constants of `chrono.inc` come from pawn-chrono, under its own [MIT License](LICENSE-pawn-chrono).
