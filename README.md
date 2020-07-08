# Emergency Landing Guidance System (ELGS)

Created by: Alec Williams

![Image of ELGS](https://github.com/AlecWilliams/ELGS/blob/master/ELGS.PNG)

## The Idea
The purpose of this project is to provide pilots with a system that can safely analyse and calculate a landing path to bring the aircraft down. With the aircraft's gliding ratio and GPS data such as elevation and velocity, we can calculate the maximum possible range the aircraft has in the case of engine failure. Using that information and a database of pre-analyzed satellite imagery, we can determine the optimal path for the aircraft to follow so that it touches down on even and safe terrain.

The project of analyzing satellite imagery to create and store potential landing zones is still under development, so for the sake of this project I manually went over several images of San Luis Obispo and created possible landing zones by hand. Zones marked with zig zag lines are landig zones where an aircraft must approach at a certain angle due to terrain features such as rows on farm fields. For this demonstration, I only marked a few zones as such.

## The Process
Once the satellite imagery has been analyzed, the result is a black image with possible landing zones in green. Zones that need to be approached at a certain angle are a slightly different shade of green. This due to the fact that I used the blue value in the RGB for each pixel in the zone to store the angle of approach needed. This information is used later on when calculating possible landing paths in each zone.


A flood fill algorithm is then run on the image giving the exact size and shape of each zone stored as pixel coordinates in a vector. With this information the program can then calculate possible lading paths in each zone that are at least the minimum landing distance required for a small aircraft. These paths are then stored in a vector to be used later. Below is an example image of the possible landing paths calculated from the landing zones.



With all possible landing paths known, we can calculate the optimal three paths in real time and display them. The path is updated as the plane moves and once in landing mode the program will redirect the aircraft if it strays too far from its current landing path. If the path is followed, the plane should reach an elevation of 0 right at the start of the landing zone.




To run program, open in visual studio and run. May take up to 1 minute to launch.
Requires OpenGL be installed on C:\
Controls: w,a,s,d to move the plane around. E and Q to change altitude, G to enter gliding mode.

Software only provides guidance, plane must manually follow landing path.
