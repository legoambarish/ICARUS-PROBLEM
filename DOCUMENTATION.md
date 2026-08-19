# DOCUMENTATION.md

## Project Statement
OBC Round; flight-software debugging problem.

## Overview
- Covers both branches: solution and fault-injection.
- Build issue.
- A memory-corruption bug that caused a crash partway through the run.
- A buffer-overflow bug in a sensor-copy routine.
- A measurement/scaling bug that made the reported battery voltage wrong.
- An integer-overflow bug that made a staleness flag get stuck.
- A unbounded input function sitting unused in the code.
- A bug where a checksum was computed on stale data.
- A bug in the ground-station receiver script.
- The fault-injection trigger, prediction and what actually happened.

# Bugs Found (8)

## 1. Build fails

Bug: The Makefile linked against a library path that didn't exist

Location: Ran `make` inside the container as the first step and got a linker error:

```bash
make
```

```
gcc -O1 -fno-stack-protector -D_FORTIFY_SOURCE=0 -std=c11 -Wall -Iinclude -o obc_sim src/drivers/adc_driver.o ... -L./lib/static -lobc_physics -lm
/usr/bin/ld: cannot find -lobc_physics: No such file or directory
collect2: error: ld returned 1 exit status
make: *** [Makefile:13: obc_sim] Error 1
```

Investigated further and found the library actually exists at a different path:

```bash
find /c/Users/User/icarus-workspace -iname "*obc_physics*" -o -iname "*.a"
```

```
/c/Users/User/icarus-workspace/include/obc_physics.h
/c/Users/User/icarus-workspace/lib/libobc_physics.a
/c/Users/User/icarus-workspace/lib/obc_physics_build.c
```

Confirmed this wasn't a local issue by running a completely fresh container from the untouched image:

```bash
docker run --rm tanaychordia/icarus-round2:final bash -c "ls -la /obc/lib; echo ---; ls -la /obc/lib/static 2>&1"
```

```
total 16
drwxrwxr-x 1 root root 4096 Aug 16 18:03 .
drwxr-xr-x 1 root root 4096 Aug 16 17:55 ..
-rw-rw-r-- 1 root root 2142 Aug 16 18:03 libobc_physics.a
-rw-r--r-- 1 root root  239 Aug 16 17:46 obc_physics_build.c
---
ls: cannot access '/obc/lib/static': No such file or directory
```

Reason: Without resolving this issue, no progress could be made as this makes the obc_sim executable.

Fix: The `Makefile`'s `LDFLAGS := -L./lib/static -lobc_physics -lm` needs the library to exist at `lib/static/libobc_physics.a`, so I created that directory and copied the existing library into it.

```cmd
mkdir C:\Users\User\icarus-workspace\lib\static
copy C:\Users\User\icarus-workspace\lib\libobc_physics.a C:\Users\User\icarus-workspace\lib\static\
```

COnfirmation:

```bash
make
./obc_sim
```

```
TICK:1 | ORBIT:1 | TEMP:-14 | VBAT:3.125 | SAFE:0 | THERM_STALE: 0
TICK:2 | ORBIT:1 | TEMP:-14 | VBAT:3.125 | SAFE:0 | THERM_STALE: 0
etc
```

## 2. Ring buffer overflow

Bug: A "circular" buffer that was supposed to wrap around when full never actually wrapped, silently walking off the end and corrupting adjacent memory.

Location: Ran the simulation after fixing the build:

```bash
./obc_sim
```

It ran for 1025 ticks and then crashed:

```
TICK:1024 | ORBIT:7 | TEMP:-13 | VBAT:2.812 | SAFE:0 | THERM_STALE: 0
TICK:1025 | ORBIT:7 | TEMP:-13 | VBAT:2.812 | SAFE:0 | THERM_STALE: 0

[FATAL] Segmentation fault at tick 1026
```

Rebuilt with debug symbols to investigate in gdb:

```bash
make clean
make CFLAGS="-g -O0 -fno-stack-protector -D_FORTIFY_SOURCE=0 -std=c11 -Wall -Iinclude"
gdb ./obc_sim
```

```
run
```

```
Program received signal SIGSEGV, Segmentation fault.
0x00006544ef3b7951 in scheduler_run_tick (state=0x7ffee00f44c0) at src/rtos/scheduler.c:89
89              state->tasks[pick].run_count++;
```

