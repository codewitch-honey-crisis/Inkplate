# Inkplate

A project that supports the Soldered Inkplate 10 (and possibly the earlier radonica model) in the ESP-IDF.

Currently a weather station, plus framework for creating more projects. Has a captive portal. Embeds react content.

Must have nodeJS, vite, and platformIO installed to build.

The weather station uses weatherapi.com to fetch weather data for your area (configured via the portal)

You can reset the configuration by using the left button any time the device is asleep.

Otherwise the device will wake in sync with weatherapi.com info updates to update the display.