# LEDband driver software
This software is kind of a hello-world example. It would display a text by scrolling the text in.

This code is currenty not functional as some bits are still missing. These will be added later on.

## Fonts
To test things there is a simulator. Currently, the software is still missing a font file. It should be formatted in 8x8 blocks (thus 8 bytes per character), following the ASCII tables. Save it as font1.c as:
```
unsigned char font1[2048] =
{
...
}
```
Some fonts can be found on the internet. They are not included here for licensing reasons. At some point I may create one myself.