At first this looked like the bug was in the scheduler, but `pick` was a valid index (0). That pointed toward memory corruption happening somewhere else.

Confirmed by inspecting the pointer directly:

```
frame 1
print state.tasks
```

```
$1 = (TaskRecord *) 0xeee25d9400
```

Corrupted as that cannot be an address for a static array. Used a Watchpoint to catch the exact moment:

```
kill
break scheduler_run_tick
run
```

```
frame 1
watch state.tasks
```

```
delete 1
continue
```

The watchpoint fired:

```
Hardware watchpoint 3: state.tasks

Old value = (TaskRecord *) 0x63ad4ee80040 <tasks>
New value = (TaskRecord *) 0xeee25d9400
__memmove_avx_unaligned_erms () at ../sysdeps/x86_64/multiarch/memmove-vec-unaligned-erms.S:399
```

```
bt
```

```
#0  __memmove_avx_unaligned_erms ()
#1  cbuf_insert (state=..., frame=...) at src/drivers/telemetry_ingest.c:8
#2  ring_buffer_commit (...) at src/drivers/telemetry_ingest.c:14
#3  dma_descriptor_stage (...) at src/drivers/telemetry_ingest.c:18
#4  fsw_tm_push (...) at src/drivers/telemetry_ingest.c:22
#5  telemetry_ingest_task (...) at src/flight/tasks.c:20
#6  scheduler_run_tick (...) at src/rtos/scheduler.c:88
#7  main () at src/main.c:54
```

The real issue was in `cbuf_insert` in `telemetry_ingest.c`.

Reason: `telemetry_cursor` walks through a fixed 1024-slot buffer (`QUEUE_SIZE`) with no bounds check, so after ~1024 ticks it walks past the end of the buffer and starts overwriting adjacent fields in the `AppState` struct, eventually landing on `tasks`, which is why the crash surfaced somewhere completely unrelated to the actual bug.

Fix: Code fix, a simple bounds check after incrementing.

Before (`src/drivers/telemetry_ingest.c`):
```c
static void cbuf_insert(AppState *state, const TelemetryFrame *frame) {
    WRITE_SLOT(state->telemetry_cursor, frame);
    state->telemetry_cursor++;
    state->telemetry_frames++;
}
```

After:
```c
static void cbuf_insert(AppState *state, const TelemetryFrame *frame) {
    WRITE_SLOT(state->telemetry_cursor, frame);
    state->telemetry_cursor++;
    if (state->telemetry_cursor >= &state->shared.queue[QUEUE_SIZE]) {
        state->telemetry_cursor = &state->shared.queue[0];
    }
    state->telemetry_frames++;
}
```

Confirmed the full 2250-tick run completes with no crash:

```bash
make clean
make
./obc_sim
```

```
TICK:2249 | ORBIT:15 | TEMP:-16 | VBAT:... | SAFE:... | THERM_STALE: ...
TICK:2250 | ORBIT:15 | TEMP:-15 | VBAT:... | SAFE:... | THERM_STALE: ...
```

## 3. Sensor-copy buffer overflow

Bug: A copy length was calculated from the current temperature with no check against the size of the destination buffer, and a signed/unsigned mismatch in the guard condition made it break for negative temperatures.

Location: After TICK:15, a warning appeared.

```bash
./obc_sim
```

```
TICK:15 | ORBIT:1 | TEMP:-3 | VBAT:3.125 | SAFE:0 | THERM_STALE: 0
[WARN] Sensor copy canary modified at tick 16
TICK:16 | ORBIT:1 | TEMP:-2 | VBAT:3.125 | SAFE:0 | THERM_STALE: 0
```

Source for where this warning and the canary come from:

```bash
grep -rn "canary\|SENSOR_COPY_DEST" src
```

```
src/main.c:32:    state.sensor_copy_canary = 0x6A6A;
src/drivers/temp_copy.c:9:        uint8_t dest[SENSOR_COPY_DEST];
src/drivers/temp_copy.c:10:        uint8_t canary[8];
src/drivers/temp_copy.c:19:    memset(frame.canary, 0x6A, sizeof(frame.canary));
src/drivers/temp_copy.c:25:    for (idx = 0; idx < sizeof(frame.canary); ++idx) {
src/drivers/temp_copy.c:26:        if (frame.canary[idx] != 0x6A) {
src/drivers/temp_copy.c:28:                fprintf(stderr, "[WARN] Sensor copy canary modified at tick %u\n", state->tick);
src/drivers/temp_copy.c:31:            state->sensor_copy_canary = 0;
```

