# Hardware Demo

[Watch / download the MP4](https://github.com/Cupcake137/IPC_Project/releases/download/v1.0.0/IPC_Project_Demo_HW_2026-10-08.mp4)

Recorded for the project on 2026-10-08. Edited from three recordings
recordings and four hardware photos. Duration: 165 seconds; 1920 x 1080,
30 fps. Original recordings remain outside the source repository.

| Start | Demonstration |
| --- | --- |
| 00:00 | Assembled hardware overview |
| 00:15 | Uno, motor driver, Pi/UART interface, ESP32 keypad and input details |
| 00:35 | Cluster telltales and debug console |
| 01:01 | Menu navigation and display settings |
| 01:17 | Live gear and throttle response |
| 01:34 | NORMAL drive mode |
| 01:49 | SPORT drive mode |
| 02:03 | Continuous ECU gear-button, potentiometer and motor demonstration |

The 42-second motor segment is continuous footage, not speed-adjusted.
English captions distinguish modeled speed/SOC and display settings from real
motor control. The recording demonstrates functional behavior, not a long-term
electrical-noise test, measured vehicle speed or certified fault validation.

The video predates the final UART-watchdog source correction. Subsequent stable
Pi operation was confirmed by the maintainer; the UART regression results are recorded
separately in LOCAL_TEST_RESULTS.md. The footage is not represented as a recording
of every later source revision or all release-acceptance checks.

The MP4 is a GitHub Release asset to keep binary media out of source history.
The README thumbnail is an actual frame from the video, not a design mockup.
