_The App (Android) will be available soon. There are some small details that still need to be sorted out before the Repository can be made public._

# PalPalette Controller

_(english Version below)_

Der PalPalette Controller ist dafür zuständig, die Verbindung zwischen der PalPalette App und den Nanoleafs herzustellen. Die von Nanoleaf bereitgestellte API ist nur lokal zu erreichen. Daher benötigt jeder Freund einen PalPalette Controller, der sich im selben Netzwerk wie die Nanoleafs befindet.

Der Controller übernimmt nun mehrere Aufgaben:

1. Abfrage der Layout-Daten: Es werden die Layout-Daten der Nanoleafs abgefragt. Verändert sich etwas am Layout, so wird diese Veränderung mit minimaler Verzögerung an das Backend weitergeleitet.
2. Ping an das Backend: Es wird in regelmäßigen Abständen ein Ping an das Backend gesendet, um zu signalisieren, dass der Controller und somit der Freund erreichbar ist.
3. Farbenübertragung über MQTT: Über MQTT erhält der Controller die von Freunden versendeten Farben. Diese werden vom Controller verarbeitet und an die Nanoleaf API weitergeleitet, um die Paneele aufleuchten zu lassen.

## Aufsetzen des PalPalette Controllers

_In der aktuellen Projektphase werden die Controller mit bereits aufgespieltem Code verschickt. Daher ist es nicht notwendig, Schritte zu unternehmen, um den Code auf die Controller zu laden._

Damit der Controller in Betrieb genommen werden kann, sind folgende Schritte notwendig:

1. Nanoleafs verbinden: Bevor der Controller selbst eingerichtet wird, sollten zuerst die Nanoleafs mit dem Netzwerk verbunden werden. Dies ist notwendig, da der Controller automatisiert nach der IP-Adresse der Nanoleafs sucht. Diese kann nur gefunden werden, wenn die Nanoleafs bereits mit dem Netzwerk verbunden sind.

2. Erstkonfiguration über Captive Portal:

   - Bei der ersten Inbetriebnahme öffnet sich ein _Captive Portal_ zur Konfiguration des Systems. Hier müssen folgende Informationen konfiguriert werden:
     - WLAN SSID
     - WLAN Passwort
     - Gruppen-ID: Diese wird verwendet, um Freundesgruppen zu identifizieren.
     - Name
     - Freundes-ID: Diese wird verwendet, um einen einzelnen Freund zu identifizieren. Wichtig ist hierbei, dass dieselbe Freundes-ID verwendet wird wie später in der App.

3. Netzwerkverbindung und Token-Authentifizierung:

   - Wurden die Netzwerkdaten korrekt eingegeben, verbindet sich der Controller mit dem Netzwerk. Sollte dies nicht der Fall sein, wird das Captive Portal automatisch wieder geöffnet.
   - Der Controller sucht im Netzwerk nach der IP-Adresse der Nanoleafs. Sobald diese gefunden wurde, benötigt der Controller einen Token zur Authentifizierung. Damit die Nanoleaf API einen solchen Token generieren kann, muss das Zeitfenster zur Authentifizierung aktiviert werden.
   - Benutzerinteraktion: Der Controller signalisiert durch schnelles Blinken seiner LED, dass eine Benutzerinteraktion erforderlich ist. Die Authentifizierung kann auf zwei Arten durchgeführt werden:
     1. In der Nanoleaf App auf der Einstellungsseite der entsprechenden Nanoleafs den Button "Stellen Sie eine Verbindung zur API her" betätigen.
     2. Den Power-Button der Nanoleafs etwa 5 Sekunden gedrückt halten, bis die Lichter des Nanoleaf Controllers anfangen zu blinken.

4. Abschluss des Setups:
   - Wenn die Authentifizierung erfolgreich war, sollten die Nanoleafs kurz aufleuchten. Der Controller startet daraufhin einmal neu. Solange der Controller mit Strom und Internet versorgt wird, können verbundene Freunde ihre Farben an die Nanoleafs senden. Das Setup ist somit abgeschlossen.

## FAQ / Fehlerbehebung

- Was tun, wenn der Controller sich nicht mit dem WLAN verbindet?

  - Stellen Sie sicher, dass die eingegebenen WLAN-Daten korrekt sind und versuchen Sie es erneut.

- Der Controller findet die IP-Adresse der Nanoleafs nicht.
  - Stellen Sie sicher, dass die Nanoleafs korrekt mit dem Netzwerk verbunden sind. Ein Neustart der Geräte kann in einigen Fällen helfen.

# PalPalette Controller

The PalPalette Controller is responsible for establishing the connection between the PalPalette App and the Nanoleafs. The API provided by Nanoleaf is accessible only locally. Therefore, each friend needs a PalPalette Controller that is on the same network as the Nanoleafs.

The controller takes on several tasks:

1. Querying Layout Data: The layout data of the Nanoleafs is queried. If there is a change in the layout, this change is forwarded to the backend with minimal delay.
2. Ping to the Backend: A ping is sent to the backend at regular intervals to signal that the controller, and thus the friend, is reachable.
3. Color Transmission via MQTT: The controller receives the colors sent by friends through MQTT. These are processed by the controller and forwarded to the Nanoleaf API to illuminate the panels.

## Setting Up the PalPalette Controller

_In the current phase of the project, the controllers are shipped with pre-installed code. Therefore, it is not necessary to take steps to load the code onto the controllers._

To get the controller operational, the following steps are necessary:

1. Connect Nanoleafs: Before the controller itself is set up, the Nanoleafs should first be connected to the network. This is necessary as the controller automatically searches for the IP address of the Nanoleafs. This can only be found if the Nanoleafs are already connected to the network.

2. Initial Configuration via Captive Portal:

   - When powered on for the first time, a _Captive Portal_ opens for system configuration. The following information must be configured:
     - WLAN SSID
     - WLAN Password
     - Group ID: This is used to identify friend groups.
     - Name
     - Friend ID: This is used to identify an individual friend. It is important that the same Friend ID is used as later in the app.

3. Network Connection and Token Authentication:

   - Once the network data is entered correctly, the controller connects to the network. If this does not occur, the Captive Portal will automatically reopen.
   - The controller searches the network for the IP address of the Nanoleafs. Once found, the controller requires a token for authentication. To generate such a token, the authentication window must be activated.
   - User Interaction: The controller signals a required user interaction by blinking its LED rapidly. Authentication can be performed in two ways:
     1. In the Nanoleaf App, on the settings page of the respective Nanoleafs, press the button "Establish a connection to the API."
     2. Press and hold the power button of the Nanoleafs for about 5 seconds, until the lights of the Nanoleaf Controller start blinking.

4. Completion of Setup:
   - If authentication is successful, the Nanoleafs should briefly light up. The controller will then restart once. As long as the controller is supplied with power and internet, connected friends can send their colors to the Nanoleafs. The setup is thus complete.

## FAQ / Troubleshooting

- What to do if the controller does not connect to the WLAN?

  - Ensure that the entered WLAN details are correct and try again.

- The controller cannot find the IP address of the Nanoleafs.
  - Ensure that the Nanoleafs are correctly connected to the network. A restart of the devices can help in some cases.