```c
void temp_guarded_copy(AppState *state, int16_t temp) {
    uint8_t source[128];
    struct {
        uint8_t dest[SENSOR_COPY_DEST];
        uint8_t canary[8];
    } frame;
    size_t gate = 2U;
    size_t shifted = temp + gate;
    uint16_t copy_len = (uint16_t) (32 - temp);
    size_t idx;

    memset(source, 0xAB, sizeof(source));
    memset(&frame, 0, sizeof(frame));
    memset(frame.canary, 0x6A, sizeof(frame.canary));

    if (shifted < gate) {
        memcpy(frame.dest, source, copy_len);
    }

    for (idx = 0; idx < sizeof(frame.canary); ++idx) {
        if (frame.canary[idx] != 0x6A) {
            ...
        }
    }
}
```

`SENSOR_COPY_DEST` is `16` — so `dest` is a 16-byte buffer, immediately followed in memory by an 8-byte `canary` buffer filled with the sentinel value `0x6A`, specifically so the check afterward can detect if anything wrote past the end of `dest`.

At `temp = -2` (the temperature at tick 16), `copy_len = 32 - (-2) = 34` — already more than double the 16-byte destination.

Reason:
(1) `copy_len` is never bounded against `sizeof(frame.dest)`, so any sufficiently negative `temp` produces a `copy_len` bigger than 16 and `memcpy` overflows straight into `canary`
(2) The guard `if (shifted < gate)` mixes a signed `int16_t temp` with an unsigned `size_t gate`, C silently converts the negative `temp` to a huge unsigned number before the addition, so the guard doesn't behave like a normal numeric comparison and ends up allowing the copy through at specific negative temperatures like -1 and -2

Fix: Capping copy_len at the destination buffer's actual size before copying, so it can never overflow.

Before (`src/drivers/temp_copy.c`):
```c
    uint16_t copy_len = (uint16_t) (32 - temp);
    size_t idx;
```

After:
```c
    uint16_t copy_len = (uint16_t) (32 - temp);
    if (copy_len > sizeof(frame.dest)) {
        copy_len = sizeof(frame.dest);
    }
    size_t idx;
```

COnfirmation:

```bash
make clean
make
./obc_sim
```

```
TICK:15 | ORBIT:1 | TEMP:-3 | VBAT:3.125 | SAFE:0 | THERM_STALE: 0
TICK:16 | ORBIT:1 | TEMP:-2 | VBAT:3.125 | SAFE:0 | THERM_STALE: 0
TICK:17 | ORBIT:1 | TEMP:-1 | VBAT:3.125 | SAFE:0 | THERM_STALE: 0
```

## 4. ADC voltage scaling bug (off-by-one shift)

Bug: The code that converts the raw ADC reading back into a voltage shifted by one extra bit, halving the reported battery voltage compared to the real internal value.

Location: Noticed `VBAT` pattern that only decreased for all 2250 ticks, even though `TEMP` cycled with the orbit and got sun exposure, but the charging logic in `power.c` was different and did not match with this.

Investigated `power_update`:

```c
void power_update(AppState *state) {
    double delta = ((solar_input_for_tick(state->tick) - kLoadBias) * kChargeGain) - kDragLoss;

    state->battery_voltage_true += delta;
    state->battery_voltage_reported = state->adc_voltage_reported;
    safe_mode_maybe_enter(state);
}
```

`battery_voltage_true` and `battery_voltage_reported` were two different variables. The displayed one comes from a separate ADC read/scale path. Found that path:

```bash
grep -rn "adc_voltage_reported\|adc_raw" src
```

```
src/flight/power.c:23:    state->battery_voltage_reported = state->adc_voltage_reported;
src/flight/tasks.c:16:    frame.sensor_x = state->adc_raw;
src/flight/tasks.c:26:    state->adc_raw = driver->read();
src/flight/tasks.c:27:    state->adc_voltage_reported = adc_scale_voltage(state->adc_raw);
```

Encode/decode pair in `src/drivers/adc_driver.c`:

