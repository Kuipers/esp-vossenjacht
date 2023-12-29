# ESP Vossenjacht

Dit spel is een vossenjacht voor scouts die met behulp van hun smartphones opzoek gaan naar wifi punten in veld.

Compile de code voor een `Generic ESP2866 Module` met behulp van de Arduino IDE.

Om deze code te compilen heb je de volgende libraries nodig:
- Arduinojson
- Ticker

Als de code successvol compiled is kan deze op een ESP2866 geflashed worden.
Na het opniew starten van de ESP kan je contact maken met het WiFi accesspoint dat de ESP uitzend.
Als er succesvol verbinding is gemaakt met de vos dan opent er automatisch een pagina.

# Vragen en antwoorden

De vos kan geconfigureerd worden door een JSON bericht.
Installeer de `rest client` in `vscode` zodat je de http file kunt gebruiken.

Simple:
```http
POST http://1.2.3.4/upadte HTTP/1.1
content-type: application/json

{
	"ssid": "vos 1",
	"led": true,
	"interval": 0,
    "question": "In welk jaar is scouting Riel opgericht?</br>A. 1985</br>B. 2020</br>C. 1986",
    "answer": "a",
    "code": "Onthoud deze code: 5"
}
```

Moeilijker:
```http
POST http://1.2.3.4/update HTTP/1.1
content-type: application/json

{
	"ssid": "vos 1",
	"led": true,
	"accept_wrong": true,
	"interval": 0,
	"question": "Wat is de call van de Scouting Bolstergroep Riel?</br>A. PA1BVH/J</br>B. PI4SBR/J</br>C. PG2SGR/J",
	"answer": "b",
	"code": "Code: 5, loop via het voetbalpaadje naar de Heisteeg.",
	"code_wrong": "Code: 7, loop via het voetbalpaadje naar de Heisteeg."
}
```

De opties:

| Optie | Waarde | Beschrijving |
| --- | --- | --- |
| ssid | string | De naam van het WiFi netwerk dat de vos uitzend |
| led | boolean | De led op de vos aan of uit |
| accept_wrong | boolean | Als deze op true staat dan wordt een verkeerd antwoord ook geaccepteerd als OK. Maar wordt het `code_wrong: ...` getoond |
| interval | int | De tijd in seconden tussen het uitzenden van het wifi signaal. Dit maakt het moeilijker te vinden |
| question | string | De vraag die de scouts moeten beantwoorden |
| answer | string | Het juiste antwoord op de vraag. De vraag is hoofdletter ongevoelig en spaties worden verwijderd aan het begin en eind |
| code | string | De code die de scouts moeten onthouden als het antwoord correct is |
| code_wrong | string | De code die de scouts moeten onthouden als het antwoord fout is en `accept_wrong` aan staat |

Als je de rest-client extensie hebt geinstalleerd in vscode dan kan je de `http` file gebruiken om de vos te configureren.
Middels `ctrl+alt+r` kan je de json berichten versturen naar de vos.