# omnitrix
A functional omnitrix from the Ben 10 universe
Here's the video where I build it: - <a href="https://www.youtube.com/watch?v=m1AEkRq4pEk" target="_blank">https://www.youtube.com/watch?v=m1AEkRq4pEk</a> 

![Omnitrix](https://raw.githubusercontent.com/tolgaozuygur/omnitrix/main/images/omnitrix_photo.jpg)

## Parts Required
- <a href="https://hallederiz.io/products/crowpanel-2-1inch-esp32-rotary-ekran-480-480-ips" target="_blank">1x Crowpanel 2.1 inch ESP32 Rotary Display</a>
- <a href="https://www.ozdisan.com/p/arduino-sensorleri-ve-modulleri-613/boardoza-mcp73831-1206515" target="_blank">Boardoza MCP73831 Battery Charger</a>
- <a href="https://www.ozdisan.com/p/arduino-sensorleri-ve-modulleri-613/boardoza-tps63020-5v-1580501" target="_blank">Boardoza TPS63020 5V buck-boost converter</a> (this was out of stock, you can use any 5v buck boost module -1A min- as a replacement)
- <a href="https://www.ozdisan.com/p/arduino-sensorleri-ve-modulleri-613/ozd-arduino-buzzer-module-525792" target="_blank">Buzzer module</a>
- <a href="https://www.ozdisan.com/p/surgulu-sivicler-495/kls-electronic-l-kls7-ss05-12g13g6-456117" target="_blank">Slide Switch</a>
- 1x 18650 3.7v Battery (preferably with pre-soldered cables and built in BMS module. I used a 3200 mAh one.)
- 4x M4x20 bolt
- 4x M4 nut
- 11x M3x10 bolt
- 8x M3 nut
- 15x M2x6 bolt
- 6x M2 nut
- 2x Toggle Latch. It should be 65mm in length when locked. Check the YouTube video above to see the type I used (tr: sandık klipsi kilidi)

## Uploading the code to the ESP32 Screen
Upload the omnitrixFirmware.ino using Arduino IDE. Please refer to this [guide](https://www.elecrow.com/wiki/CrowPanel_2.1inch-HMI_ESP32_Rotary_Display_480_IPS_Round_Touch_Knob_Screen.html) to be able to upload to the screen.
Note: The partitioning instructions on this guide is wrong. Use the values I provided in the "partition fix" in this repo. 

## Wiring Diagram and 3d Printing
Wiring is pretty simple, I didn't prepare an assembly guide for the printed parts but it also is pretty straight forward. If you have any issues please check the Youtube video above.
![Wiring Diagram](https://raw.githubusercontent.com/tolgaozuygur/omnitrix/main/images/wiring_diagram.png)
![Wiring Photo](https://raw.githubusercontent.com/tolgaozuygur/omnitrix/main/images/wiring_photo.jpg)

## Customizing the interface
If you want to change some assets in the interface, you can edit the OmnitrixUI using Squareline







