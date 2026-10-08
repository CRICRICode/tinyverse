# Tinyverse [![Download Zip](https://img.shields.io/badge/download-v0.1.0-blue)](https://github.com/CRICRICode/tinyverse/releases/download/v1.0.0/Tinyverse.zip)

**Tinyverse** è un platform 3D in terza persona ambientato su piccoli pianeti, sviluppato principalmente in **C++ con Unreal Engine 5.8** per il progetto di GameEngine. Il giocatore esplora superfici sferiche, salta tra pianeti e piattaforme mobili, raccoglie monete e affronta nemici che pattugliano, inseguono e attaccano con una carica.

Il progetto mette insieme locomozione con gravità locale, telecamera adattata alla superficie, combattimento, interfaccia, persistenza e audio. La logica dei sistemi principali è in C++; i Blueprint collegano questi sistemi ad animazioni, asset, Behavior Tree, menu e configurazione dei livelli. Il sistema di **reflection di Unreal** costituisce anche il collegamento con l'esame di **HighLevel**.

## Indice

- [Panoramica e meccaniche di gioco](#panoramica-e-meccaniche-di-gioco)
- [Controlli](#controlli)
- [Gravità planetaria e movimento](#gravità-planetaria-e-movimento)
- [Telecamera e animazioni](#telecamera-e-animazioni)
- [Nemici e intelligenza artificiale](#nemici-e-intelligenza-artificiale)
- [Salute, danni e monete](#salute-danni-e-monete)
- [Piattaforme dinamiche](#piattaforme-dinamiche)
- [Interfaccia e flusso di gioco](#interfaccia-e-flusso-di-gioco)
- [Salvataggi e persistenza del mondo](#salvataggi-e-persistenza-del-mondo)
- [Audio e integrazione FMOD](#audio-e-integrazione-fmod)
- [Livelli e contenuti visivi](#livelli-e-contenuti-visivi)
- [Architettura e struttura della repository](#architettura-e-struttura-della-repository)
- [Reflection e collegamento con HighLevel](#reflection-e-collegamento-con-highlevel)
- [Requisiti, installazione e avvio](#requisiti-installazione-e-avvio)
- [Configurazione e strumenti di sviluppo](#configurazione-e-strumenti-di-sviluppo)
- [Percorso di dimostrazione](#percorso-di-dimostrazione)
- [Licenza e contenuti di terze parti](#licenza-e-contenuti-di-terze-parti)

## Panoramica e meccaniche di gioco

Il nucleo del gioco è l'esplorazione di un ambiente composto da più pianeti. La direzione di caduta cambia con il pianeta attivo: muoversi sulla parte inferiore di una sfera continua a significare camminare sulla sua superficie, con personaggio e telecamera orientati rispetto alla gravità locale.

Le meccaniche sviluppate per Tinyverse comprendono:

- **Esplorazione planetaria:** movimento tangente alla superficie, salto, controllo in aria e passaggio tra zone di influenza gravitazionale.
- **Telecamera in terza persona:** rotazione manuale, allineamento alla normale locale e ricentramento automatico configurabile.
- **Nemici sulla superficie:** pattugliamento, inversione di direzione, rilevamento e inseguimento del giocatore.
- **Attacco a carica:** preparazione, scatto e recupero, con velocità, durata e danno configurabili.
- **Stomp:** il giocatore può colpire il trigger superiore del nemico, infliggere danno e rimbalzare verso l'alto locale.
- **Danni e respinta:** il contatto con il nemico riduce la salute e applica knockback, con un intervallo di invulnerabilità dopo il colpo.
- **Monete e progressione della salute:** raccolta, contatore e ricompensa periodica che cura oppure aumenta la salute massima.
- **Piattaforme mobili:** movimento tra due estremi, inversione del percorso e sosta configurabile.
- **HUD e menu:** cuori, contatore monete, menu principale, pausa, salvataggio manuale e menu di morte.
- **Persistenza:** ripristino del giocatore e conservazione della rimozione di monete raccolte e nemici sconfitti.
- **Feedback sonoro:** passi, salto, raccolta, ricompensa salute, sconfitta dei nemici e selezione dei pulsanti tramite FMOD.

## Controlli

I controlli usano **Enhanced Input**. Le associazioni sono contenute in `Content/Input/IMC_Default.uasset` e `Content/Input/IMC_MouseLook.uasset`.

| Azione | Tastiera e mouse | Gamepad |
| --- | --- | --- |
| Movimento | `W`, `A`, `S`, `D` oppure frecce direzionali | Stick sinistro |
| Orientamento della telecamera | Movimento del mouse | Stick destro |
| Salto | `Spazio` | Pulsante inferiore del gruppo frontale, ad esempio A / Croce |
| Pausa | `Esc` | Pulsante Menu / Start |
| Interazione con i menu | Cursore e pulsanti dell'interfaccia | Secondo la navigazione configurata nei widget |

Le Input Action del gameplay sono `IA_PlanetMove`, `IA_Jump`, `IA_Look`, `IA_MouseLook` e `IA_Pause`. Il salvataggio manuale è accessibile dal menu di pausa.

## Gravità planetaria e movimento

### Pianeti come sorgenti di gravità

[`ATinyverseGravityPlanet`](Source/Tinyverse/TinyverseGravityPlanet.h) è un Actor che definisce una sorgente gravitazionale. Ogni pianeta possiede un volume sferico di influenza e parametri esposti all'editor:

- raggio fisico, impostato manualmente oppure ricavato dai bounds dei componenti primitivi;
- accelerazione alla superficie, `SurfaceGravity`;
- raggio di influenza, `InfluenceRadius`;
- attenuazione opzionale della gravità con la distanza, `bUseGravityFalloff`;
- esponente di attenuazione, `GravityFalloffExponent`.

La direzione della gravità in una posizione è il vettore normalizzato che punta verso il centro del pianeta. Se è attivo il falloff, l'intensità segue questa relazione:

```text
g = SurfaceGravity × (PlanetRadius / max(DistanceFromCenter, PlanetRadius)) ^ GravityFalloffExponent
```

Con esponente `2` si ottiene un andamento inversamente proporzionale al quadrato della distanza; con falloff disattivato l'accelerazione resta costante all'interno del volume. Fuori dall'influenza il pianeta restituisce intensità zero.

### Selezione del pianeta attivo

[`ATinyverseCharacter`](Source/Tinyverse/TinyverseCharacter.cpp) individua le sorgenti che possono influenzarlo e sceglie una gravità attiva. La selezione usa la distanza dalla superficie rapportata alla forza gravitazionale locale:

```text
SelectionScore = max(0, DistanceFromCenter - PlanetRadius) / LocalGravityStrength
```

Il candidato con punteggio minore è preferito. Il comportamento comprende alcuni accorgimenti:

1. Il pianeta su cui il personaggio è effettivamente appoggiato ha precedenza, quando il floor hit identifica un `ATinyverseGravityPlanet` valido.
2. `GravitySwitchRatio` introduce isteresi: una nuova sorgente deve essere abbastanza migliore di quella attiva prima di sostituirla. Il valore C++ iniziale è `1.15`.
3. La lista dei candidati viene ricostruita durante l'aggiornamento, oltre alla registrazione tramite overlap, per evitare che un evento di collisione mancato lasci il personaggio senza una sorgente valida.
4. La nuova direzione fisica è applicata subito; l'orientamento visivo del personaggio e della telecamera converge gradualmente.

Il sistema sceglie una sorgente alla volta. Quando nessun pianeta valido influenza il personaggio, imposta la scala gravitazionale a zero.

### Movimento sulla superficie

La locomozione usa `UCharacterMovementComponent`, a cui viene assegnata la direzione personalizzata tramite `SetGravityDirection`. L'intensità locale viene tradotta in `GravityScale` rispetto alla gravità del mondo.

Il movimento non assume un asse Z globale: la direzione avanti della telecamera viene proiettata sul piano tangente alla superficie e la direzione destra viene ricavata con un prodotto vettoriale. Gli input producono quindi movimento coerente con la visuale anche mentre cambia l'orientamento del pianeta.

L'orientamento del personaggio segue la direzione desiderata di movimento attraverso interpolazione di quaternioni. Il salto sfrutta il movimento del Character con la gravità corrente e mantiene il controllo in aria configurato nel componente.

## Telecamera e animazioni

La telecamera è costruita con un **Spring Arm** e una **Camera Component**. Il braccio gestisce anche l'avvicinamento della telecamera quando incontra ostacoli.

Il frame della camera viene riallineato alla direzione `LocalUp`, opposta alla gravità locale. Il cambio di normale viene trasferito al vettore avanti della camera tramite quaternioni, poi il vettore viene riportato sul piano tangente. Questo permette di seguire la superficie senza dipendere dalla verticale globale.

Le opzioni esposte comprendono sensibilità orizzontale e verticale, inversione degli assi, limiti del pitch, inclinazione iniziale, altezza del target e comportamento automatico. Dopo un intervallo senza input manuale, la camera può tornare dietro al personaggio e recuperare il pitch preferito.

Il ricentramento automatico dello yaw viene sospeso quando il giocatore si muove all'indietro; al rilascio di quell'input il ritardo viene riavviato. Questo accorgimento evita rotazioni automatiche indesiderate durante la retromarcia.

[`UTinyverseAnimationLibrary`](Source/Tinyverse/TinyverseAnimationLibrary.cpp) espone ai Blueprint `CalculatePlanetDirection`, che ricava l'angolo della velocità tangente rispetto agli assi locali del personaggio. Le animazioni possono così utilizzare una direzione coerente con la locomozione planetaria.

[`UAnimNotify_TinyverseFootstep`](Source/Tinyverse/AnimNotify_TinyverseFootstep.cpp) sincronizza i suoni dei passi con le animazioni. Il notify può indicare un socket o un osso del piede; il Character usa quella posizione per riprodurre il suono, purché sia a terra e superi la velocità minima configurata.

## Nemici e intelligenza artificiale

### Nemico planetario

[`ATinyverseEnemy`](Source/Tinyverse/TinyverseEnemy.h) è un Character con componente salute, trigger di stomp, trigger di danno e componente di identità persistente. Nei Blueprint sono presenti `BP_Enemy` e `BP_Boss`, entrambi basati sulla classe nemico C++.

Il nemico viene associato a un pianeta attraverso `GravityPlanet`. Aggiorna la propria gravità verso il centro di quell'Actor e mantiene l'orientamento tangente alla superficie. La sua associazione al pianeta è configurata per istanza; la selezione automatica tra più sorgenti è implementata nel giocatore.

Il pattugliamento usa una direzione tangente e una rotazione interpolata. Il metodo `ReversePatrolDirection` consente ai task AI di invertire il verso. Quando il bersaglio viene seguito, velocità e orientamento passano ai parametri di tracking.

### Behavior Tree e Blackboard

[`ATinyverseEnemyAIController`](Source/Tinyverse/TinyverseEnemyAIController.cpp) avvia il Behavior Tree assegnato quando prende possesso di un nemico valido. Gli asset specifici di Tinyverse si trovano in `Content/TinyPlanet/Blueprints/AI/`:

| Asset | Responsabilità |
| --- | --- |
| `BT_Enemy` | Organizza i comportamenti di pattuglia, inseguimento e carica |
| `BB_Enemy` | Conserva il bersaglio `TargetActor` e la condizione `CanCharge` |
| `BTS_FindChargeTarget` | Aggiorna il bersaglio e la possibilità di caricare in base alle distanze |
| `BTT_PatrolPlanet` | Richiama il movimento di pattuglia e l'inversione di direzione |
| `BTT_TrackTargetPlanet` | Richiama il tracking tangente e il suo arresto |
| `BTT_ChargePlanet` | Avvia, aggiorna e annulla la carica attraverso le funzioni C++ |
| `BP_EnemyAIController` | Configura il controller e l'asset di comportamento |

Il rilevamento e la perdita del bersaglio usano raggi distinti, evitando di perdere immediatamente un giocatore che si allontana leggermente dalla soglia di rilevamento.

### Fasi della carica

L'enumerazione riflessa `ETinyverseEnemyChargePhase` descrive quattro fasi:

| Fase | Comportamento |
| --- | --- |
| `Inactive` | Nessuna carica attiva; sono disponibili gli altri comportamenti |
| `Windup` | Il nemico si ferma e si orienta verso il bersaglio |
| `Charging` | Avanza nella direzione fissata all'inizio dello scatto, mantenendola tangente alla superficie |
| `Recovery` | Si ferma per il tempo di recupero prima di tornare disponibile |

Durante lo scatto la direzione viene adattata alla curvatura, ma non viene continuamente ricalcolata per puntare il giocatore. Il contatto con il giocatore o uno stomp durante una carica porta il nemico in recupero. La morte annulla la carica e rimuove il nemico.

### Stomp, danno da contatto e knockback

Lo **stomp** è gestito dal trigger superiore del nemico. Quando il giocatore vi entra, riceve una velocità di rimbalzo verso l'alto locale; la componente tangente della sua velocità viene mantenuta. Al nemico viene applicato `StompDamage` tramite il sistema danni di Unreal.

Il trigger di contatto applica `ContactDamage`, oppure `ChargeDamage` durante `Charging`. Se il giocatore è già nel trigger di stomp, quel contatto non infligge danno. La respinta viene applicata solo quando la salute del giocatore è effettivamente diminuita: combina una direzione tangente che lo allontana dal nemico e un impulso verso l'alto locale.

I parametri di movimento, rilevamento, danno, rimbalzo e recupero possono essere modificati nell'editor, permettendo di ottenere nemici con comportamenti e difficoltà differenti senza cambiare la classe base.

## Salute, danni e monete

### Componente salute riutilizzabile

[`UTinyverseHealthComponent`](Source/Tinyverse/TinyverseHealthComponent.h) è un Actor Component utilizzato dal giocatore e dai nemici. Gestisce salute corrente, salute massima, limite massimo, guarigione, ricompense e invulnerabilità temporanea.

Il componente si collega a `OnTakeAnyDamage` del proprietario. Ignora danni non positivi, danni su un soggetto già morto e danni ricevuti durante il cooldown. Quando la salute cambia invia `OnHealthChanged`; quando raggiunge zero invia `OnDeath`. HUD, menu e logica del nemico possono reagire a questi eventi.

### Raccolta e ricompense

La moneta è implementata nel Blueprint `BP_Coin`. Il flusso di raccolta richiama `CollectCoin` del Character, aggiorna il contatore tramite `OnCoinCountChanged`, riproduce il suono FMOD e usa l'identità persistente per registrare la rimozione dell'oggetto.

Ogni `CoinsPerHealthReward` monete, con valore C++ iniziale **10**, viene richiesta una ricompensa di un punto salute:

| Stato del giocatore | Risultato della ricompensa |
| --- | --- |
| Salute corrente inferiore al massimo | Recupera un punto, fino al massimo corrente |
| Salute piena e massimo inferiore al limite | Aumenta di un punto sia il massimo sia la salute corrente |
| Salute piena e massimo già al limite | Il contatore aumenta, senza ulteriori punti salute |
| Giocatore morto | La ricompensa non lo rianima |

Il contatore è cumulativo: la ricompensa viene verificata a ogni multiplo della soglia. Il suono di aumento salute viene riprodotto solo se la ricompensa ha prodotto un cambiamento effettivo.

### Valori iniziali nel codice

Questi valori sono i **default C++**; Blueprint e istanze dei livelli possono modificarli.

| Parametro | Default | Dove si configura |
| --- | --- | --- |
| Salute massima iniziale | `3` | `TinyverseHealthComponent` |
| Limite della salute massima | `5` | `TinyverseHealthComponent` |
| Invulnerabilità dopo un danno | `0.75 s` | `TinyverseHealthComponent` |
| Monete per ricompensa | `10` | `TinyverseCharacter` |
| Velocità massima del giocatore | `500 cm/s` | Character Movement |
| Velocità iniziale di salto | `500 cm/s` | Character Movement |
| Accelerazione gravitazionale alla superficie | `980 cm/s²` | `TinyverseGravityPlanet` |
| Raggio di influenza del pianeta | `3000 cm` | `TinyverseGravityPlanet` |
| Velocità di pattuglia / inseguimento / carica | `150 / 250 / 500 cm/s` | `TinyverseEnemy` |
| Raggio di rilevamento / perdita / attivazione carica | `800 / 1000 / 300 cm` | `TinyverseEnemy` |
| Durata di preparazione / scatto / recupero | `0.35 / 0.8 / 0.6 s` | `TinyverseEnemy` |
| Danno da contatto / carica / stomp | `1 / 1 / 1` | `TinyverseEnemy` |
| Velocità di rimbalzo dello stomp | `600 cm/s` | `TinyverseEnemy` |

## Piattaforme dinamiche

[`ATinyverseDynamicPlatform`](Source/Tinyverse/TinyverseDynamicPlatform.cpp) controlla una Static Mesh mobile. All'avvio memorizza la posizione iniziale e ricava l'estremo finale da `MovementOffset`, trasformato secondo l'orientamento dell'Actor senza applicare la sua scala.

La piattaforma interpola la posizione a velocità costante con `VInterpConstantTo`. Arrivata a un estremo, sospende il Tick e avvia un timer; al termine della sosta inverte il verso e riprende il movimento.

`MovementOffset` è esposto con `MakeEditWidget`, così il percorso può essere impostato direttamente nella viewport. `MovementSpeed` e `WaitDuration` controllano velocità e sosta; i default C++ sono `100 cm/s` e `5 s`. Il Blueprint `BP_DynamicPlatform` permette di inserire e configurare queste piattaforme nei livelli.

## Interfaccia e flusso di gioco

L'interfaccia usa **UMG**. I widget correnti sono in `Content/TinyPlanet/Blueprints/UI/`.

| Widget | Funzione |
| --- | --- |
| `WBP_MainMenu` | Nuova partita, continua ed uscita; sfondo video tramite Media Player |
| `WBP_GameMenu` | Menu di pausa, ripresa, salvataggio manuale e navigazione verso il menu |
| `WBP_DeathMenu` | Riprova dal salvataggio oppure torna al menu |
| `WBP_PlayerHUD` | Visualizza salute tramite cuori e numero di monete |
| `WBP_Button` | Pulsante riutilizzabile con etichetta, varianti visive ed evento di click |

I pulsanti hanno varianti **Primary**, **Secondary** e di uscita/distruttiva, con immagini distinte per stato normale, hover, pressione e disabilitato. I menu usano pannelli arrotondati semitrasparenti; il menu principale include uno sfondo animato.

Lo sfondo è integrato nel widget del menu attraverso il Media Player `MP_MainMenuBackground` e la Media Texture `MP_MainMenuBackground_Video`, presenti in `Content/TinyPlanet/2D/`. Il Blueprint del menu configura la riproduzione in loop.

Il menu verifica l'esistenza del salvataggio per rendere disponibile **Continue** e gestisce l'avvio di una nuova partita. La morte del giocatore viene collegata al menu di morte nel Blueprint del personaggio.

La logica Blueprint del controller utilizzato da Tinyverse gestisce apertura e chiusura della pausa, cursore, modalità di input e autosalvataggio periodico. Il ritorno al gameplay ripristina cattura del mouse e focus della viewport. Il controller C++ configura i Mapping Context di Enhanced Input.

Il HUD reagisce a `OnHealthChanged` e `OnCoinCountChanged`, collegando gli eventi dei sistemi C++ ai widget Blueprint.

## Salvataggi e persistenza del mondo

### Dati salvati

[`UTinyverseSaveGame`](Source/Tinyverse/TinyverseSaveGame.h) raccoglie i dati della partita in un oggetto `USaveGame`:

| Campo | Contenuto |
| --- | --- |
| `SaveVersion` | Versione del formato; attualmente `2` |
| `SavedLevelPath` | Percorso del livello corrente, senza prefisso PIE |
| `PlayerTransform` | Posizione, rotazione e scala del giocatore |
| `CurrentHealth`, `MaxHealth` | Stato della salute del giocatore |
| `CoinCount` | Numero cumulativo di monete raccolte |
| `RemoveActorIds` | Insieme dei GUID degli Actor rimossi persistentemente |

[`UTinyverseSaveSubsystem`](Source/Tinyverse/TinyverseSaveSubsystem.cpp) è un `UGameInstanceSubsystem`: mantiene lo stato necessario anche durante il cambio di livello. Usa lo slot **`TinyverseProgress`** con indice utente `0` e le API `CreateSaveGameObject`, `SaveGameToSlot` e `LoadGameFromSlot`.

I flussi disponibili sono:

1. **Nuova partita:** cancella il precedente slot, azzera le rimozioni registrate, apre il livello scelto e crea un salvataggio iniziale quando il giocatore viene inizializzato.
2. **Salvataggio manuale:** acquisisce livello, transform, salute, monete e identità rimosse della sessione corrente.
3. **Autosalvataggio:** viene richiesto periodicamente dalla logica Blueprint del controller; il flusso è documentato nel Blueprint come salvataggio ogni cinque minuti.
4. **Continua:** carica i dati, ripristina prima l'insieme delle rimozioni, apre il livello salvato e applica lo stato del giocatore durante l'inizializzazione.
5. **Riprova:** il menu di morte richiama il caricamento della partita salvata.

I dati di caricamento vengono conservati in `PendingSave` e consumati quando il Character è pronto. Il ripristino controlla che il livello corrente corrisponda a quello salvato. I salvataggi con versione superiore a quella supportata vengono rifiutati; la presenza del campo versione non implica un sistema generale di migrazione dei formati.

Nell'ambiente di sviluppo Unreal i file `.sav` sono normalmente collocati in `Saved/SaveGames/`; per una build distribuita il percorso dipende dalla piattaforma. Vedi la [documentazione Epic sul salvataggio](https://dev.epicgames.com/documentation/unreal-engine/saving-and-loading-your-game-in-unreal-engine).

### Identità persistente degli Actor

[`UTinyverseSaveIdentityComponent`](Source/Tinyverse/TinyverseSaveIdentityComponent.h) assegna agli oggetti un `FGuid` persistente. Quando una moneta viene raccolta o un nemico sconfitto, `MarkOwnerAsRemoved` registra l'identificatore nel subsystem. Al caricamento l'oggetto può controllare se risulta già rimosso e non ricomparire.

La proprietà **Persist Removal Between Loads** permette di scegliere per singolo oggetto se la rimozione debba persistere. Disabilitandola, quell'oggetto può ricomparire quando il livello viene caricato nuovamente.

Nell'editor il componente genera gli identificatori mancanti e li rigenera dopo duplicazione ordinaria o importazione, per evitare che due istanze condividano la stessa identità. `RegeneratePersistentId`, esposta con `CallInEditor`, permette anche una rigenerazione manuale.

La persistenza del mondo riguarda gli **Actor marcati come rimossi**. Il salvataggio non registra una fotografia completa della simulazione: posizioni dei nemici ancora vivi, loro salute, fase della carica, avanzamento delle piattaforme e orientamento manuale della telecamera non sono campi del formato attuale.

## Audio e integrazione FMOD

Il modulo C++ dipende da **FMODStudio**. La repository include l'integrazione Unreal **FMOD 2.03.14**, i runtime del plugin, gli asset generati, le bank e il progetto audio modificabile [`FMODProject/Tinyverse_FMOD.fspro`](FMODProject/Tinyverse_FMOD.fspro).

| Evento FMOD | Quando viene riprodotto |
| --- | --- |
| `event:/SFX/Player/FootSteps` | Notify delle animazioni di locomozione, quando il giocatore si muove a terra |
| `event:/SFX/Player/Jump` | Avvio di un salto consentito |
| `event:/SFX/Collectible/Coin` | Raccolta di una moneta |
| `event:/SFX/Player/Health` | Ricompensa che recupera o aumenta effettivamente la salute |
| `event:/SFX/Enemy/Killed` | Sconfitta del nemico |
| `event:/SFX/Menu/Selection` | Hover dei pulsanti riutilizzabili |

Il Character permette di assegnare direttamente un `UFMODEvent` oppure di risolverlo a runtime attraverso il percorso dell'evento. I suoni di gameplay vengono riprodotti alla posizione appropriata; [`UTinyverseAudioLibrary`](Source/Tinyverse/TinyverseAudioLibrary.cpp) espone `PlayUIEventByPath` per gli eventi UI riprodotti in 2D.

Il progetto FMOD contiene cinque campioni per i passi e gli audio sorgente di salto, moneta e ricompensa salute. In `Content/FMOD/Desktop/` sono presenti `Master.bank`, `Master.strings.bank` e le bank `SFX_Player`, `SFX_Enemy`, `SFX_Menu` e `SFX_Ambient`. La presenza della bank ambientale non implica che tutte le categorie contengano già eventi utilizzati dal gameplay.

[`Config/DefaultGame.ini`](Config/DefaultGame.ini) include i percorsi degli asset FMOD da cuocere e configura `FMOD/Desktop` per lo staging delle bank durante il packaging.

Per modificare l'audio:

1. Aprire il progetto `.fspro` con una versione compatibile di FMOD Studio.
2. Verificare il percorso di output delle bank: quello salvato in `Metadata/Workspace.xml` dipende dalla disposizione delle cartelle sulla macchina di sviluppo e va adattato al proprio clone.
3. Impostare l'output su `Content/FMOD`, in modo da produrre le bank Desktop in `Content/FMOD/Desktop`.
4. Ricostruire le bank e aggiornare gli asset tramite l'integrazione FMOD nell'editor Unreal.
5. Conservare i percorsi degli eventi attesi dal codice, oppure aggiornare le proprietà esposte nei Blueprint.

## Livelli e contenuti visivi

| Livello | Ruolo |
| --- | --- |
| `Content/TinyPlanet/Maps/MenuMap.umap` | Mappa iniziale dell'editor e del gioco, configurata in `DefaultEngine.ini` |
| `Content/TinyPlanet/Maps/L_FirstLevel.umap` | Livello principale con più pianeti, monete, nemici, Blueprint boss e piattaforme mobili |
| `Content/TinyPlanet/Maps/L_GravityPrototype.umap` | Ambiente di prototipazione della gravità e delle meccaniche |

Il lavoro visivo include materiali per Terra, Marte, Luna, Venere, Nettuno, pianeti immaginari e superfici ghiacciate, con texture planetarie e mappe di dettaglio. Sono presenti HDRI spaziali, supporto **HDRI Backdrop**, Blueprint di illuminazione `BP_LightStudio` e media per lo sfondo del menu.

La configurazione del renderer abilita Lumen, Virtual Shadow Maps, mesh distance fields, ray tracing e Substrate; su Windows il RHI predefinito è DirectX 12 con Shader Model 6. Queste impostazioni descrivono la configurazione del progetto e non costituiscono un benchmark delle prestazioni.

## Architettura e struttura della repository

Il progetto contiene un modulo runtime, **Tinyverse**, e due target: `Tinyverse` per il gioco e `TinyverseEditor` per l'editor. Le dipendenze dei sistemi descritti comprendono Core, CoreUObject, Engine, InputCore, EnhancedInput, AIModule, UMG, Slate e FMODStudio e sono definite in [`Tinyverse.Build.cs`](Source/Tinyverse/Tinyverse.Build.cs).

La struttura seguente presenta i sorgenti e i contenuti utilizzati dai sistemi specifici di Tinyverse:

```text
Tinyverse/
├── Tinyverse.uproject
├── Config/                         Configurazione di motore, input e packaging
├── Source/
│   ├── Tinyverse.Target.cs
│   ├── TinyverseEditor.Target.cs
│   └── Tinyverse/
│       ├── TinyverseCharacter.*    Giocatore, gravità, camera, monete e audio
│       ├── TinyverseGravityPlanet.*
│       ├── TinyverseEnemy.*
│       ├── TinyverseEnemyAIController.*
│       ├── TinyverseHealthComponent.*
│       ├── TinyverseDynamicPlatform.*
│       ├── TinyverseSaveGame.h
│       ├── TinyverseSaveSubsystem.*
│       ├── TinyverseSaveIdentityComponent.*
│       ├── TinyverseAnimationLibrary.*
│       ├── TinyverseAudioLibrary.*
│       ├── AnimNotify_TinyverseFootstep.*
│       ├── TinyversePlayerController.*
│       └── TinyverseGameMode.*
├── Content/
│   ├── TinyPlanet/                 Livelli, Blueprint, AI, UI e materiali del gioco
│   ├── Input/                      Input Action e Mapping Context
│   ├── FMOD/                       Asset audio generati e bank Desktop
│   └── HDRIs/                      Sfondi e illuminazione dell'ambiente
├── FMODProject/                    Progetto FMOD, metadata e audio sorgente
└── Plugins/FMODStudio/             Integrazione FMOD e runtime inclusi
```

### Responsabilità dei sistemi

| Sistema | Implementazione principale | Collegamento con il resto del gioco |
| --- | --- | --- |
| Giocatore | `ATinyverseCharacter` | Legge input, usa pianeti, salute, camera, monete, animazioni e audio |
| Gravità | `ATinyverseGravityPlanet` | Fornisce direzione, intensità e punteggio di selezione |
| Nemici | `ATinyverseEnemy` | Espone operazioni utilizzate dai task del Behavior Tree |
| Salute | `UTinyverseHealthComponent` | Emette eventi di variazione e morte per HUD e gameplay |
| Persistenza | `UTinyverseSaveSubsystem` e `UTinyverseSaveGame` | Conserva i dati tra sessioni e cambi di livello |
| Identità degli oggetti | `UTinyverseSaveIdentityComponent` | Collega Actor del livello e GUID di rimozione |
| Piattaforme | `ATinyverseDynamicPlatform` | Espone percorso, velocità e tempi di attesa |
| Interfaccia | Widget e controller Blueprint | Coordina menu, input, HUD, pausa e richieste di salvataggio |
| Audio e animazione | Function Library e AnimNotify | Collegano gli eventi di gioco agli asset e alla riproduzione |

## Reflection e collegamento con HighLevel

Tinyverse usa il **sistema di reflection integrato in Unreal Engine** per rendere tipi, proprietà, funzioni ed eventi C++ disponibili al motore, all'editor e ai Blueprint. Le annotazioni sono elaborate da **Unreal Header Tool (UHT)** prima della compilazione C++; il codice generato integra i tipi nel sistema UObject. Gli header includono il rispettivo file `.generated.h` e le dichiarazioni riflesse usano `GENERATED_BODY()`.

Questo consente di associare dati e comportamento ai tipi e di utilizzarli tramite i sistemi del motore. Il progetto applica la reflection di Unreal; non implementa una propria libreria di reflection. Il funzionamento della generazione è descritto nella [documentazione ufficiale di UHT](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-header-tool-for-unreal-engine).

### 1. Tipi riflessi: UCLASS e UENUM

`UCLASS` registra classi del sistema UObject, come il Character, gli Actor planetari, i componenti e il SaveGame. `UENUM` rende riconoscibili le enumerazioni utilizzate dal gameplay.

Un esempio specifico di Tinyverse è l'enumerazione in [`TinyverseEnemy.h`](Source/Tinyverse/TinyverseEnemy.h):

```cpp
UENUM(BlueprintType)
enum class ETinyverseEnemyChargePhase : uint8
{
    Inactive,
    Windup,
    Charging,
    Recovery
};
```

`BlueprintType` permette di usare il tipo nei Blueprint. Lo stato corrente viene a sua volta esposto attraverso una proprietà riflessa. Nello stesso header `ATinyverseEnemy` è dichiarata `UCLASS(Abstract, Blueprintable)`, così la classe C++ funge da base per i Blueprint dei nemici.

### 2. Parametri di gameplay: UPROPERTY e metadati

`UPROPERTY` rende i membri riconoscibili dal sistema di proprietà Unreal. Gli specificatori determinano come vengono modificati, visualizzati ed esposti ai Blueprint.

Un esempio da [`TinyverseGravityPlanet.h`](Source/Tinyverse/TinyverseGravityPlanet.h):

```cpp
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gravity|Influence",
    meta=(ClampMin="1.0", Units="cm"))
float InfluenceRadius = 3000.0f;
```

L'esempio riporta gli specificatori principali della proprietà; nel sorgente è presente anche il relativo `ToolTip`.

Nel progetto vengono usati:

- `EditAnywhere`, `EditDefaultsOnly` e `EditInstanceOnly` per distinguere dove configurare un valore;
- `VisibleAnywhere` e `VisibleInstanceOnly` per mostrare componenti e stato senza renderli modificabili nello stesso modo;
- `BlueprintReadOnly` e `BlueprintReadWrite` per l'accesso dai grafi;
- `Category`, `ToolTip`, `Units`, `ClampMin` e `ClampMax` per organizzare e guidare l'editing;
- `EditCondition` e `EditConditionHides` per mostrare i parametri pertinenti, per esempio quelli del falloff;
- `MakeEditWidget` per modificare il vettore del percorso di una piattaforma nella viewport;
- `Transient` per identificare dati temporanei.

I casi concreti comprendono gravità e camera in [`TinyverseCharacter.h`](Source/Tinyverse/TinyverseCharacter.h), dimensioni e forza dei pianeti in [`TinyverseGravityPlanet.h`](Source/Tinyverse/TinyverseGravityPlanet.h), danni e AI in [`TinyverseEnemy.h`](Source/Tinyverse/TinyverseEnemy.h), salute in [`TinyverseHealthComponent.h`](Source/Tinyverse/TinyverseHealthComponent.h).

### 3. Funzioni ed eventi accessibili ai Blueprint

`UFUNCTION(BlueprintCallable)` espone operazioni C++ come nodi eseguibili. `BlueprintPure` espone funzioni di lettura adatte a essere usate senza pin di esecuzione.

```cpp
// TinyverseCharacter.h
UFUNCTION(BlueprintCallable, Category="Collectibles")
void CollectCoin();

// TinyverseHealthComponent.h
UFUNCTION(BlueprintPure, Category="Health")
float GetCurrentHealth() const;
```

La moneta può quindi richiamare il Character senza duplicare nei Blueprint la logica di conteggio e ricompensa. Allo stesso modo i task AI richiamano `StartCharge`, `UpdateCharge`, `MoveAlongPlanet` e `TrackTargetAlongPlanet`, mentre i menu richiamano le funzioni del SaveSubsystem.

### 4. Delegati dinamici e callback

Il componente salute dichiara delegati `DECLARE_DYNAMIC_MULTICAST_DELEGATE_*` e li espone con `UPROPERTY(BlueprintAssignable)`. I Blueprint possono collegare più risposte a `OnHealthChanged` e `OnDeath`; il Character usa lo stesso schema per `OnCoinCountChanged`.

Il flusso osservabile è:

```text
Danno Unreal → HealthComponent → OnHealthChanged → aggiornamento HUD
                              → OnDeath → risposta del nemico o menu di morte
Raccolta moneta → Character → OnCoinCountChanged → contatore HUD
```

Anche gli eventi di overlap usano callback riconosciute dal sistema riflesso. In [`TinyverseGravityPlanet.cpp`](Source/Tinyverse/TinyverseGravityPlanet.cpp), per esempio:

```cpp
GravityVolume->OnComponentBeginOverlap.AddDynamic(
    this,
    &ATinyverseGravityPlanet::HandleGravityVolumeBeginOverlap);
```

Il metodo corrispondente è dichiarato `UFUNCTION()` nell'header. Il componente salute usa `AddUniqueDynamic` per `HandleOwnerTakeAnyDamage`, e rimuove il binding in `EndPlay`. `UFUNCTION()` può quindi servire ai delegati dinamici anche senza rendere la funzione un nodo `BlueprintCallable`.

### 5. AI: Behavior Tree e reflection

L'IA sviluppata per Tinyverse usa gli asset di [`Content/TinyPlanet/Blueprints/AI/`](Content/TinyPlanet/Blueprints/AI/). La sezione [Behavior Tree e Blackboard](#behavior-tree-e-blackboard) descrive i comportamenti; qui il collegamento con HighLevel riguarda il modo in cui la reflection rende configurabili e richiamabili i sistemi C++ dai Blueprint dell'IA.

In [`TinyverseEnemyAIController.h`](Source/Tinyverse/TinyverseEnemyAIController.h), il controller è dichiarato `UCLASS(Blueprintable)` e il Behavior Tree è una proprietà riflessa:

```cpp
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AI")
TObjectPtr<UBehaviorTree> BehaviorTreeAsset = nullptr;
```

Il Blueprint `BP_EnemyAIController` può quindi assegnare `BT_Enemy` nell'editor. Il metodo C++ `OnPossess` verifica il nemico e l'asset, poi avvia il comportamento con `RunBehaviorTree`.

I task `BTT_PatrolPlanet`, `BTT_TrackTargetPlanet` e `BTT_ChargePlanet` derivano da `BTTask_BlueprintBase`, mentre `BTS_FindChargeTarget` deriva da `BTService_BlueprintBase`. Nei loro grafi, gli eventi di esecuzione, aggiornamento e interruzione coordinano le operazioni esposte dal nemico:

- `BTT_PatrolPlanet` richiama `MoveAlongPlanet` e `ReversePatrolDirection`.
- `BTT_TrackTargetPlanet` richiama `TrackTargetAlongPlanet` e `StopTracking`.
- `BTT_ChargePlanet` richiama `StartCharge`, `UpdateCharge` e `CancelCharge`.
- `BTS_FindChargeTarget` aggiorna le chiavi `TargetActor` e `CanCharge` del Blackboard, usando i parametri di rilevamento e attivazione della carica.

Le operazioni del nemico sono dichiarate `UFUNCTION(BlueprintCallable)`; parametri come raggi, velocità e durate sono esposti con `UPROPERTY`, mentre la fase di carica usa l'enumerazione `UENUM(BlueprintType)`. La reflection collega così la configurazione e le decisioni nei grafi Blueprint alla locomozione e al combattimento implementati in C++.

### 6. Salvataggi e serializzazione

In [`TinyverseSaveGame.h`](Source/Tinyverse/TinyverseSaveGame.h) i campi persistenti sono proprietà riflesse annotate con `SaveGame`:

```cpp
UPROPERTY(SaveGame)
FTransform PlayerTransform = FTransform::Identity;

UPROPERTY(SaveGame)
TSet<FGuid> RemoveActorIds;
```

Il subsystem costruisce un oggetto `UTinyverseSaveGame`, ne valorizza i campi e lo passa alle API Unreal di scrittura e lettura. La serializzazione viene gestita dal motore sui dati raccolti nell'oggetto; la ricostruzione dello stato di gameplay è responsabilità del codice del progetto.

**Precisazione tecnica:** `SaveGame` identifica le proprietà destinate al salvataggio ed è utilizzabile dagli archivi che filtrano quel flag. L'API `SaveGameToSlot` usata qui serializza però tutte le proprietà non transient del SaveGame e non controlla il flag `SaveGame`, come specifica la [documentazione Epic dell'API](https://dev.epicgames.com/documentation/en-us/unreal-engine/BlueprintAPI/SaveGame/SaveGametoSlot). Il sistema non salva automaticamente tutti gli Actor: il progetto copia esplicitamente i dati necessari in `UTinyverseSaveGame`.

### 7. Riferimenti agli oggetti e strumenti dell'editor

Proprietà come `ActiveGravityPlanet`, `NearbyGravityPlanets`, `PendingSave` e i riferimenti agli eventi FMOD usano `UPROPERTY` con `TObjectPtr`. Il sistema UObject può così tracciare i riferimenti riflessi. `TSoftObjectPtr<UWorld>` permette di riferirsi al livello da aprire, mentre `TSubclassOf<UUserWidget>` configura il tipo di widget da creare.

`RegeneratePersistentId` usa `UFUNCTION(CallInEditor)`: l'editor offre un'azione per rigenerare l'identità dell'oggetto. Le Function Library, invece, espongono utilità statiche per animazione e audio con nomi e parametri adatti ai Blueprint.

Questi impieghi collegano la reflection a esigenze concrete: bilanciamento dei parametri, comunicazione C++/Blueprint, eventi di gameplay, riferimenti agli oggetti e persistenza. Un riferimento generale è la [documentazione del sistema di reflection Unreal](https://dev.epicgames.com/documentation/unreal-engine/reflection-system-in-unreal-engine).

## Requisiti, installazione e avvio

### Requisiti

- **Unreal Engine 5.8**, coerente con `EngineAssociation` nel `.uproject` e con `EngineIncludeOrderVersion.Unreal5_8` nei target.
- Toolchain C++ supportata dal motore. Su Windows, Visual Studio con strumenti di sviluppo C++ per giochi e Windows SDK compatibile.
- **Git e Git LFS**, necessari per scaricare gli asset binari.
- Integrazione **FMODStudio** inclusa in `Plugins/FMODStudio/`.
- **FMOD Studio compatibile con l'integrazione 2.03.14** se si vuole modificare e ricostruire il progetto audio. Le bank incluse permettono di lavorare sugli audio già esportati.

Il progetto è configurato per hardware desktop. I runtime FMOD inclusi coprono Win64 e Android; per altre piattaforme occorre predisporre le relative dipendenze. La presenza di impostazioni Linux e macOS nei file di configurazione non prova che il gioco sia stato compilato su quelle piattaforme.

### Clonazione

```bash
git lfs install
git clone --branch develop https://github.com/CRICRICode/tinyverse.git
cd tinyverse
git lfs pull
```

Per un clone già esistente, selezionare `develop` e scaricare gli oggetti LFS prima di aprire l'editor. `.gitattributes` configura LFS per `.uasset`, `.umap`, `.bank`, `.dll`, `.lib`, `.so` e `.wav`.

### Compilazione e avvio nell'editor

1. Generare i file di progetto C++ a partire da `Tinyverse.uproject`.
2. Aprire il progetto nell'IDE e compilare il target **TinyverseEditor**, configurazione **Development Editor**, piattaforma **Win64**.
3. Aprire `Tinyverse.uproject` con Unreal Engine 5.8.
4. Verificare che i plugin **HDRIBackdrop** e **FMODStudio** siano disponibili per gli ambienti e l'audio del gioco. Il `.uproject` abilita esplicitamente HDRIBackdrop; FMOD è incluso come plugin abilitato per default.
5. Attendere la compilazione degli shader e premere **Play** da `MenuMap`.
6. Selezionare **New Game** per avviare `L_FirstLevel`, oppure **Continue** per caricare lo slot esistente.

Per una compilazione da PowerShell, adattare i percorsi alla propria installazione:

```powershell
$TinyverseEngineRoot = "C:\Program Files\Epic Games\UE_5.8"
$TinyverseProjectFile = (Resolve-Path .\Tinyverse.uproject).Path
& "$TinyverseEngineRoot\Engine\Build\BatchFiles\Build.bat" `
    TinyverseEditor Win64 Development "-Project=$TinyverseProjectFile" -WaitMutex
```

### Packaging

Il target di gioco è **Tinyverse**. La configurazione in `DefaultGame.ini` usa build Development, file Pak, IoStore e compressione Oodle/Kraken, oltre alle impostazioni di inclusione FMOD.

Prima di creare un pacchetto, verificare che le mappe richiamate dai menu siano incluse nel cook e che le bank siano disponibili in `Content/FMOD/Desktop`. Il pacchetto va provato partendo da `MenuMap`, controllando anche nuova partita, continua e audio.

### Problemi comuni

| Problema | Controllo da effettuare |
| --- | --- |
| Asset mancanti o file che contengono soltanto un puntatore LFS | Eseguire `git lfs pull` e verificare `git lfs ls-files` |
| Progetto appena clonato senza le funzionalità descritte | Verificare di essere sul branch `develop` |
| Moduli mancanti o incompatibili | Usare UE 5.8, rigenerare i file dell'IDE e ricompilare `TinyverseEditor` |
| Errori di dipendenza FMOD | Verificare plugin, runtime e librerie ottenuti tramite LFS |
| Nessun suono FMOD | Controllare caricamento delle bank, asset generati e percorsi degli eventi |
| La ricostruzione FMOD scrive le bank altrove | Adattare la cartella di output del progetto audio al proprio clone |
| Continue disabilitato | Avviare una nuova partita e creare il primo salvataggio |
| Un oggetto ricompare dopo il caricamento | Controllare componente di identità, GUID valido, persistenza attiva e salvataggio effettuato dopo la rimozione |

## Configurazione e strumenti di sviluppo

Il progetto favorisce la configurazione tramite editor: modificare i default dei Blueprint per cambiare il comportamento di una categoria di oggetti, oppure i parametri delle singole istanze per differenziare pianeti, nemici e piattaforme nello stesso livello.

Per inserire nuovi elementi:

- **Pianeta:** usare `BP_GravityPlanet`, configurare geometria, raggio, influenza, accelerazione ed eventuale falloff.
- **Nemico:** usare un Blueprint derivato da `ATinyverseEnemy`, assegnare `GravityPlanet`, controller e Behavior Tree, quindi regolare salute e comportamento.
- **Moneta persistente:** usare `BP_Coin` e verificare la configurazione del SaveIdentityComponent.
- **Piattaforma:** usare `BP_DynamicPlatform`, impostare `MovementOffset`, velocità e sosta.
- **Evento sonoro:** assegnare l'asset FMOD alla proprietà esposta oppure mantenere un percorso evento valido.

Il progetto comprende diagnostica per le build di sviluppo:

- `bShowGravityDebug` disegna la direzione della gravità e registra i cambi di sorgente;
- log di raccolta monete, ricompense e salvataggi;
- log dello stomp con danno richiesto, danno applicato e stato di invulnerabilità;
- messaggi per Behavior Tree o eventi FMOD mancanti.

Questi messaggi sono in gran parte protetti da `!UE_BUILD_SHIPPING`. `DefaultEngine.ini` contiene anche **Core Redirects** per mantenere compatibili proprietà e classi rinominate con gli asset serializzati nelle revisioni precedenti.

## Percorso di dimostrazione

Per presentare i sistemi principali del progetto nell'editor:

1. Avviare da `MenuMap` e mostrare pulsanti, sfondo video, nuova partita e disponibilità di Continue.
2. In `L_FirstLevel`, percorrere una superficie sferica e cambiare pianeta, osservando gravità e allineamento della camera. Attivare il debug gravitazionale per rendere visibile la direzione.
3. Mostrare un nemico in pattuglia, avvicinarsi per l'inseguimento e osservare preparazione, scatto e recupero della carica.
4. Colpire il trigger di stomp e mostrare rimbalzo e danno; ricevere un colpo per mostrare respinta, invulnerabilità e aggiornamento dei cuori.
5. Raccogliere monete fino alla soglia, mostrando il contatore e la differenza tra cura e incremento della salute massima.
6. Usare una piattaforma mobile e mostrare percorso, inversione e pausa agli estremi.
7. Salvare, tornare al menu e continuare: verificare transform, salute, monete e assenza degli oggetti rimossi persistentemente. Il pulsante Riprova del menu di morte consente di mostrare il recupero dal salvataggio.
8. Per HighLevel, aprire gli header indicati nella sezione reflection, mostrare l'enumerazione delle fasi di carica, modificare un parametro esposto nell'editor e seguire un evento dal componente C++ al widget Blueprint. Aprire un task AI, per esempio `BTT_ChargePlanet`, e mostrare le chiamate alle funzioni C++ riflesse. Mostrare poi le proprietà del SaveGame e l'azione `RegeneratePersistentId` disponibile nell'editor.

## Licenza e contenuti di terze parti

La repository contiene una [licenza MIT](LICENSE), © 2026 Giuseppe. Unreal Engine, FMOD e i contenuti di terze parti mantengono le proprie condizioni di licenza; il file MIT della repository non sostituisce tali condizioni.

Il progetto utilizza risorse grafiche di terze parti e l'integrazione FMOD di Firelight Technologies. Quando si riutilizzano o distribuiscono questi contenuti, fare riferimento anche alle condizioni dei rispettivi fornitori.
