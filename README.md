# Toki-Robot
A mini companiton desk robot.

## 1. In inital step is to setup the microcontroller and OLED Test
#### Enusre following libs installed in Adrduino IDE, Adafruit_GFX, Adafruit_SSD1306.
    We will use ESP32, connect the OLED display and check its working.
    Also we can add animated facial expressions in this step.
    Additional script added for touch sensor, also use Serial port(type 1 and enter) to change faces in other script.
    
<img width="420" height="460" alt="image" src="https://github.com/user-attachments/assets/1520eec2-e310-4438-a675-7799c7150816" />


## 2. Now test for webserver, we want to use the face oled script and webserver.
    This step involves a webserver in ESP32, we will create a webpage and connect to it with mobile or pc.
    We can change on screen face moods with this.
    This will be a setup for later, we will give facial experessions to ai model that we will integrate later.

    2.1 PC browser (Expression buttons / Device status)

    2.2 Your Wi-Fi router (PC and ESP32 on the same network)

    2.3 ESP32 DevKit (HTTP server / OLED renderer / Toki's state)

