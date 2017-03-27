# ns3-primer
The basic NS3 scripts to get you started building WiFi networks.

## Quick startup
In order to start a simulation, just issue the following command:
./waf --run "scratch/quick-vht-wifi"

It will create an IEEE 802.11ac network with one AP, and will execute a simulation with one station, ouput the results through the screen, and then repeat the simulation but with two stations.

The mofifiable parameters at this moment are:
	- channelWidth: 20, 40, 80 or 160 MHz.
	- mcs: 0-9
	- stations: each simulation is iterated over (1..stations).
	- distance: separation between AP and stations
	- simulationTime: simulation time.

In order to pass values other than the default, you must issue the execution command with the updated parameters in this way:
./waf --run "scratch/quick-vht-wifi --stations=x --channelWidth=y"

, and so on for every input parameter you want to change (you can also modify the main script to add your own).

## Using executioner.py
An executioner.py script is added in order to help you create scenarios. That is, instead of plugging additional values for the predefined input parameters, you can define different scenarios and iterate your simulations without worrying if you set the values for all simulations correctly.

The executioner.py script expects one parameter. This is defined as the "scenario". Currently, two default scenarios are defined:
	- 0: 1 station, 2 seconds of simulationTime.
	- 1: 1 station, mcs 7.

Default values are defined at initialization of the SuperTable class (see executioner.py). These values correspond to the same input parameters defined for the ns-3 script. After creating your scenario, you can trigger your simulations through executioner.py.

============================

Special notes:
The ns-3 script should be located inside the scratch directory. While executioner.py should be placed at the same level of the scrat directory.


Have fun.

L


