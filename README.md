# LDID User windows application. 

General system description
==========================

The LDID system (don't remember exactly what the initials stand for) was a set of small programs created to solve a situation in which there were the need to install several PCs as servers using ISP connections with dinamic IPs, and those servers must be available at all times at real IP level to clients.
An obvious solution was to use one of several dynamic DNS services available in the market (comercials or not), but as the company I was working for (and the one that will use the system) had a set of real IPs available, and in those days I was really inspired in the creation of communication services, it was easier for me, and more secure for the company in the long run, to design our own dynamic dns service.

The system was composed by three programs:

1- A central daemon running in a server with real IP in the company headquarters.

2- A daemon for the server that will run with a dynamic IP.

3- A program for the clients that needed to locate the dynamic IP servers.

It's functioning was quite simple: the daemons in the dynamic servers ran an infinite cycle sending messages with their IP address periodically to the central server. This one then kept a table updated with that information (indexed by a known key for each server). When the clients needed to connect to a dynamic server they requested the IP address of this one using the key of the server they wanted to access.


This program description
=======================

This is the windows program for the client. In this specific version of the application was included a function for, after being resolved the IPs addresses of the possible dynamic servers, configure a vpn (using an OpenVPN file), connect to the openvpn server and finally running a user application program. A pending action in this project is to change those function to be optionals. 

The working parameters would be (read from settings.txt):

Par�metros:

	* Working Mode (modo):

		0 : One or more servers with real IPs.
		1 : One or more servers with dynamic IPs (it's used then the LDID server).

	* VPN working range (rango).
	* Real IP of the LDID server (Servidor_LDID).
	* UDP port of the LDID server (Puerto_LDID).
	* Names of the application servers (arreglo_nombres_servidores).
	* Time between requests to the LDID sever (tiempo_ldid).
	* Total requests to LDID server before failure (intentos_ldid).
	* Port of the OpenVPN server in app server.
    * Path of the OpenVPN program (Path_ovpn).
    * Path of the user application program (Path_teleEEG).

modo = 0;
rango = 0;
Servidor_LDID = NULL;
Puerto_LDID = 0;
Cantidad_servidores = 0;
arreglo_nombres_servidores = NULL;
tiempo_ldid = 0;
intentos_ldid = 0;
puerto_ovpn = 0;
Path_ovpn = NULL;
Path_teleEEG = NULL;