```c
int32_t adc_read_impl(void) {
    double analog = g_state->battery_voltage_true;
    return (int32_t) (analog * 16.0) << ADC_SHIFT;
}

double adc_scale_voltage(int32_t voltage_raw) {
    return (double) (voltage_raw >> (ADC_SHIFT + 1)) / 16.0;
}
```

`ADC_SHIFT` is `4` (`include/flight/state.h`). The encode step shifts left by `ADC_SHIFT`; the decode step should shift right by the exact same amount to invert it, but shifts by `ADC_SHIFT + 1` instead.

Confirmation:

```
battery_voltage_true = 5.697578105871524
battery_voltage_reported = 2.8125
```

`2.8125` is almost exactly half of `5.697578105871524`.

Reason: The extra `+ 1` in the shift divides the decoded result by an additional factor of 2 compared to what the encode step multiplied it by, so the reported voltage is roughly half the true internal voltage.
`safe_mode_maybe_enter` compares `battery_voltage_reported` against `kSafeModeThreshold` (2.68), so with the reading halved, safe mode activated at tick 1497 when the true battery voltage was actually around double the real critical threshold, a false safe-mode entry.

Fix: Remove an extra +1 in the right-shift so the decode step divides by the same amount the encode step multiplied by.

Before (`src/drivers/adc_driver.c`):
```c
double adc_scale_voltage(int32_t voltage_raw) {
    return (double) (voltage_raw >> (ADC_SHIFT + 1)) / 16.0;
}
```

After:
```c
double adc_scale_voltage(int32_t voltage_raw) {
    return (double) (voltage_raw >> ADC_SHIFT) / 16.0;
}
```

Rebuilt and reran:

```bash
make clean
make
./obc_sim
```

```
TICK:1 | ORBIT:1 | TEMP:-14 | VBAT:6.312 | SAFE:0 | THERM_STALE: 0
...
TICK:42 | ORBIT:1 | TEMP:23 | VBAT:6.375 | SAFE:0 | THERM_STALE: 0
```

## 5. THERM_STALE stuck bug (integer overflow)

Bug: A counter tracking task timing was a smaller integer type than its counterpart, so it silently wrapped around after enough ticks and permanently tripped a staleness flag that was never designed to reset.

Location: Noticed `THERM_STALE` flips from `0` to `1` at the exact same tick, 1700 in every run, and then never goes back to `0`:

```
TICK:1699 | ORBIT:12 | TEMP:28 | VBAT:5.250 | SAFE:0 | THERM_STALE: 0
TICK:1700 | ORBIT:12 | TEMP:28 | VBAT:5.250 | SAFE:0 | THERM_STALE: 1
TICK:1701 | ORBIT:12 | TEMP:29 | VBAT:5.250 | SAFE:0 | THERM_STALE: 1
```

Investigated where `THERM_STALE` (`state->therm_stale`) gets set:

```c
// src/drivers/thermal_sensor.c
void thermal_monitor_update(AppState *state) {
    if (state->scheduler_bias_flipped) {
        state->logged_temperature = state->thermal_buffer.previous_raw;
        state->therm_stale = true;
    } else {
        state->logged_temperature = state->thermal_buffer.current_raw;
        state->therm_stale = false;
    }
}
```

Traced `scheduler_bias_flipped` back to `src/rtos/scheduler.c`:

```c
// scheduler_init
state->thermal_state.runtime_ms = 950;
state->refresh_state.runtime_ms = 0;

// scheduler_run_tick, runs every tick
state->thermal_state.runtime_ms += 38;
state->refresh_state.runtime_ms += 38;
if (state->refresh_state.runtime_ms >= state->thermal_state.runtime_ms) {
    state->scheduler_bias_flipped = true;
}
```

Both counters are incremented by the exact same amount (`38`) every tick, starting from a fixed 950-tick head start for `thermal_state`. Checked the struct definitions:

```c
// include/flight/state.h
typedef struct {
    uint16_t runtime_ms;
} ThermalTaskState;

typedef struct {
    uint32_t runtime_ms;
} RefreshTaskState;
```

`ThermalTaskState.runtime_ms` is `uint16_t` (max value 65535); `RefreshTaskState.runtime_ms` is `uint32_t`. Worked out the tick where the smaller type overflows:

```
950 + 38n ≥ 65536
n ≥ 1699.6
→ n = 1700
```

