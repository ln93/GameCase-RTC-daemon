# GameCase-RTC-daemon
A daemon program to communicate with RTC Chips of GameCase series. 

run

    ./rtc -d

The program would:

1. Set Linux localtime from RTC time.
2. Log&refresh battery,backlight,volume to /dev/shm/battery, /dev/shm/backlight, /dev/shm/volume every 1 sec.
3. Update RTC time from linux localtime every 20 sec.
4. If batterylevel is dropped to zero for over 20 sec, it will try to tell Retroarch to save state&quit, then shutdown console.
