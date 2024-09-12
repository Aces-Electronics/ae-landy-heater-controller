

<h1 align="center">
  <a href="http://aceselectronics.com.au"><img src=".repo_files/ae_red_320_nobg.png" alt="Ace's Electronics" width="200"></a>  

  Ace's Electronics Pty Ltd  
  12V heater controller for mirrors and windscreens  
</h1>

# Looking to get up and running?
- [Wiki - Quickstart Guide](../../wiki/Quick-Start-Guide)

# Project Goals:
The overall goal of this project is to make a simple, robust and reliable heater controller for vehicles that have heated mirrors and windscreens, like a Land Rover Defender for example. The controller will be solid state and be able to handle up to 30A at 12-14V with a maximum input voltage of 20V.

# Design goals:  
- Smallest form-factor possible while being able to handle 30A without getting too hot
- Simple configuration
- Easy to mount
- Zero maintenance

# JonoTron-3000 v0.3
Version 0.3 was made to address a small issue with the switch pull-up, which was incorrectly tied to 12V instead of 3.3V. The hardware pull-up has been removed completely in ths version as the ESP32 has internal pull-ups that are more than capable.

![v0.3 Front](product_images/Screenshot%202024-09-12%20at%2010.33.30 PM.png)
![AE-Land-Heater_Controller](product_images/Screenshot%202024-09-10%20at%209.38.59 PM.png)

# JonoTron-3000 v0.2
Version 0.2 was made to addresss a couple of isssues, namely:
- Input and ouputs polarity was swapped, thanks to the xt-60 injection mould anomally that made a - look like a + when I eyeballed it to see which side was negative
- All outputs were tied together and were't individually controllable
- The board couldn't really handle 30A, not safely

Boards are tested and workng well!

v0.2 renders: 

![v0.2 Front](.repo_files/v0_2/JonoBro_v0_2.png)

# Where to Buy:
Available to purchase in Australia or New Zealand via <a href="https://d1b959-f7.myshopify.com/">The AE Store</a>

## Support:
Contact me direct via ace<@>aceselectronics.com.au, just make it look like a normal email address. (hiding from the bots ;)