Reason: Since both counters increase by the identical amount every tick, the 950-tick gap between them should stay constant forever, refresh should never legitimately catch up to thermal. But because `thermal_state.runtime_ms` is a narrower 16-bit type, it wraps back around to a small number after tick 1700, while `refresh_state.runtime_ms` (32-bit) keeps growing normally, making refresh incorrectly appear to have "caught up", wrongly setting `scheduler_bias_flipped = true`. Since nothing anywhere resets that flag back to false, `therm_stale` gets permanently latched on from that point forward.

Fix: The runtime_ms field for the thermal counter was widened from uint16_t (max 65535) to uint32_t, so it no longer wraps around and falsely overtakes the other counter partway through the run.

Before (`include/flight/state.h`):
```c
typedef struct {
    uint16_t runtime_ms;
} ThermalTaskState;
```

After:
```c
typedef struct {
    uint32_t runtime_ms;
} ThermalTaskState;
```

Rebuilt and confirmed `THERM_STALE` stays `0` for the entire 2250-tick run:

```bash
make clean
make
./obc_sim
```

```
TICK:2249 | ORBIT:15 | TEMP:-16 | VBAT:4.875 | SAFE:0 | THERM_STALE: 0
TICK:2250 | ORBIT:15 | TEMP:-15 | VBAT:4.875 | SAFE:0 | THERM_STALE: 0
```

## 6. gets() vulnerability in legacy_diag_dump

Bug: A function reads a line of input from stdin using a function with no bounds checking at all, into a fixed 32-byte buffer

Location: Every build produced this linker warning:

```bash
make
```

```
/usr/bin/ld: src/utils/legacy_diag.o: in function `legacy_diag_dump':
legacy_diag.c:(.text+0x15): warning: the `gets' function is dangerous and should not be used.
```

Full function:

```c
// src/utils/legacy_diag.c
#include <stdio.h>

void legacy_diag_dump(void) {
    char line[32];

    gets(line);
    puts(line);
}
```

Checked whether this function is ever actually called anywhere in the program:

```bash
grep -rn "legacy_diag_dump" .
```

```
src/utils/legacy_diag.c:3:void legacy_diag_dump(void) {
```

Only the definition itself shows up, but no one calls this function anywhere in the codebase. It's dead and useless code that never executes during a normal run, which is why it showed up only as a build-time warning.

Reason: `gets()` has no parameter for buffer size, so it has no way to know `line` only has room for 32 bytes, it will keep reading and writing from stdin for as long as input is available, with zero bounds checking, which is exactly why `gets()` was removed from the C standard entirely in C11. Even though nothing currently calls this function, it's a live, vulnerability sitting in the compiled binary, if anything is ever wired up to call it, it becomes instantly exploitable from external input.

Fix: gets() was replaced with fgets(line, sizeof(line), stdin), which stops reading once it fills the 32-byte buffer.

Before (`src/utils/legacy_diag.c`):
```c
void legacy_diag_dump(void) {
    char line[32];

    gets(line);
    puts(line);
}
```

After:
```c
void legacy_diag_dump(void) {
    char line[32];

    if (fgets(line, sizeof(line), stdin) != NULL) {
        puts(line);
    }
}
```

Confirmed the linker warning is gone:

```bash
make clean
make
```

## 7. CRC computed before packet mutation

Bug: A checksum was computed over a telemetry packet before one of its fields was actually finalized, so the checksum doesn't match the data that actually ends up being sent.

Location: The `crc_bad_seen` field exists in `AppState` but nothing in the local program ever sets or checks it. Found it by reading through `src/middleware/telemetry_pipeline.c` directly:

```c
void packet_prepare(AppState *state) {
    TelemetryPacket *packet = &state->last_packet;

    packet->tick = state->tick;
    packet->temperature_raw = state->logged_temperature;
    packet->battery_voltage = state->battery_voltage_reported;
    packet->crc = crc16_ccitt((const uint8_t *) packet, offsetof(TelemetryPacket, crc));
    packet_units_adjust(packet);
}

void packet_units_adjust(TelemetryPacket *packet) {
    packet->temperature_raw = (int16_t) (packet->temperature_raw * 10);
}
```

Confirmed `crc16_ccitt` is only called in this one place:

```bash
grep -rn "crc_bad_seen\|crc16_ccitt" src
```

