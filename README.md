### 🛠️ Journey of a 3D Printing Tinker

When I first got into 3D printing, bought my initial printer, and started upgrading it, I instantly fell in love with the custom peripherals built by the community. Over the years, my mancave has slowly turned into a display gallery of various printer monitors scattered around.

It all started with Qrome's printer monitor, followed later by the version from nospig (Mark Gibson).

A lot has changed in the 3D printing and Arduino worlds over the years. Alongside Marlin, we now have Klipper, and wonderful affordable hardware like Cheap Yellow Displays (CYD). Naturally, I wanted a dedicated monitor for my Klipper setups too. I stumbled upon MoonWatch by rackrick—a fantastic project that now sits proudly next to my K1 Max. Then I found KlippyMon by wabbitguy, which is also guarding one of my printers. Of course, the Qrome and nospig builds are still part of the collection, all running to my complete satisfaction in the mancave.

### 🌤️ The Spark of Inspiration

The nospig printer monitor has always been one of my favorites because it doubles as a weather station—another great interest of mine. For years, I struggled to get the screengrab function working. I even asked a question on the repository ages ago, but never got a response; the original author is probably just out enjoying life elsewhere!

However, while tinkering the other day, I finally managed to crack it and get it working. That success gave me the inspiration to take this classic ESP8266 + ILI9341 project and port it over to run on a CYD (Cheap Yellow Display).

### 🚀 The Result

And I'm pretty damn proud to say I pulled it off!

* **Multi-Firmware Support**: It now works on both Marlin and Klipper (though Klipper requires a few configuration tweaks in your .cfg files depending on your setup).
* **Core Metrics**: It displays live temperatures for both nozzle and bed, print progress time, and the current file being printed.

All in all, once housed in a nice custom enclosure, it's ready to take its place alongside the other monitors in the mancave.

### 🙏 Credits & Acknowledgments

A massive thank you to all the brilliant tinkerers who laid the groundwork for these projects:

* **nospig (Mark Gibson)**
* **Qrome**
* **rackrick (MoonWatch)**
* **wabbitguy (KlippyMon)**

*And for fellow tinkerers wanting to get this running on their CYD: good luck and enjoy figuring it out, just like I did!*
