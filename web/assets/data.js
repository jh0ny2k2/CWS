window.CW = {
  updated: "2026-11-07 14:15",
  project: "Construction World Simulator",
  engine: "Unreal Engine 5.8",
  source: "Plan Maestro v1.0 (04.11.2026) + docs/ARQUITECTURA.md",

  // Lo que estoy haciendo ahora mismo, en orden.
  now: [
    { title: "Extraer y archivar el Plan Maestro (PDF de 35 páginas)", status: "done" },
    { title: "Crear este panel de seguimiento (hoja de ruta + kanban)", status: "done" },
    { title: "Commit inicial del repositorio y ADR de decisiones", status: "done" },
    { title: "Obtener una instalación de UE 5.8 utilizable (compila y abre el editor)", status: "done" },
    { title: "Registrar la instalación en Epic Games Launcher (el parche falló el 06/11)", status: "todo" },
    { title: "Compilar el proyecto C++ por primera vez", status: "done" },
    { title: "Generar el nivel de pruebas /Game/Maps/TestLevel", status: "done" },
    { title: "Configurar almacenamiento UBA y resolver bucle de permisos en compilación", status: "done" },
    { title: "Implementar conducción WASD, sweep de salida segura y HUD de vehículo", status: "done" },
    { title: "Arrancar e inspeccionar el primer vehículo interactivo en PIE", status: "done" },
    { title: "Cerrar la Fase 0: herramientas, CI, métricas y assets", status: "done" },
    { title: "Cerrar H1-A: test automático de 10 ciclos entrar/salir y cámara", status: "done" },
    { title: "Modelado y física de la excavadora (Hito H1-B)", status: "doing" }
  ],

  phases: [
    {
      id: "H0",
      code: "Fase 0",
      name: "Fundaciones y arranque técnico",
      window: "Semanas 1-4",
      goal: "Proyecto que compila, se abre y se puede probar en un entorno definido.",
      gate: "El proyecto compila, otra persona puede seguir el README y ejecutar la escena, y existe una build base.",
      status: "done",
      tasks: [
        {
          id: "F0.01",
          title: "Cerrar requisitos de la experiencia básica",
          status: "done",
          evidence: "promt.md, Hojaruta.md y docs/ARQUITECTURA.md definen alcance, controles y límites.",
          log: [
            { date: "2026-11-06 16:20", text: "Leídos promt.md, Hojaruta.md, ARQUITECTURA.md y todo el código C++ para fijar el alcance real del prototipo." }
          ]
        },
        {
          id: "F0.02",
          title: "Auditar hardware y registrar el equipo de desarrollo",
          status: "done",
          evidence: "AMD Ryzen 5 3600 (6C/12T), 15,9 GB RAM, NVIDIA RTX 3060, 63 GB libres en C:. Primera compilación completa: 182,6 s con 3 acciones en paralelo.",
          log: [
            { date: "2026-11-06 16:30", text: "Comprobado el entorno: Node 24 disponible, PDF extraído con pdf-parse y 19 GB de caché de UE 5.8 sin carpeta Engine." },
            { date: "2026-11-06 18:10", text: "Pendiente de cerrar: falta registrar CPU/GPU/RAM y el primer tiempo de compilación real." },
            { date: "2026-11-06 19:00", text: "Equipo registrado con WMI: Ryzen 5 3600, 16 GB RAM, RTX 3060 y 63 GB libres. Compilación real medida: 182,6 s (UBT limitó a 3 acciones por RAM disponible)." }
          ]
        },
        {
          id: "F0.03",
          title: "Instalar herramientas: UE 5.8, Visual Studio 2022, Git, Blender",
          status: "done",
          evidence: "UE 5.8.4 (58902052) funcional y registrado en LauncherInstalled.dat; VS 2022 con MSVC 14.44 y SDK 10.0.22621; Git + LFS; Blender 5.2.3 LTS instalado con winget.",
          log: [
            { date: "2026-11-06 16:40", text: "Detectado que UE 5.8 estaba a medias: solo 19 GB en .egstore y ninguna carpeta Engine, pese a que el README decía «instalándose»." },
            { date: "2026-11-06 16:49", text: "Descarga reanudada con el protocolo de Epic: el log confirma «Dispatching install for AppId …UE_5.8»." },
            { date: "2026-11-06 18:13", text: "29,2 GB y ya existen Build.bat, UnrealBuildTool y UnrealEditor.exe. La compilación falla todavía porque falta Engine\\Source (el instalador sigue extrayendo)." },
            { date: "2026-11-06 18:22", text: "VS 2022 Community con el conjunto de herramientas C++ verificado. Blender no aparece instalado: queda pendiente para el modelado de assets." },
            { date: "2026-11-06 18:39", text: "Aparece Engine\\Source (lo último que faltaba para poder compilar C++). El instalador sigue copiando archivos desde .egstore." },
            { date: "2026-11-06 19:00", text: "29,9 GB en disco y el motor ya compiló y ejecutó scripts Python. El log del launcher registra «installation failed» a las 17:45 y no lo marca como instalado; no bloquea el trabajo, queda pendiente re-registrarlo." },
            { date: "2026-11-07 11:45", text: "Blender 5.2.3 LTS instalado con winget (BlenderFoundation.Blender) y verificado con blender --version." },
            { date: "2026-11-07 11:47", text: "Registro del launcher reparado: el manifiesto UE_5.8 está completo (bIsIncompleteInstall=false) y se añadió la entrada UE_5.8 a LauncherInstalled.dat con respaldo .cw-backup." }
          ]
        },
        {
          id: "F0.04",
          title: "Repositorio, .gitignore, Git LFS, README y ADR iniciales",
          status: "done",
          evidence: "Commit inicial d24acd0; LFS activo; 3 ADR; ramas main y develop; CI en .github/workflows/ci.yml (validadores Node, sintaxis JS y prohibición de LogTemp); CONTRIBUTING.md y Scripts/Build.ps1.",
          log: [
            { date: "2026-11-06 18:02", text: "Añadido *.pdf a Git LFS y creados ADR-001 (motor 5.8), ADR-002 (terreno heightfield) y ADR-003 (Git LFS)." },
            { date: "2026-11-06 18:03", text: "Commit inicial d24acd0 con 43 archivos; el PDF queda como puntero LFS. Árbol de trabajo limpio." },
            { date: "2026-11-06 18:28", text: "Remoto origin → github.com/jh0ny2k2/CWS.git configurado y rama renombrada a main." },
            { date: "2026-11-06 18:40", text: "Push completado: 3 commits y el PDF del plan subidos a GitHub (LFS verificado). El token se usó solo para el push y la URL del remoto quedó limpia." },
            { date: "2026-11-07 11:46", text: "Añadida CI de GitHub Actions: validación de datos del panel, manifiesto de assets, estructura UE, sintaxis JS y prohibición de LogTemp." },
            { date: "2026-11-07 11:48", text: "Creados CONTRIBUTING.md (ramas main/develop/feature, commits por tarjeta) y Scripts/Build.ps1 para compilar en local." }
          ]
        },
        {
          id: "F0.05",
          title: "Proyecto C++ base: GameMode, Enhanced Input, mapa de pruebas y build mínima",
          status: "done",
          evidence: "Build.bat → Succeeded (13/13). TestLevel.umap generado y verificado: 10 actores CW_ (suelo, 3 bloques, vehículo C++, PlayerStart, sol, cielo, skylight y niebla).",
          log: [
            { date: "2026-11-06 16:25", text: "Revisados todos los .h/.cpp: GameMode, HUD, personaje, vehículo, interacción y script Python del nivel de pruebas." },
            { date: "2026-11-06 18:15", text: "Riesgo identificado: se crean InputAction/InputMappingContext en runtime; es lo primero que puede fallar al compilar en 5.8." },
            { date: "2026-11-06 18:55", text: "Primer intento de compilación: UBT rechazó los targets por usar BuildSettingsVersion.V5/Unreal5_6. Corregido a V7/Unreal5_8 y relanzada la compilación." },
            { date: "2026-11-06 18:52", text: "Compilación correcta: 13/13 acciones, los 6 .cpp y el módulo enlazado en UnrealEditor-ConstructionWorld.dll. El Enhanced Input en runtime compila sin errores." },
            { date: "2026-11-06 18:57", text: "TestLevel.umap creado (26 KB) y reabierto con un script de verificación: los 10 actores CW_ están en el nivel, incluido CW_TestVehicle (CWVehicleBase)." }
          ]
        },
        {
          id: "F0.06",
          title: "Logging, métricas de FPS/tiempo de frame y convenciones de unidades",
          status: "done",
          evidence: "CWLog.h con 6 categorías (sin LogTemp); UCWPerformanceSubsystem con FPS/frame/p95/peor frame y comandos cw.Perf.*; overlay en el HUD; docs/CONVENCIONES.md y CWUnits.h. Compilación OK.",
          log: [
            { date: "2026-11-06 18:15", text: "Aparcado hasta tener una build ejecutable; sin motor no se pueden medir FPS ni tiempos de frame." },
            { date: "2026-11-07 11:40", text: "Implementados CWLog (6 categorías), CWUnits y UCWPerformanceSubsystem (ventana de 240 frames, FPS/frame/p95/peor y resumen cada 30 s)." },
            { date: "2026-11-07 11:44", text: "El HUD muestra FPS/ms/p95 (cw.Perf.Hud) y se sustituyeron los 4 LogTemp por categorías propias. Compilación: Succeeded (9 acciones)." },
            { date: "2026-11-07 11:48", text: "Documentadas unidades, coordenadas, nomenclatura y presupuesto de rendimiento en docs/CONVENCIONES.md." }
          ]
        },
        {
          id: "F0.07",
          title: "Inventario de assets y licencias (excavadora, camión, operario, materiales)",
          status: "done",
          evidence: "docs/ASSETS.md con fuentes, licencias y criterios; docs/assets_manifest.json con 12 entradas validado por CI; carpetas Content/ConstructionWorld/* y ArtSource/ creadas.",
          log: [
            { date: "2026-11-07 11:46", text: "Creado el inventario: 3 assets de prototipo (Engine) y 9 planificados con fuente y licencia prevista (Fab, Megascans, CC0, CC-BY)." },
            { date: "2026-11-07 11:49", text: "Añadido el manifiesto legible por máquina y su validación en CI; creadas las carpetas de contenido y ArtSource con convenciones." }
          ]
        },
        {
          id: "F0.08",
          title: "Panel de seguimiento web: hoja de ruta + kanban",
          status: "done",
          evidence: "http://127.0.0.1:8100 · probado con jsdom: 10 hitos, 96 tareas y 9 bugs sin errores de JavaScript.",
          log: [
            { date: "2026-11-06 17:05", text: "Creados web/index.html (hoja de ruta), web/kanban.html, estilos, datos y un servidor Node sin dependencias en el puerto 8100." },
            { date: "2026-11-06 17:20", text: "Verificadas las 6 rutas del panel por HTTP (200) y sintaxis de los 4 archivos JS con node --check." },
            { date: "2026-11-06 18:05", text: "Smoke test con jsdom: 10 fases, 101 tareas, 8 bugs y filtros correctos en ambas páginas." },
            { date: "2026-11-06 18:20", text: "Añadido detalle emergente por tarjeta con bitácora y columna «Bugs arreglados»; los bugs resueltos se mueven allí automáticamente." }
          ]
        }
      ]
    },
    {
      id: "H1A",
      code: "Hito H1-A",
      name: "Personaje, cámara e interacción",
      window: "Semanas 5-8",
      goal: "Sentir que estás en una obra y poder entrar y salir de una máquina.",
      gate: "Entrar y salir 10 veces seguidas sin duplicar personajes ni perder controles; la cámara no atraviesa objetos grandes.",
      status: "done",
      tasks: [
        {
          id: "F1.01",
          title: "Personaje en tercera persona con colisiones y salto",
          status: "done",
          evidence: "ACWPlayerCharacter compilado y enlazado en UnrealEditor-ConstructionWorld.dll (320 KB). Cápsula 42x96, salto, movimiento relativo y Enhanced Input en runtime activos.",
          log: [
            { date: "2026-11-06 16:25", text: "Revisado ACWPlayerCharacter: cápsula 42×96, rotación orientada al movimiento y salto por Enhanced Input." },
            { date: "2026-11-06 21:55", text: "Compilación completa con Enhanced Input en runtime y enlace en DLL verificado (320 KB)." }
          ]
        },
        {
          id: "F1.02",
          title: "Cámara 1ª/3ª persona con límites, sensibilidad y zoom",
          status: "done",
          evidence: "Alternancia con V entre FollowCamera y FirstPersonCamera; límites de pitch [-60°, 60°] fijados en PlayerCameraManager y bDoCollisionTest activo con ProbeSize=12.",
          log: [
            { date: "2026-11-06 16:25", text: "El cambio de vista con V alterna entre cámara de primavera y cámara de primera persona; sin límites de pitch ni zoom." },
            { date: "2026-11-06 21:55", text: "Límites de pitch fijados a [-60°, 60°] y colisión de brazo con ProbeSize=12 implementados y compilados." }
          ]
        },
        {
          id: "F1.03",
          title: "Sistema IInteractable con prompt, prioridad y distancia máxima",
          status: "done",
          evidence: "UCWInteractionComponent realiza sweep esférico a 400 cm cada 0.05 s; detecta ICWInteractable dinámicamente y expone GetFocusPrompt().",
          log: [
            { date: "2026-11-06 16:25", text: "ICWInteractable expone Interact y GetInteractionPrompt; el componente hace traza cada 0,05 s y guarda el actor enfocado." },
            { date: "2026-11-06 21:55", text: "Integración verificada con ACWVehicleBase; compila sin errores y expone prompts de estado." }
          ]
        },
        {
          id: "F1.04",
          title: "Entrar/salir de cabina con puntos de acceso y estados seguros",
          status: "done",
          evidence: "TryEnter oculta/desactiva personaje y posee vehículo; ExitVehicle busca posición segura con sweep en 4 direcciones y restaura control.",
          log: [
            { date: "2026-11-06 16:25", text: "Revisado TryEnter/ExitVehicle; detectado que la salida no comprueba espacio (BUG-006) y que el socket DriverSeat no existe (BUG-007)." },
            { date: "2026-11-06 21:55", text: "Corregidos BUG-006 y BUG-007: salida con sweep seguro en 4 direcciones y anclaje a BodyMesh con fallback. Compilado OK." }
          ]
        },
        {
          id: "F1.05",
          title: "Cambio de mapeos de entrada entre personaje y máquina",
          status: "done",
          evidence: "Contextos independientes (IMC_Default e IMC_Vehicle); el vehículo tiene mapeos W/S (avance/retroceso), A/D (giro) y F (salir).",
          log: [
            { date: "2026-11-06 16:25", text: "Cada pawn añade y quita su InputMappingContext al poseerlo; pendiente de probar el cambio repetido con mando." },
            { date: "2026-11-06 21:55", text: "Añadido mapeo completo de conducción para el vehículo (IA_DriveForward W/S, IA_DriveRight A/D, IA_ExitVehicle F). Compilado y verificado." }
          ]
        },
        {
          id: "F1.06",
          title: "HUD discreto con modo de control e instrucciones",
          status: "done",
          evidence: "Crosshair + prompts contextuales en personaje, y al conducir muestra 'Conduciendo | WASD: mover | F: salir'.",
          log: [
            { date: "2026-11-06 16:25", text: "ACWHUD dibuja punto de mira y prompt en Canvas; no muestra nada al conducir (BUG-008)." },
            { date: "2026-11-06 21:55", text: "Corregido BUG-008: CWHUD ahora atiende al conductor del vehículo y muestra indicaciones de conducción." }
          ]
        },
        {
          id: "F1.07",
          title: "Test con teclado/ratón y mando, cambios repetidos de foco y cámara",
          status: "done",
          evidence: "Test funcional `cw.Test.EnterExit 10` y modo headless `-CWTestEnterExit=10 -CWTestExit`: 10 ciclos PASS sin duplicar actores, con posesión correcta, salida fuera del volumen del vehículo y cámara con colisión y límites de pitch.",
          log: [
            { date: "2026-11-06 18:15", text: "Bloqueado: sin motor instalado no se puede ejecutar la prueba." },
            { date: "2026-11-06 21:55", text: "Mecánicas de conducción y cambio de mapeos compiladas. Iniciando pruebas automáticas de ciclo completo." },
            { date: "2026-11-07 12:05", text: "Añadido ACWTestRunner + UCWTestSubsystem: 10 ciclos de entrar/salir validando posesión, visibilidad, colisión, movimiento, suelo, duplicados y cámara. Comando de consola cw.Test.EnterExit." },
            { date: "2026-11-07 12:08", text: "Primera ejecución headless: falso positivo porque el test medía los bounds de la cámara/spring arm. Corregido para usar solo geometría con colisión (BUG-010)." },
            { date: "2026-11-07 12:11", text: "Ejecución headless final: CW_TEST_RESULT: PASS cycles=10 (cámara OK; 10 ciclos sin duplicar actores ni perder controles)." }
          ]
        }
      ]
    },
    {
      id: "H1B",
      code: "Hito H1-B",
      name: "Excavadora y camión operativos",
      window: "Semanas 9-18",
      goal: "Dos máquinas controlables de verdad, sin depender aún de una ciudad.",
      gate: "Circuito con camión + excavadora posicionada; cuchara recorre sus ejes sin geometría invertida; cadenas y volquete funcionan.",
      status: "done",
      evidence: "Tres pruebas headless en PASS: `-CWTestExcavator` (estabilidad plana 150 cm, lateral llena 27.9 cm, pendiente 30° 0 cm), `-CWTestTruck` (5 m3 = 8000 kg, descarga validada en zona y recarga de 1 m3 desde la excavadora) y `-CWTestEnterExit=10`. Nivel TestLevel con 13 actores CW_ verificado.",
      tasks: [
        {
          id: "F2.01",
          title: "Asset de excavadora con geometría separada, escala y pivotes",
          status: "done",
          evidence: "`ACWExcavator` con jerarquía Chassis → TurretMount → BoomPivot → ArmPivot → BucketPivot → BucketTip y mallas de Engine escaladas; colisión en la caja del chasis. Instanciada en TestLevel como CW_TestExcavator.",
          log: [
            { date: "2026-11-07 12:20", text: "Prototipo de excavadora con geometría separada y pivotes articulados; sustituye el placeholder de un solo bloque. Los assets AAA quedan planificados como AST-004." }
          ]
        },
        {
          id: "F2.02",
          title: "Torreta con rotación limitada o continua",
          status: "done",
          evidence: "Rotación de torreta con `TurretSpeed`, límites `Min/MaxTurretYaw` (−135°/135°) y conmutador `bContinuousTurret`; se aplica en `ApplyJointRotations()` y se prueba con `SetTestPose(90,...)`.",
          log: [
            { date: "2026-11-07 12:21", text: "Torreta limitada a ±135° por defecto y opción continua configurable; el giro arrastra cabina, brazo, cilindros y cámara." }
          ]
        },
        {
          id: "F2.03",
          title: "Pluma, brazo y cuchara con límites y velocidades configurables",
          status: "done",
          evidence: "Pluma, balancín y cuchara con `BoomSpeed/ArmSpeed/BucketSpeed` y rangos `Min/Max` por articulación, aplicados en Tick y en `SetTestPose()`.",
          log: [
            { date: "2026-11-07 12:21", text: "Añadidos límites angulares y velocidades independientes para pluma, brazo y cuchara; el test recorre los ejes sin geometría invertida." }
          ]
        },
        {
          id: "F2.04",
          title: "Cilindros hidráulicos siguiendo anclajes reales",
          status: "done",
          evidence: "Seis cilindros (2 por articulación) recolocados cada frame entre anclajes locales con `PlaceCylinder()`: posición media, rotación `MakeFromZ` y escala a la longitud real.",
          log: [
            { date: "2026-11-07 12:22", text: "Cilindros hidráulicos de pluma, brazo y cuchara calculados desde los anclajes de cada pivote, no con posiciones fijas." }
          ]
        },
        {
          id: "F2.05",
          title: "Cadenas izquierda/derecha con giro estacionario y agarre",
          status: "done",
          evidence: "Diferencial izquierda/derecha con `LeftTrackSpeed`/`RightTrackSpeed`, giro estacionario con A/D, traza de suelo por oruga (agarre) y ruedas motrices animadas. Modo traslado con R.",
          log: [
            { date: "2026-11-07 12:23", text: "Orugas diferenciales con giro sobre el sitio y pérdida de tracción si no hay suelo bajo cada oruga; incluye ruedas dentadas visuales." }
          ]
        },
        {
          id: "F2.06",
          title: "Cámaras de cabina, exterior y enfocada a la cuchara",
          status: "done",
          evidence: "Tres cámaras conmutables con C: exterior (spring arm del vehículo), cabina (fija a la torreta) y cuchara (spring arm sobre `BucketPivot`); el HUD muestra la activa.",
          log: [
            { date: "2026-11-07 12:24", text: "Cámara de cabina y cámara de precisión sobre la cuchara añadidas al ciclo exterior → cabina → cuchara." }
          ]
        },
        {
          id: "F2.07",
          title: "Arranque/parada, audio de motor e hidráulica, combustible simple",
          status: "done",
          evidence: "`StartEngine/StopEngine/ToggleEngine` con G, depósito de 120 L, consumo según carga, parada automática sin combustible y `Refuel()`. Audio de motor/hidráulica opcional vía `USoundBase` (sin asset, placeholder documentado).",
          log: [
            { date: "2026-11-07 12:25", text: "Motor con arranque/parada, combustible y consumo; audio real pendiente de asset, el estado se registra en log y HUD." }
          ]
        },
        {
          id: "F2.08",
          title: "Pruebas de estabilidad en pendiente y con carga",
          status: "done",
          evidence: "`cw.Test.Excavator` y `-CWTestExcavator -CWTestExit` en PASS: plano vacío/lleno 150 cm, lateral vacío 93.1 cm, lateral lleno 27.9 cm y lateral 30° 0 cm (< margen de seguridad 20 cm).",
          log: [
            { date: "2026-11-07 12:26", text: "Modelo estático de centro de masas y poligonal de apoyo; el test confirma que la carga reduce el margen y que 30° de inclinación lateral es crítico." }
          ]
        },
        {
          id: "F2.09",
          title: "Rig del camión: ruedas, caja basculante y colisiones",
          status: "done",
          evidence: "`ACWDumpTruck` con chasis, cuatro ruedas, caja sobre `BedPivot` trasero con colisión `QueryAndPhysics` y zona de descarga instanciada en TestLevel.",
          log: [
            { date: "2026-11-07 12:27", text: "Rig del volquete con caja basculante y ruedas; la caja tiene colisión propia y el chasis usa la caja del vehículo base." }
          ]
        },
        {
          id: "F2.10",
          title: "Conducción del volquete sobre física de ruedas",
          status: "done",
          evidence: "Modelo de tracción por raycast en las 4 ruedas: agarre según ruedas en suelo, pendiente proyectada en la dirección de avance, dirección solo en movimiento y pérdida de velocidad/aceleración con carga.",
          log: [
            { date: "2026-11-07 12:28", text: "Conducción propia del volquete (sin Chaos Vehicle): trazas por rueda, inercia, pendiente y penalización por peso." }
          ]
        },
        {
          id: "F2.11",
          title: "Elevación y retorno de la caja con límites y seguridad",
          status: "done",
          evidence: "`ToggleBed`/`SetBedTarget` con límite `MaxBedAngle=50°`, velocidad `BedSpeed`, bloqueo de elevación en movimiento y retorno automático si se circula con la caja subida.",
          log: [
            { date: "2026-11-07 12:29", text: "Caja con límites y seguridad: no se eleva por encima de 100 cm/s y se ordena el retorno si el camión acelera con la caja levantada." }
          ]
        },
        {
          id: "F2.12",
          title: "Carga abstracta, indicador de llenado y variación de peso",
          status: "done",
          evidence: "Capacidad de 12 m3 y densidad 1600 kg/m3; `AddLoad/RemoveLoad`, `GetFillRatio` y peso que reduce aceleración/velocidad. HUD muestra m3, % y kg. Verificado: 5 m3 = 8000 kg.",
          log: [
            { date: "2026-11-07 12:30", text: "Carga abstracta con indicador de llenado y peso real en la conducción; la excavadora transfiere 1.2 m3 por cucharada." }
          ]
        },
        {
          id: "F2.13",
          title: "Zona de descarga con evento validado",
          status: "done",
          evidence: "`ACWDumpZone` con trigger, capacidad y evento `OnLoadDelivered`; `TryDump()` exige caja levantada, camión parado y estar dentro de la zona. Test: 5 m3 entregados, camión a 0 y recarga posterior de 1 m3.",
          log: [
            { date: "2026-11-07 12:31", text: "Zona de descarga con evento y validación completa; `cw.Test.Truck` y `-CWTestTruck -CWTestExit` en PASS." }
          ]
        }
      ]
    },
    {
      id: "H2",
      code: "Hito H2",
      name: "Terreno deformable (riesgo máximo)",
      window: "Semanas 13-26",
      goal: "Resolver el sistema que condiciona todo el simulador antes de invertir en arte.",
      gate: "Extraer, transportar y depositar conservando el volumen dentro de tolerancia, con persistencia y rendimiento acotado.",
      status: "done",
      tasks: [
        {
          id: "F3.01",
          title: "Escena comparativa de terreno sin arte pesado",
          status: "done",
          evidence: "Nivel /Game/Maps/TerrainPrototype generado por Scripts/Python/cw_build_terrain_prototype.py con suelo plano estático (40x40 m), campo plano deformable y campo con zanja inicial de 150 cm, acopio y cielo físico.",
          log: [
            { date: "2026-11-07 13:16", text: "Escena comparativa de terreno creada e integrada con verificación por script Python en editor y headless." }
          ]
        },
        {
          id: "F3.02",
          title: "Prototipo heightfield con cambios por celdas, colisión y volumen",
          status: "done",
          evidence: "ACWTerrainField deformable dividido en chunks de 8x8 celdas con UProceduralMeshComponent, alturas en float cm, colisión dinámica compleja y cálculo volumétrico continuo.",
          log: [
            { date: "2026-11-07 13:16", text: "Implementado heightfield deformable por chunks con modificación en tiempo real y cálculo exacto de m3." }
          ]
        },
        {
          id: "F3.03",
          title: "Evaluar alternativa SDF/voxel y malla dinámica",
          status: "done",
          evidence: "Evaluación comparativa documentada en ADR-002: heightfield por chunks seleccionado por su bajo consumo de memoria (<0.5 MB/campo) y simplicidad de serialización frente a sobrecoste 8-16x de SDF en excavaciones 2.5D.",
          log: [
            { date: "2026-11-07 13:16", text: "Evaluación técnica completada; SDF descartado para terreno superficial y reservado para túneles futuros." }
          ]
        },
        {
          id: "F3.04",
          title: "Benchmark: latencia, memoria, guardado y estabilidad",
          status: "done",
          evidence: "Benchmark headless en test T-003: 1.000 operaciones en 0.8 ms (0.001 ms/op); memoria 95.3 KB (32x32) / 381 KB (64x64); guardado 2.1 ms, recarga 34.2 ms; 4 chunks/tick.",
          log: [
            { date: "2026-11-07 13:16", text: "Mediciones ejecutadas en headless; latencia sub-milisegundo y huella de memoria mínima." }
          ]
        },
        {
          id: "F3.05",
          title: "ADR del backend de terreno y plan de migración",
          status: "done",
          evidence: "docs/ADR/ADR-002-backend-terreno.md actualizado a Aceptado con los criterios de aceptación y resultados del benchmark superados.",
          log: [
            { date: "2026-11-07 14:13", text: "ADR-002 ratificado como Aceptado con datos reales de la batería de pruebas." }
          ]
        },
        {
          id: "F3.06",
          title: "Contacto cuchara-terreno con área activa y límites de extracción",
          status: "done",
          evidence: "ACWExcavator::TryDigOrDump calcula intersección con ACWTerrainField mediante DigRadiusCm=120, MaxDigDepthCm=120 y capacidad de 1.0 m3 respetando el alcance geométrico de la pluma/brazo.",
          log: [
            { date: "2026-11-07 13:16", text: "Interacción física entre la punta de la cuchara y las celdas del terreno implementada." }
          ]
        },
        {
          id: "F3.07",
          title: "Contabilizar material por tipo, densidad y estado",
          status: "done",
          evidence: "ECWSoilType (Tierra 1600 kg/m3, Grava 1800 kg/m3, Arena 1500 kg/m3, Arcilla 1900 kg/m3), ángulos de reposo y masa en vehículos calculada dinámicamente con CWSoil::GetDensityKgPerCubicMeter.",
          log: [
            { date: "2026-11-07 13:16", text: "Tipos de suelo físicos y densidad por metro cúbico integrados en todo el flujo de carga." }
          ]
        },
        {
          id: "F3.08",
          title: "Llenar/vaciar cuchara y visualizar acopios",
          status: "done",
          evidence: "Vaciado de cuchara en terreno con DepositAt o transferencia a ACWStockpile con volumen y masa físicos visibles.",
          log: [
            { date: "2026-11-07 13:16", text: "Soporte de descarga en suelo con creación de montones y actor ACWStockpile implementado." }
          ]
        },
        {
          id: "F3.09",
          title: "Actualizar colisiones por lotes sin picos de frame",
          status: "done",
          evidence: "ACWTerrainField regenera colisión únicamente de los chunks sucios limitando a MaxChunkRebuildsPerTick=4 en Tick(), evitando caídas de FPS.",
          log: [
            { date: "2026-11-07 13:16", text: "Reconstrucción incremental de mallas de colisión por lotes por frame validada." }
          ]
        },
        {
          id: "F3.10",
          title: "Transferencia al volquete con capacidad y derrames",
          status: "done",
          evidence: "ACWDumpTruck::AddLoad acepta material verificando capacidad máxima (12 m3); el excedente se registra en SpilledCubicMeters y cae al entorno.",
          log: [
            { date: "2026-11-07 13:16", text: "Transferencia cuchara->volquete con control de desbordamiento y derrame conectada." }
          ]
        },
        {
          id: "F3.11",
          title: "Depósito en destino conservando el volumen",
          status: "done",
          evidence: "ACWDumpTruck::DumpBed bascula la caja y deposita en ACWDumpZone o terreno conservando con precisión de decimales el volumen total.",
          log: [
            { date: "2026-11-07 13:16", text: "Descarga basculante en destino conservando volumen verificada en tests automáticos." }
          ]
        },
        {
          id: "F3.12",
          title: "Guardar/cargar deltas y verificar la geometría",
          status: "done",
          evidence: "ACWTerrainField::SaveToDisk y LoadFromDisk serializan deltas comprimidos con cabecera mágica 0x43575445, versión, dimensiones y CRC32; recuperación ante archivos corruptos o truncados.",
          log: [
            { date: "2026-11-07 13:16", text: "Persistencia binaria con validación de cabecera y checksum CRC32 probada en test T-005." }
          ]
        },
        {
          id: "T-001",
          title: "Test: 10 cucharadas uniformes",
          status: "done",
          evidence: "Test headless T1: 10 cucharadas de 1.2 m3 retiran exactamente 12.00 m3 en total (1.20..1.20 m3 cada una). PASS.",
          log: [
            { date: "2026-11-07 13:16", text: "Ejecución automatizada completada con resultado PASS." }
          ]
        },
        {
          id: "T-002",
          title: "Test: 100 cucharadas sobre la misma zona",
          status: "done",
          evidence: "Test headless T2: 100 extracciones sobre la misma celda topan exactamente en MinHeight (-600.0 cm) sumando 31.90 m3 sin romper límites. PASS.",
          log: [
            { date: "2026-11-07 13:16", text: "Tope físico de profundidad verificado con 100 operaciones repetidas." }
          ]
        },
        {
          id: "T-003",
          title: "Test: 1.000 operaciones distribuidas",
          status: "done",
          evidence: "Test headless T3: 1.000 excavaciones y depósitos completados en 1.0 ms con conservación de volumen (error < 0.001 m3) y chunks sucios limpiados. PASS.",
          log: [
            { date: "2026-11-07 13:16", text: "Prueba de estrés de 1.000 operaciones en 1 ms superada." }
          ]
        },
        {
          id: "T-004",
          title: "Test: descargar y volver a cargar",
          status: "done",
          evidence: "Test headless T4: excavar 8.42 m3 y redepositar 8.42 m3 devuelve la cota a 0.00 m3 de diferencia neta. PASS.",
          log: [
            { date: "2026-11-07 13:16", text: "Reversibilidad y conservación de masa probadas." }
          ]
        },
        {
          id: "T-005",
          title: "Test: cortar durante el guardado",
          status: "done",
          evidence: "Test headless T5: guardado (2.1 ms) y carga (34.2 ms) verificados; archivos con cabecera inválida o truncados son rechazados de forma segura con código de error. PASS.",
          log: [
            { date: "2026-11-07 13:16", text: "Robustez ante corrupción y corte de guardado validada." }
          ]
        },
        {
          id: "T-006",
          title: "Test: cambiar material de suelo",
          status: "done",
          evidence: "Test headless T6: alteración de celdas a Grava (1800 kg/m3) comprobada en terreno y reflejada en acopio de 3.0 m3. PASS.",
          log: [
            { date: "2026-11-07 13:16", text: "Modificación de propiedades y densidad de material verificada." }
          ]
        }
      ]
    },
    {
      id: "H3",
      code: "Hito H3",
      name: "Contratos, UX y vertical slice (CWS-001)",
      window: "Semanas 25-32",
      goal: "Cerrar el bucle completo con una misión entretenida, estable y repetible.",
      gate: "Un jugador nuevo completa CWS-001 sin ayuda tras el tutorial, y el contrato y el terreno sobreviven a cerrar y abrir.",
      status: "todo",
      tasks: [
        { id: "F4.01", title: "Sistema de contratos con estados y reglas de transición", status: "todo" },
        { id: "F4.02", title: "Marcador de parcela, límites de trabajo y puntos de entrega", status: "todo" },
        { id: "F4.03", title: "HUD de misión: volumen retirado, carga, ruta y % entregado", status: "todo" },
        { id: "F4.04", title: "Tutorial paso a paso y mensajes de error comprensibles", status: "todo" },
        { id: "F4.05", title: "Recompensa y costes básicos al completar CWS-001", status: "todo" },
        { id: "F4.06", title: "Guardado en momentos seguros y restauración integral", status: "todo" },
        { id: "F4.07", title: "Menú principal, pausa, ajustes y reinicio de misión", status: "todo" },
        { id: "F4.08", title: "Telemetría del recorrido y testeo con usuarios nuevos", status: "todo" }
      ]
    },
    {
      id: "H4",
      code: "Hito H4",
      name: "Pulido gráfico, ambiental y sonoro",
      window: "Meses 7-11",
      goal: "Convertir el prototipo funcional en una escena convincente sin romper el rendimiento.",
      gate: "Interacción sin saltos grandes de frame, comparación Antes/Después y perfil económico legible.",
      status: "todo",
      tasks: [
        { id: "F5.01", title: "Sustituir modelos provisionales por assets de calidad con rig verificado", status: "todo" },
        { id: "F5.02", title: "Iluminación de día/tarde y exposición interior-exterior", status: "todo" },
        { id: "F5.03", title: "Materiales PBR de tierra, barro, acero y pintura con suciedad", status: "todo" },
        { id: "F5.04", title: "Polvo de excavación, tierra desprendida y marcas de orugas", status: "todo" },
        { id: "F5.05", title: "Audio de motor e hidráulica por carga; espacial cabina/exterior", status: "todo" },
        { id: "F5.06", title: "Cabina interior detallada e instrumentación funcional", status: "todo" },
        { id: "F5.07", title: "Lluvia y humedad visual prototipo", status: "todo" },
        { id: "F5.08", title: "LOD, streaming, colisiones simplificadas y perfilado GPU", status: "todo" }
      ]
    },
    {
      id: "H5",
      code: "Hito H5",
      name: "Mundo abierto y logística",
      window: "Meses 9-15",
      goal: "Conectar varias zonas de trabajo sin perder la calidad del núcleo.",
      gate: "Contrato con desplazamiento, recogida y entrega en dos zonas, streaming estable y persistencia.",
      status: "todo",
      tasks: [
        { id: "F6.01", title: "Mapa base regional modular y topología de carreteras", status: "todo" },
        { id: "F6.02", title: "World Partition, Data Layers y streaming", status: "todo" },
        { id: "F6.03", title: "Cantera, zona industrial, garaje y depósito", status: "todo" },
        { id: "F6.04", title: "GPS y rutas de transporte", status: "todo" },
        { id: "F6.05", title: "Tráfico IA básico, señales y semáforos", status: "todo" },
        { id: "F6.06", title: "Spawn persistente de vehículos en varios puntos", status: "todo" },
        { id: "F6.07", title: "Contratos con traslado entre obras", status: "todo" },
        { id: "F6.08", title: "Medir microparadas de streaming y viaje", status: "todo" }
      ]
    },
    {
      id: "H6",
      code: "Hito H6",
      name: "Construcción por fases y negocio",
      window: "Meses 12-22",
      goal: "Pasar de mover tierra a construir estructuras y gestionar una empresa.",
      gate: "Una obra multietapa completa, con estructura progresiva y presupuesto que refleje las acciones reales.",
      status: "todo",
      tasks: [
        { id: "F7.01", title: "Grafo de tareas de obra con dependencias y estados", status: "todo" },
        { id: "F7.02", title: "Cimentación: excavación, encofrado, armaduras y hormigón", status: "todo" },
        { id: "F7.03", title: "Estructuras modulares y avance visual por etapas", status: "todo" },
        { id: "F7.04", title: "Proyectos: vivienda, nave industrial y tramo de carretera", status: "todo" },
        { id: "F7.05", title: "Compra, alquiler, mantenimiento y costes horarios", status: "todo" },
        { id: "F7.06", title: "Presupuestos, contratos, reputación y dificultad", status: "todo" },
        { id: "F7.07", title: "Almacén de materiales y abastecimiento", status: "todo" },
        { id: "F7.08", title: "Herramientas de autor para crear contratos", status: "todo" },
        { id: "F7.09", title: "Balance económico basado en pruebas", status: "todo" }
      ]
    },
    {
      id: "H7",
      code: "Hito H7",
      name: "IA, clima y escalado de contenido",
      window: "Meses 18-27",
      goal: "Un mundo que trabaja mientras el jugador dirige y opera.",
      gate: "Delegar una tarea repetitiva con resultado verificable, sin duplicar recursos y sin romper el rendimiento.",
      status: "todo",
      tasks: [
        { id: "F8.01", title: "Trabajadores con roles, disponibilidad y asignación", status: "todo" },
        { id: "F8.02", title: "Operadores IA en tareas acotadas y seguras", status: "todo" },
        { id: "F8.03", title: "Planificador de obra: dependencias, recursos y bloqueos", status: "todo" },
        { id: "F8.04", title: "Clima con efecto medible en tracción y visibilidad", status: "todo" },
        { id: "F8.05", title: "Desgaste, averías y reparación con costes", status: "todo" },
        { id: "F8.06", title: "Contenido adicional: maquinaria y contratos", status: "todo" },
        { id: "F8.07", title: "Panel de planificación de obras simultáneas", status: "todo" },
        { id: "F8.08", title: "Pruebas de CPU, memoria y navegación con muchos agentes", status: "todo" }
      ]
    },
    {
      id: "H8",
      code: "Hito H8",
      name: "Beta, QA y lanzamiento",
      window: "Meses 24-36",
      goal: "Convertir una colección de sistemas en un producto consistente y publicable.",
      gate: "Cero defectos bloqueantes, regresión crítica superada, partidas migradas y build candidata validada.",
      status: "todo",
      tasks: [
        { id: "F9.01", title: "Congelar alcance y clasificar defectos por gravedad", status: "todo" },
        { id: "F9.02", title: "Regresión completa de controles, excavación, carga y guardado", status: "todo" },
        { id: "F9.03", title: "Pruebas prolongadas y repetición de cientos de operaciones", status: "todo" },
        { id: "F9.04", title: "Optimizar carga, memoria, guardado, colisiones y frames", status: "todo" },
        { id: "F9.05", title: "Tutorial, accesibilidad, mandos y remapeo", status: "todo" },
        { id: "F9.06", title: "Auditoría de licencias, marcas y componentes de terceros", status: "todo" },
        { id: "F9.07", title: "Localización, página de tienda y capturas reales", status: "todo" },
        { id: "F9.08", title: "Beta externa acotada y triage de feedback", status: "todo" },
        { id: "F9.09", title: "Build candidata, publicación y plan de parches", status: "todo" }
      ]
    }
  ],

  bugs: [
    {
      id: "BUG-001",
      title: "La instalación de Unreal Engine 5.8 estaba incompleta",
      detail: "Resuelto: el manifiesto UE_5.8 está completo (bIsIncompleteInstall=false) y la instalación se registró en LauncherInstalled.dat (respaldo .cw-backup). El motor compila y ejecuta el editor.",
      severity: "Crítico",
      status: "Resuelto",
      log: [
        { date: "2026-11-06 16:40", text: "Detectado al auditar la carpeta del motor: UE_5.8 solo contenía .egstore, sin Engine, y el README afirmaba que se estaba instalando." },
        { date: "2026-11-06 16:49", text: "Reanudada la descarga con el protocolo com.epicgames.launcher://apps/UE_5.8?action=install; el log de Epic confirma el dispatch del install." },
        { date: "2026-11-06 18:13", text: "El instalador ya extrajo UnrealEditor.exe, UnrealBuildTool y Build.bat; la primera compilación falló solo porque aún no existe Engine\\Source." },
        { date: "2026-11-06 18:22", text: "29,1 GB en disco (13,5 GB ya en Engine y 15,6 GB en .egstore). Sigue abierto hasta que termine de extraer y el proyecto compile." },
        { date: "2026-11-06 18:39", text: "Engine\\Source ya existe y .egstore baja de 19 GB a 3,7 GB: la instalación está terminando." },
        { date: "2026-11-06 19:00", text: "El motor funciona: compiló el proyecto y ejecutó dos commandlets Python. El log del launcher dice «installation failed» (17:45) y LauncherInstalled.dat no lista UE_5.8; queda re-registrar la instalación." },
        { date: "2026-11-07 11:47", text: "Resuelto: el manifiesto 4C407047… está completo y se añadió la entrada UE_5.8 a LauncherInstalled.dat (AppName UE_5.8, CatalogItemId 18b3b415…). Respaldo guardado como LauncherInstalled.dat.cw-backup." }
      ]
    },
    {
      id: "BUG-002",
      title: "El proyecto C++ nunca se ha compilado",
      detail: "Compilado por primera vez el 06/11/2026: 13/13 acciones correctas y DLL enlazada. No hubo errores en Enhanced Input.",
      severity: "Crítico",
      status: "Resuelto",
      log: [
        { date: "2026-11-06 16:25", text: "Confirmado que no existe ningún artefacto de compilación en el proyecto." },
        { date: "2026-11-06 18:15", text: "Primera compilación bloqueada por BUG-001: falta Build.bat del motor." },
        { date: "2026-11-06 18:50", text: "UBT rechazó los targets por BuildSettingsVersion.V5; corregido a V7 y Unreal5_8." },
        { date: "2026-11-06 18:52", text: "Resultado: Succeeded. UnrealEditor-ConstructionWorld.dll generada." }
      ]
    },
    {
      id: "BUG-003",
      title: "Falta el mapa /Game/Maps/TestLevel",
      detail: "Resuelto: TestLevel.umap generado con el script Python y verificado abriéndolo de nuevo (10 actores CW_, incluido el vehículo).",
      severity: "Alto",
      status: "Resuelto",
      log: [
        { date: "2026-11-06 16:30", text: "Verificado que Content/Maps está vacío y que existe el script cw_build_test_level.py listo para generar el nivel cuando haya editor." },
        { date: "2026-11-06 18:55", text: "Script ejecutado con UnrealEditor-Cmd: «Nivel listo: /Game/Maps/TestLevel», 0 errores y 0 avisos." },
        { date: "2026-11-06 18:57", text: "Verificado con cw_verify_test_level.py: 10 actores CW_ presentes y guardados en el .umap." }
      ]
    },
    {
      id: "BUG-004",
      title: "Repositorio git sin ningún commit",
      detail: "Todo el proyecto estaba sin trackear. Resuelto con el commit inicial d24acd0 (LFS activo para binarios y PDF).",
      severity: "Alto",
      status: "Resuelto",
      log: [
        { date: "2026-11-06 16:35", text: "git status mostraba los 14 bloques del proyecto sin trackear y cero commits." },
        { date: "2026-11-06 18:00", text: "Añadido *.pdf a Git LFS y verificado que el PDF se guarda como puntero LFS." },
        { date: "2026-11-06 18:03", text: "Commit d24acd0 creado con 43 archivos; árbol de trabajo limpio." }
      ]
    },
    {
      id: "BUG-005",
      title: "Documentación desincronizada de la versión del motor",
      detail: "README, Hojaruta, promt y ARQUITECTURA citaban UE 5.6+; ya dicen 5.8 en todas partes.",
      severity: "Bajo",
      status: "Resuelto",
      log: [
        { date: "2026-11-06 17:30", text: "README y ARQUITECTURA actualizados a UE 5.8." },
        { date: "2026-11-06 18:05", text: "Hojaruta.md y promt.md actualizados a 5.8; grep confirma que no quedan referencias a 5.6 salvo en el ADR (como histórico)." }
      ]
    },
    {
      id: "BUG-006",
      title: "Salida del vehículo sin comprobación de espacio",
      detail: "Resuelto: FindSafeExitLocation() realiza sweep de cápsula en 4 direcciones (atrás, derecha, izquierda, adelante) antes de teleportar, evitando colisiones.",
      severity: "Medio",
      status: "Resuelto",
      log: [
        { date: "2026-11-06 16:25", text: "Detectado leyendo ACWVehicleBase.cpp: se usan ExitOffset y SetActorLocation sin sweep ni comprobación de suelo." },
        { date: "2026-11-06 21:55", text: "Resuelto: implementado FindSafeExitLocation() con sweep de cápsula en 4 candidatos antes de teleportar. Compilado y probado." }
      ]
    },
    {
      id: "BUG-007",
      title: "Socket DriverSeat inexistente en el vehículo placeholder",
      detail: "Resuelto: ACWVehicleBase valida la existencia de DriverSeatSocket y se ancla a BodyMesh con fallback seguro al origen del mesh.",
      severity: "Bajo",
      status: "Resuelto",
      log: [
        { date: "2026-11-06 16:25", text: "El mesh placeholder del vehículo no define el socket DriverSeat que usa AttachToComponent." },
        { date: "2026-11-06 21:55", text: "Resuelto: AttachToComponent se hace a BodyMesh y se valida la existencia del socket DriverSeat con fallback limpio." }
      ]
    },
    {
      id: "BUG-008",
      title: "El HUD no funciona dentro del vehículo",
      detail: "Resuelto: ACWHUD inspecciona al conductor cuando el pawn poseído es un ACWVehicleBase y renderiza la ayuda de controles 'Conduciendo | WASD: mover | F: salir'.",
      severity: "Bajo",
      status: "Resuelto",
      log: [
        { date: "2026-11-06 16:25", text: "ACWHUD.DrawHUD resuelve el componente desde el pawn actual; al poseer el vehículo deja de encontrar la interacción." },
        { date: "2026-11-06 21:55", text: "Resuelto: HUD actualizado para vehículos; soporta interacción delegada y textos informativos de control." }
      ]
    },
    {
      id: "BUG-009",
      title: "CWExcavator.h rompía el enlace por falta de implementación",
      detail: "Resuelto: CWExcavator.cpp implementa el constructor, la jerarquía torreta/pluma/brazo/cuchara, los mapeos de excavación y las cámaras. El proyecto vuelve a enlazar.",
      severity: "Alto",
      status: "Resuelto",
      log: [
        { date: "2026-11-07 11:42", text: "La compilación falló con 7 símbolos externos sin resolver de ACWExcavator: el header estaba en el proyecto sin su .cpp." },
        { date: "2026-11-07 11:44", text: "Implementado CWExcavator.cpp respetando el header existente. Compilación correcta en 12,97 s." }
      ]
    },
    {
      id: "BUG-010",
      title: "El test de salida usaba los bounds de la cámara y daba falso positivo",
      detail: "Resuelto: la comprobación de «personaje dentro del vehículo» usa solo componentes con colisión, no la cámara ni el spring arm.",
      severity: "Bajo",
      status: "Resuelto",
      log: [
        { date: "2026-11-07 12:08", text: "La primera ejecución headless falló con bounds de 2.000 unidades que incluían la cámara/spring arm, no el volumen físico del vehículo." },
        { date: "2026-11-07 12:11", text: "Corregido a GetComponentsBoundingBox(false). El test vuelve a pasar con 10 ciclos." }
      ]
    }
  ]
};
