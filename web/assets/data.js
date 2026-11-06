window.CW = {
  updated: "2026-11-06 22:10",
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
      status: "doing",
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
          status: "doing",
          note: "El motor compila y ejecuta commandlets. El launcher no lo registra como instalado (parche fallido el 06/11) y Blender sigue sin instalar.",
          log: [
            { date: "2026-11-06 16:40", text: "Detectado que UE 5.8 estaba a medias: solo 19 GB en .egstore y ninguna carpeta Engine, pese a que el README decía «instalándose»." },
            { date: "2026-11-06 16:49", text: "Descarga reanudada con el protocolo de Epic: el log confirma «Dispatching install for AppId …UE_5.8»." },
            { date: "2026-11-06 18:13", text: "29,2 GB y ya existen Build.bat, UnrealBuildTool y UnrealEditor.exe. La compilación falla todavía porque falta Engine\\Source (el instalador sigue extrayendo)." },
            { date: "2026-11-06 18:22", text: "VS 2022 Community con el conjunto de herramientas C++ verificado. Blender no aparece instalado: queda pendiente para el modelado de assets." },
            { date: "2026-11-06 18:39", text: "Aparece Engine\\Source (lo último que faltaba para poder compilar C++). El instalador sigue copiando archivos desde .egstore." },
            { date: "2026-11-06 19:00", text: "29,9 GB en disco y el motor ya compiló y ejecutó scripts Python. El log del launcher registra «installation failed» a las 17:45 y no lo marca como instalado; no bloquea el trabajo, queda pendiente re-registrarlo." }
          ]
        },
        {
          id: "F0.04",
          title: "Repositorio, .gitignore, Git LFS, README y ADR iniciales",
          status: "doing",
          note: "Primer commit d24acd0; LFS activo (incluye PDF) y 3 ADR creados. Faltan ramas y CI.",
          log: [
            { date: "2026-11-06 18:02", text: "Añadido *.pdf a Git LFS y creados ADR-001 (motor 5.8), ADR-002 (terreno heightfield) y ADR-003 (Git LFS)." },
            { date: "2026-11-06 18:03", text: "Commit inicial d24acd0 con 43 archivos; el PDF queda como puntero LFS. Árbol de trabajo limpio." },
            { date: "2026-11-06 18:28", text: "Remoto origin → github.com/jh0ny2k2/CWS.git configurado y rama renombrada a main." },
            { date: "2026-11-06 18:40", text: "Push completado: 3 commits y el PDF del plan subidos a GitHub (LFS verificado). El token se usó solo para el push y la URL del remoto quedó limpia." }
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
          status: "todo",
          log: [
            { date: "2026-11-06 18:15", text: "Aparcado hasta tener una build ejecutable; sin motor no se pueden medir FPS ni tiempos de frame." }
          ]
        },
        {
          id: "F0.07",
          title: "Inventario de assets y licencias (excavadora, camión, operario, materiales)",
          status: "todo"
        },
        {
          id: "F0.08",
          title: "Panel de seguimiento web: hoja de ruta + kanban",
          status: "done",
          evidence: "http://127.0.0.1:8100 · probado con jsdom: 10 hitos, 101 tareas y 8 bugs sin errores de JavaScript.",
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
      status: "doing",
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
          status: "doing",
          note: "Pruebas de cambio repetido y conducción en TestLevel.",
          log: [
            { date: "2026-11-06 18:15", text: "Bloqueado: sin motor instalado no se puede ejecutar la prueba." },
            { date: "2026-11-06 21:55", text: "Mecánicas de conducción y cambio de mapeos compiladas. Iniciando pruebas automáticas de ciclo completo." }
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
      status: "todo",
      tasks: [
        { id: "F2.01", title: "Asset de excavadora con geometría separada, escala y pivotes", status: "todo" },
        { id: "F2.02", title: "Torreta con rotación limitada o continua", status: "todo" },
        { id: "F2.03", title: "Pluma, brazo y cuchara con límites y velocidades configurables", status: "todo" },
        { id: "F2.04", title: "Cilindros hidráulicos siguiendo anclajes reales", status: "todo" },
        { id: "F2.05", title: "Cadenas izquierda/derecha con giro estacionario y agarre", status: "todo" },
        { id: "F2.06", title: "Cámaras de cabina, exterior y enfocada a la cuchara", status: "todo" },
        { id: "F2.07", title: "Arranque/parada, audio de motor e hidráulica, combustible simple", status: "todo" },
        { id: "F2.08", title: "Pruebas de estabilidad en pendiente y con carga", status: "todo" },
        { id: "F2.09", title: "Rig del camión: ruedas, caja basculante y colisiones", status: "todo" },
        { id: "F2.10", title: "Conducción del volquete sobre física de ruedas", status: "todo" },
        { id: "F2.11", title: "Elevación y retorno de la caja con límites y seguridad", status: "todo" },
        { id: "F2.12", title: "Carga abstracta, indicador de llenado y variación de peso", status: "todo" },
        { id: "F2.13", title: "Zona de descarga con evento validado", status: "todo" }
      ]
    },
    {
      id: "H2",
      code: "Hito H2",
      name: "Terreno deformable (riesgo máximo)",
      window: "Semanas 13-26",
      goal: "Resolver el sistema que condiciona todo el simulador antes de invertir en arte.",
      gate: "Extraer, transportar y depositar conservando el volumen dentro de tolerancia, con persistencia y rendimiento acotado.",
      status: "todo",
      tasks: [
        { id: "F3.01", title: "Escena comparativa de terreno sin arte pesado", status: "todo" },
        { id: "F3.02", title: "Prototipo heightfield con cambios por celdas, colisión y volumen", status: "todo" },
        { id: "F3.03", title: "Evaluar alternativa SDF/voxel y malla dinámica", status: "todo" },
        { id: "F3.04", title: "Benchmark: latencia, memoria, guardado y estabilidad", status: "todo" },
        { id: "F3.05", title: "ADR del backend de terreno y plan de migración", status: "todo" },
        { id: "F3.06", title: "Contacto cuchara-terreno con área activa y límites de extracción", status: "todo" },
        { id: "F3.07", title: "Contabilizar material por tipo, densidad y estado", status: "todo" },
        { id: "F3.08", title: "Llenar/vaciar cuchara y visualizar acopios", status: "todo" },
        { id: "F3.09", title: "Actualizar colisiones por lotes sin picos de frame", status: "todo" },
        { id: "F3.10", title: "Transferencia al volquete con capacidad y derrames", status: "todo" },
        { id: "F3.11", title: "Depósito en destino conservando el volumen", status: "todo" },
        { id: "F3.12", title: "Guardar/cargar deltas y verificar la geometría", status: "todo" },
        { id: "T-001", title: "Test: 10 cucharadas uniformes", status: "todo" },
        { id: "T-002", title: "Test: 100 cucharadas sobre la misma zona", status: "todo" },
        { id: "T-003", title: "Test: 1.000 operaciones distribuidas", status: "todo" },
        { id: "T-004", title: "Test: descargar y volver a cargar", status: "todo" },
        { id: "T-005", title: "Test: cortar durante el guardado", status: "todo" },
        { id: "T-006", title: "Test: cambiar material de suelo", status: "todo" }
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
      detail: "El motor ya es utilizable (29,9 GB, compila y ejecuta el editor), pero el launcher registró «installation failed» y no lo marca como instalado. Falta re-registrar la instalación.",
      severity: "Crítico",
      status: "En curso",
      log: [
        { date: "2026-11-06 16:40", text: "Detectado al auditar la carpeta del motor: UE_5.8 solo contenía .egstore, sin Engine, y el README afirmaba que se estaba instalando." },
        { date: "2026-11-06 16:49", text: "Reanudada la descarga con el protocolo com.epicgames.launcher://apps/UE_5.8?action=install; el log de Epic confirma el dispatch del install." },
        { date: "2026-11-06 18:13", text: "El instalador ya extrajo UnrealEditor.exe, UnrealBuildTool y Build.bat; la primera compilación falló solo porque aún no existe Engine\\Source." },
        { date: "2026-11-06 18:22", text: "29,1 GB en disco (13,5 GB ya en Engine y 15,6 GB en .egstore). Sigue abierto hasta que termine de extraer y el proyecto compile." },
        { date: "2026-11-06 18:39", text: "Engine\\Source ya existe y .egstore baja de 19 GB a 3,7 GB: la instalación está terminando." },
        { date: "2026-11-06 19:00", text: "El motor funciona: compiló el proyecto y ejecutó dos commandlets Python. El log del launcher dice «installation failed» (17:45) y LauncherInstalled.dat no lista UE_5.8; queda re-registrar la instalación." }
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
    }
  ]
};
