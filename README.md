KinectTemporalMaskEditor
========================

Using a Kinect to control temporal distortion of the kinect video input.

Appearances
-----------

Video of Time Distortion Machine being used to distort a Cheetah.
http://danielmclaren.tumblr.com/post/49997667085

Time Distortion Machine at Vancouver Mini Maker Faire
http://danielmclaren.tumblr.com/post/51944052777

Video Loops at Lab Art Show
http://danielmclaren.tumblr.com/post/76018699679

Time's Turn at MakerLabs "We're Here" event
http://danielmclaren.tumblr.com/post/90948489865

Time's Turn at Your Kontinent Festival 2014
http://danielmclaren.tumblr.com/post/92546525635

Notes
-----

Use these ffmpeg commands to extract frames as images and then recombine them into a movie clip:

`ffmpeg -ss 0:00:00 -i "input.mov" -t 10 -sameq -f image2 out/frame%04d.jpg`

`ffmpeg -f image2 -start_number 1 -i out/frame%04d.jpg -vcodec libx264 -sameq output.mov`
