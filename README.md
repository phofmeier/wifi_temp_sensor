# Wifi Temperature Sensor

## Circuit Diagram

The circuit diagram shown below is for one NTC Temperature Sensor.  

```
         ^  3.3 V 
         |
         |
        +++
        | | R_1/2: 22 kOhm
        +++
         |
ESP32    |    +---------------+
Pin -----+----| Sensor        | 
34/35      +--| Connector 1/2 | 
           |  +---------------+
           |   
          ---
          GND
```

## Microcontroller 

- NodeMCU Lua Lolin V3 Module ESP8266 ESP-12E
- [Link](https://www.amazon.de/AZDelivery-NodeMCU-ESP8266-ESP-12E-Development/dp/B06Y1ZPNMS/ref=sr_1_3?_encoding=UTF8&camp=1634&creative=19450&keywords=esp8266%2Bboard%2Bv3&linkCode=ur2&qid=1575812938&sr=8-3&th=1#detail_bullets_id)
- Arduino MKR1000
- Arduino Nano 33 IoT denke bester
- ESP32

## Temperature Sensor


- if Pt100 use somthing like [this](https://www.adafruit.com/product/3328) but check before if really a pt100
- Klinkenstecker für die Sonden
- if NTC ADC needed

### Model
Using the Model of a NTC resistor and the Voltage divider we can make the functions below  
$$
R_{\mathrm{NTC}} = R_\mathrm{N} \exp({B (\frac{1}{T}-\frac{1}{T_\mathrm{N}})} )
$$
$$
R_\mathrm{N} \in \mathbb{R}^+ \ Resistance \ at \ temperatur \ T_\mathrm{N} \ in \ Ohm \\
T_\mathrm{N} \in \mathbb{R} \ in \ Kelvin
$$

$$
U = \frac{3.3 V}{R_1 + R_\mathrm{NTC}} R_\mathrm{NTC}
$$

The temperature itself can be calculate after changing the equation on the top

$$
T = \frac{1}{\ln(R_\mathrm{NTC}/R_\mathrm{N})/B +1/T_\mathrm{N}}
$$

$$
R_\mathrm{NTC} = \frac{U R_1}{3.3 - U}
$$