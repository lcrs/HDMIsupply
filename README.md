# HDMIsupply
Live video input plugin for Flame/Smoke - captures input from a Blackmagic Ultrastudio Mini Recorder and pipes it in as a timeline clip or a batch node. Quick demo vid:

[![HDMIsupply demo vid](http://img.youtube.com/vi/5m1iBZD-_JU/3.jpg)](https://www.youtube.com/watch?v=5m1iBZD-_JU)

## How
Download from the releases page or the green button top right, make sure Flame isn't set to use the Blackmagic video device for preview or capture, make sure your project framerate and the input's framerate match, then add the Spark.

Select the right input in the Blackmagic Desktop Video Setup app, and use their Media Express app to check that the input works outside of Flame.  Tested with an Ultrastudio Mini Recorder, the tiny Thunderbolt one, but may work with others which support an uncompressed 10bit YUV HD input.

## Tips
If the playback is showing the wrong framerate temporarily add a clip of the correct rate to the tree and then delete it again - this is probably a bug on Autodesk's side

For best performance recording to the node cache set your project's format to "Uncompressed / RAW" so Flame doesn't have to compress to OpenEXR or ProRes on the fly.

If using this on-set do chat to the camera dept beforehand, especially if you want a feed that's clean of the usual camera metadata and framelines - it may be complicated to route the HDMI or SDI via the playback op's setup.  Piggybacking off a monitor's SDI loopthrough without asking is a little rude :)

For a ton of messages about what is happening, set the HDMISUPPLY_DEBUG environment variable a la:
```
env HDMISUPPLY_DEBUG=1 /opt/Autodesk/flame_2019.2.1/bin/startApplication
```

## Building
Only tested on macOS 10.14.1 with Flame 2019.2.1 so far, and requires the older Xcode 9.4.1 to compile since Flame seems to ship with an old copy of install_name_tool which can't handle newer binaries.

It is possible it will work with minimal changes on Linux if the standard Autodesk CentOS setup includes Thunderbolt drivers and the Blackmagic stuff installs properly...
