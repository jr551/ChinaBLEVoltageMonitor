# BLE Voltage Lab Handlebar Holder

Parametric OpenSCAD holder for the Waveshare ESP32-C6-LCD-1.47 display board.

The model is shaped like a small watch pod: a rounded body, front retaining
face, LCD window, back-loading PCB pocket, and a bottom wire exit for power
leads.

It targets:

- 22.2 mm bicycle handlebars
- Waveshare board outline: 20.32 x 36.37 mm
- four M2 board holes with 13.28 x 32.00 mm centre spacing
- M4 clamp bolts
- bottom cable hole: 7.0 x 4.5 mm

Open `handlebar_holder.scad` in OpenSCAD, render, then export STL. Print a rough draft first and tune the variables at the top for your printer, cable, and exact board fit.

Suggested first print:

- PETG or ASA
- 0.2 mm layer height
- 4 walls
- 35% infill
- watch face upward

Source dimensions are from the official Waveshare ESP32-C6-LCD-1.47 drawing.