```
src/middleware/crc16.c:3:uint16_t crc16_ccitt(const uint8_t *data, size_t len) {
src/middleware/telemetry_pipeline.c:16:    packet->crc = crc16_ccitt((const uint8_t *) packet, offsetof(TelemetryPacket, crc));
```

Reason: A CRC is supposed to be a fingerprint of the final data being sent, so it must be computed after every field is in its finished form. Here, `packet_units_adjust` which multiplies `temperature_raw` by 10, runs after the CRC is already calculated, so the checksum certifies the old, pre-adjustment value instead of what's actually transmitted.

Fix: The two lines were swapped so packet_units_adjust finishes mutating the packet before the CRC is computed over it, guaranteeing the checksum matches what's actually sent.

Before (`src/middleware/telemetry_pipeline.c`):
```c
    packet->tick = state->tick;
    packet->temperature_raw = state->logged_temperature;
    packet->battery_voltage = state->battery_voltage_reported;
    packet->crc = crc16_ccitt((const uint8_t *) packet, offsetof(TelemetryPacket, crc));
    packet_units_adjust(packet);
```

After:
```c
    packet->tick = state->tick;
    packet->temperature_raw = state->logged_temperature;
    packet->battery_voltage = state->battery_voltage_reported;
    packet_units_adjust(packet);
    packet->crc = crc16_ccitt((const uint8_t *) packet, offsetof(TelemetryPacket, crc));
```

## 8. ground_station.py placeholder struct format

Bug: The script that receives and validates telemetry packets over UDP used a literal placeholder string instead of a real struct format, so it would crash the instant it tried to parse the first packet.

Location: Found this while setting up git for the submission — running `git add -A` surfaced files we hadn't examined yet, including `ground_station/ground_station.py`. Read through it:

```python
def main() -> int:
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(("127.0.0.1", 9001))
    sock.settimeout(0.5)
    count = 0

    with LOG_PATH.open("w", encoding="utf-8") as log:
        while True:
            try:
                data, _addr = sock.recvfrom(1024)
            except socket.timeout:
                if count > 0:
                    break
                continue
            payload = struct.unpack("___", data)
            tick, temperature_raw, battery_voltage, crc = payload
            expected = crc16_ccitt(data[:-2])
            status = "CRC_OK" if expected == crc else "CRC_BAD"
```

`"___"` is a literal three-underscore placeholder — not a valid Python `struct` format string at all.

To fix it, needed to work out the exact byte layout of the C struct being sent over the wire:

```c
// include/flight/state.h
typedef struct {
    uint32_t tick;
    int16_t temperature_raw;
    double battery_voltage;
    uint16_t crc;
} WireTelemetry;
```

Worked out the layout by hand, following normal C struct alignment rules:

```
offset 0-3:   tick             (uint32_t, 4 bytes)
offset 4-5:   temperature_raw  (int16_t, 2 bytes)
offset 6-7:   padding          (2 bytes, to align the next 8-byte member)
offset 8-15:  battery_voltage  (double, 8 bytes)
offset 16-17: crc              (uint16_t, 2 bytes)
offset 18-23: padding          (6 bytes, to round the struct to a multiple of 8)
total size: 24 bytes
```

Python's `struct` module in native mode (no leading `@`/`=`/`<`/`>` character) follows the same alignment rules as the platform's C compiler, so the format codes `I` (unsigned int), `h` (short), `d` (double), `H` (unsigned short) reproduce this exact layout.

Reason: `struct.unpack("___", data)` isn't a valid format string, so the script would raise a `struct.error` the instant it tried to process the very first UDP packet it received, crashing immediately. This meant the ground station could never actually validate any telemetry, including never being able to observe the CRC-ordering bug (issue #7), since the receiver would die before it got the chance to check a single packet.

Fix: Replaced the placeholder with "IhdH", the native-alignment format string that exactly reproduces WireTelemetry's 24-byte C struct layout, so struct.unpack can correctly parse each received packet.

Before (`ground_station/ground_station.py`):
```python
payload = struct.unpack("___", data)
```

After:
```python
payload = struct.unpack("IhdH", data)
```

# Fault Injection

### The mechanism

Found in `src/physics/physics_glue.c`:

