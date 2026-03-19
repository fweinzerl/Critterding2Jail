# Roadmap: Critterding2 → 3D Raeuber-Beute-ALife

Ziel: Ein stabiles, emergentes Oekosystem mit Charakter, getrieben durch
Red-Queen-Raeuber-Beute-Dynamik, in 3D, ohne dass das System in triviale
Loesungen oder Extinktion kippt.

Wichtiger Fokus aus der Praxis: Wenn nach 10 Minuten kaum sichtbare Dynamik entsteht oder Bedienung frustriert, ist zuerst Beobachtbarkeit und Steuerbarkeit der Simulation der Engpass.

## 1) Vision
- Langfristig stabile Koexistenz von Raeubern und Beute.
- Sichtbare, interessante Rollen und Raumtaktiken statt rein numerischer Optimierung.
- Weiterentwicklung auf Basis der vorhandenen Simulation, nicht gegen sie.

## 2) Leitprinzipien
### P1: Nicht alles gleichzeitig frei machen
- Verhalten variiert schnell.
- Koerperparameter variieren langsam.
- Bewegungsprimitiven/Gaits bleiben anfangs fix oder sehr stabil.

### P2: Entkopplung ueber Morphology-Schnittstelle
- Erst Archetypen (z. B. Biped/Quadruped/Hopper) plus Parameter.
- Keine fruehen freien Topologie-Mutationen.

### P3: Druck durch Co-Evolution und unperfekte Information
- Occlusion, Noise, Deckung und energetische Kosten sind Pflicht.
- So entstehen robuste statt triviale Strategien.

## 3) Architektur-Ziele mit minimaler Einschneidung
- Species-Schicht als duenne Parametrisierung (mindestens Raeuber/Beute).
- Additive Systeme fuer Umwelt/Sensorik statt Rewrite zentraler Kernteile.
- Action-Space frueh begrenzen (Steering + Speed-Intent), damit die Dynamik stabil lernbar bleibt.

## 4) Iterationen (priorisiert)
### Iteration 0: Sichtbar + steuerbar (sofort)
Ziel: In 1-3 Minuten erkennbar, ob die Simulation lebt.
- Reproduzierbarkeit ueber Seeds und einfache Run-Logs.

Abnahme:
- Ein Run laesst sich reproduzierbar starten.

### Iteration 1: Minimales Oekosystem
Ziel: Erste nichttriviale Raeuber-Beute-Dynamik mit vorhandenem Unterbau.
- 2 Species (R/B), zunaechst nur parametergetrieben.
- Unterschiedliche Metabolismus-, Reproduktions- und Bewegungskosten.
- Angriff mit klaren Bedingungen und deutlichen Energiekosten.

Abnahme:
- Beide Species interagieren funktional.
- Keine sofortige Dauer-Extinktion in typischen Seeds.

### Iteration 2: Umwelt als Selektionsdruck
Ziel: 3D-Raum wird taktisch relevant.
- Deckung/Obstacles zur Sichtlinien-Unterbrechung.
- Resource-Patches statt homogener Ressourcenverteilung.
- Refugien/Gefahrenzonen als Stabilitaetshebel.

Abnahme:
- Sichtbare raeumliche Muster (z. B. Flucht-zu-Deckung, Patrouillen, Ambush).

### Iteration 3: Sensorik + begrenzter Action-Space
Ziel: Wahrnehmung spielnah, nicht perfekt.
- Sichtkegel mit grober Distanz-/Groesseninformation.
- Selbstzustand (Energie, Geschwindigkeit, Bodenkontakt, Neigung).
- Occlusion + Noise als Standard.
- Actions: Steering + Speed-Intent, optional diskrete Spezialaktionen.

Abnahme:
- Verhalten reagiert robust auf unvollstaendige/noisy Information.
- Kein Zwang zu fragiler Gelenk-Mikrosteuerung.

### Iteration 4: Stabilisierung und kontrollierte Oeffnung
Ziel: Variation erhoehen, ohne das System zu zerreissen.
- Konservative Mutation und Soft Constraints.
- Extinktionspuffer: Reproduktionsschwellen, Cooldowns, Resource-Regeneration.
- Erst spaeter neue Sensoren/Gaits/Koerperparameter als seltene Innovation.

Abnahme:
- Langer Sim-Betrieb mit Oszillation statt Dauerkollaps.
- Messbar steigende Verhaltensdiversitaet.

## 5) Risikoeinschaetzung
- Sicher machbar: Iteration 0-2 (bei konsequentem Scope).
- Unklar: Sensorik/Occlusion-Aufwand je nach bestehender Daten- und Render-Pipeline.
- Hohes Risiko: Zu fruehe Oeffnung von Gaits/Morphologie/Topologie.

## 6) Qualitätskriterien
### Oekosystem-Stabilitaet
- Raeuber und Beute ueberleben ueber lange Zeitraeume.
- Populationen oszillieren, statt haeufig zu kollabieren.

### Emergenz/Charakter
- Erkennbare Rollen (z. B. Sprinter, Ambusher, Herdentiere).
- Raumtaktiken sind regelmaessig beobachtbar.

### Nicht-triviale Loesungen
- Keine dominante Degeneration (z. B. passives Ausnutzen von Reproduktionsluecken).
- Kein permanenter Superraeuber-Zustand als Standardregime.

## 7) Minimaler MVP
1) Iteration-0-Rest: Seed-Repro + einfache Run-Logs.
2) 2 Species auf einem einfachen, stabilen Koerper-Archetyp.
3) Deckung + Resource-Patches.
4) Sensorik: Sichtkegel + Kern-Selbstzustand.
5) Actions: Steering + Speed-Intent.
6) Teurer, klar definierter Angriff.

Erst wenn dieser MVP stabil schwingt, folgen Gaits, Archetypen-Erweiterung und
tiefere Morphologie.

---

## 8) Beobachtbarkeit und Tooling

Diese Aufgaben sind Voraussetzung fuer produktives Arbeiten an der Simulation
und sollten parallel zu den Iterationen umgesetzt werden.

### Control Panels vereinheitlichen

Es gibt zwei separate `CdControlPanel`-Implementierungen:

- `src/plugins/be_plugin_app_critterding/control_panel.*` (Single-Thread)
- `src/plugins/be_plugin_app_critterding_threads/control_panel.*` (Multi-Thread)

Zielarchitektur:
1. Ein gemeinsames Panel mit einem schmalen Datenadapter.
2. Zwei Provider: `SingleThreadProvider`, `ThreadsProvider`.
3. Ein UI-Builder, ein Update-Loop.

Abnahme:
- Ein UI-Codepfad fuer Layout und Live-Updates.
- Provider ist die einzige Stelle, die Single-/Multi-Thread-Unterschiede kennt.
- Beide Startmodi funktionieren weiterhin.

### Life/Death-Graphen im Life Stats Panel

Scope (MVP):
- Leichtgewichtige Zeitreihen-Charts in `CdLifeStatsPanel`.
- Erste Kurven: Population (Kritter + Futter), Events (Births/Deaths pro Sekunde).
- Feste Ring-Buffer (z. B. 300 Samples), Update bei Panel-Refresh-Rate.

Spaeter:
- Smoothing-Toggle, zusaetzliche Trends (avg Distanz, Energie-Verteilung).
- Auto-Scale / Fixed-Scale Umschalter.

Abnahme:
- `F4` oeffnet ohne Absturz in beiden Runtimes.
- Graphen aktualisieren sich kontinuierlich.
- Keine schreibbaren Controls im Life Stats Panel.
