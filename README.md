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
- ESP32

## Temperature Sensor

- NTC

### Model
Using the Model of a NTC resistor and the Voltage divider.  
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