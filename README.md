# MouseTracker

A small Windows tool for checking how well your mouse is tracking.

Every time your mouse sends a report, MouseTracker draws a cursor where the pointer is. Move the mouse quickly and you get a trail of cursors. If the trail is evenly spaced, your mouse is fine. If there are big jumps in it, the mouse is skipping or dropping reports.

It also shows your polling rate in the top left corner:

- **Current** is the rate over the last quarter second
- **Average** is the rate for the whole session

Both only count time when the mouse is moving. It works with mice up to 8000 Hz.

## How to use it

1. Run MouseTracker.exe
2. Click the window so it is in focus
3. Swipe the mouse around inside it

The trail clears itself when you stop for half a second and start moving again.

You can change the background and cursor colors from the Color menu.

## Tips

- Fast swipes show skips best. At slow speeds the cursors overlap, which is normal.
- Close other heavy programs while testing so they do not affect the result.
- For wireless mice, try testing at different distances from the receiver.

## Building

Open the solution in Visual Studio 2022 and build in Release. All of the code is in MouseTracker.cpp. There is nothing else to install.

## License

MIT
