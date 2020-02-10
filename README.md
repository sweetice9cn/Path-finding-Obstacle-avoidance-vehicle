# Path-finding-Obstacle-avoidance-vehicle
Nov, 2019- Dec, 2019
<h3>Picture of the embedded system car</h3>
<img src="0.png">
<h3>Materials</h3>
<ul>
	<li>car kit with two motors</li>
	<li>Feather HUZZAH esp8266 microcontroller</li>
	<li>Motor Driver (DRV8833)</li>
	<li>2 x ultrasonic sensors</li>
	<li>4 x AAA batteries</li>
<li>Portable battery for powering the microcontroller</li>
<li>tape, wire</li>
</ul>
<h3>Block diagram of the algorithm</h3>
<img src="1.png">
<h3>Block diagram for the obstacle avoidance algorithm</h3>
<img src="2.png">
<h3>Implemented functions</h3>
<p>car movement</br>
<ul>
	<li>void moveForward(), void moveBackward()</br>
		move car forward/backward, using delay() to control moving distance
	</li>
	<li>void turnLeft(), void turnRight()</br>
		move car forward/backward, using delay() to control moving distance
	</li>
	<li>void turnLeftUnit(), void turnRightUnit()</br>
		turn car 30 degrees left/right for fine-grained beacon finding, using delay() to control turning degrees
	</li>
</ul></p>
<p>Signal strength (RSSI) measurement</br>
