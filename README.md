# Inkplate

A project that supports the Soldered Inkplate 10 (and possibly the earlier radonica model) in the ESP-IDF.

Currently a weather station, plus framework for creating more projects. Has a captive portal. Embeds SolidJS content.

Must have nodeJS, vite, and platformIO installed to build.

The weather station uses weatherapi.com to fetch weather data for your area (configured via the portal)

You can reset the configuration by using the left button any time the device is asleep. It will also show up if it cannot connect to the network.

Otherwise the device will wake in sync with weatherapi.com info updates to update the display.

If the specified location is in the USA, the station defaults to imperial units. Otherwise it defaults to metric. Either way, it can be overridden on the configuration web.

Location can be specified as geo-coordinates, or in the US/CA/UK can be a city or postal/zipcode

The timezone may also be properly configured to show the update time as local time, but this isn't required since it can fetch it from worldtime, which it also uses to set the embedded RTC (which is otherwise unused by this project)