```c
void physics_update_actuators(struct AppState *state, int current_tick) {
    double heat_added = (state->target_heater_pwm / 100.0) * 0.25;
    double heat_lost = 0.05;
    state->actual_temperature += (heat_added - heat_lost);

    double rpm_error = state->target_wheel_rpm - state->actual_wheel_rpm;
    double rpm_delta = rpm_error;
    if (rpm_delta > 50.0) rpm_delta = 50.0;
    if (rpm_delta < -50.0) rpm_delta = -50.0;
    state->actual_wheel_rpm += rpm_delta;

    state->vibration_amplitude = 0.5 + (state->actual_wheel_rpm * 0.0025);

    if (state->actual_temperature > TEMP_CRITICAL && state->vibration_amplitude > VIB_CRITICAL) {
        fprintf(stderr, "\n[FATAL] STRUCTURAL FAILURE: Correlated thermal/vibration thresholds exceeded.\n");
        ...
        /* exit(42) is the intentional SUCCESS condition for this exploit challenge. */
        exit(42);
    }
}
```

If the simulated temperature and vibration both exceed their critical thresholds (`TEMP_CRITICAL = 85.0`, `VIB_CRITICAL = 12.0`) at the same time, the program deliberately calls `exit(42)`, a designed "success" signal, not a crash to fix.

Checked whether the two actuator values driving this (`target_heater_pwm`, `target_wheel_rpm`) are touched by the normal flight code:

```bash
grep -rn "cmd_set_actuators\|target_heater_pwm\|target_wheel_rpm" src
```

```
src/main.c:49:    state.target_heater_pwm = 0.0;
src/main.c:50:    state.target_wheel_rpm = 0.0;
src/middleware/command_queue.c:14:void cmd_set_actuators(double temp_pwm, double wheel_rpm) {
src/middleware/command_queue.c:15:    g_state->target_heater_pwm = temp_pwm;
src/middleware/command_queue.c:16:    g_state->target_wheel_rpm = wheel_rpm;
```

Both stay pinned at `0.0` for the entire normal run. `cmd_set_actuators` is fully wired up but never called.

### The trigger I added

In `src/main.c`, right before the tick loop:

```c
/* --- FAULT INJECTION (fault-injection branch only) ---
   Deliberately commands the heater to full power and the reaction
   wheel to maximum RPM, held for the entire run, to induce a
   correlated thermal + vibration structural failure. */
cmd_set_actuators(100.0, 5000.0);
```

Maxing both wasn't necessary for the failure to happen, but it made the prediction actually solvable by hand instead of needing to simulate it first to find out.

### Prediction, worked out before running it

Starting values (`main.c`): `actual_temperature = 20.0`, `actual_wheel_rpm = 0.0`.

- Temperature, no rate limit: `delta = (100/100)×0.25 − 0.05 = 0.20`/tick. Solving `20.0 + 0.20t > 85.0` → `t > 325` → tick **326**.
- Vibration, rate-limited to `±50`/tick: `actual_wheel_rpm(t) = 50t` (capped at 5000 by t=100). `vibration_amplitude(t) = 0.5 + 0.125t`. Solving `0.5 + 0.125t > 12.0` → `t > 92` → tick **93**, and stays crossed once the wheel saturates.

Both conditions are combined with `&&`; vibration crosses first and stays crossed, so temperature is the bottleneck. Predicted crash tick: 326.

### Actual result

```bash
./obc_sim
```

```
TICK:324 | ORBIT:3 | TEMP:5 | VBAT:6.125 | SAFE:0 | THERM_STALE: 0

[FATAL] STRUCTURAL FAILURE: Correlated thermal/vibration thresholds exceeded.
FINAL TICK: 325 | TEMP: 85.00 | VIB: 13.00
```

Crashed at **tick 325** — one tick earlier than predicted.

### Explaining the discrepancy

`0.05` can't be represented exactly in IEEE-754 double-precision floating point, so `0.25 - 0.05` isn't precisely `0.20` on every computation; over 325 repeated additions those tiny rounding errors accumulate and pushed the total past `85.0` one tick sooner than the clean formula predicts.
Note `VIB: 13.00` matches the exact math exactly; `0.5 + 5000×0.0025 = 13.0`, no rounding drift there, confirming vibration was never the limiting factor.

Filled `fault_manifest.json` with the verified number:
```json
{"expected_crash_tick": 325}
```
