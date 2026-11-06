window.CW = {
  updated: "2026-11-06 17:10",
  project: "Construction World Simulator",
  engine: "Unreal Engine 5.8",
  source: "Plan Maestro v1.0 (04.11.2026) + docs/ARQUITECTURA.md",

  // Lo que estoy haciendo ahora mismo, en orden.
  now: [
    { title: "Extraer y archivar el Plan Maestro (PDF de 35 páginas)", status: "done" },
    { title: "Crear este panel de seguimiento (hoja de ruta + kanban)", status: "doing" },
    { title: "Reanudar la instalación de Unreal Engine 5.8 (estaba a medias)", status: "doing" },
    { title: "Compilar el proyecto C++ por primera vez", status: "todo" },
    { title: "Generar el nivel de pruebas /Game/Maps/TestLevel", status: "todo" },
    { title: "Commit inicial del repositorio", status: "todo" }
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
        { id: "F0.01", title: "Cerrar requisitos de la experiencia básica", status: "done", evidence: "promt.md, Hojaruta.md y docs/ARQUITECTURA.md definen alcance, controles y límites." },
        { id: "F0.02", title: "Auditar hardware y registrar el equipo de desarrollo", status: "doing", note: "Hay que medir CPU/GPU/RAM/disco y tiempos de compilación reales." },
        { id: "F0.03", title: "Instalar herramientas: UE 5.8, Visual Studio 2022, Git, Blender", status: "doing", note: "UE 5.8 se está instalando (reanudado). Falta verificar Blender." },
        { id: "F0.04", title: "Repositorio, .gitignore, Git LFS, README y ADR iniciales", status: "doing", note: "Estructura y ficheros listos; faltan commits, ramas y ADR." },
        { id: "F0.05", title: "Proyecto C++ base: GameMode, Enhanced Input, mapa de pruebas y build mínima", status: "doing", note: "Código y configuración escritos. Nunca compilado." },
        { id: "F0.06", title: "Logging, métricas de FPS/tiempo de frame y convenciones de unidades", status: "todo" },
        { id: "F0.07", title: "Inventario de assets y licencias (excavadora, camión, operario, materiales)", status: "todo" }
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
        { id: "F1.01", title: "Personaje en tercera persona con colisiones y salto", status: "doing", note: "ACWPlayerCharacter escrito; falta compilar y probar." },
        { id: "F1.02", title: "Cámara 1ª/3ª persona con límites, sensibilidad y zoom", status: "doing", note: "Cambio con V implementado; faltan límites y zoom." },
        { id: "F1.03", title: "Sistema IInteractable con prompt, prioridad y distancia máxima", status: "doing", note: "Interfaz + UCWInteractionComponent escritos." },
        { id: "F1.04", title: "Entrar/salir de cabina con puntos de acceso y estados seguros", status: "doing", note: "ACWVehicleBase::TryEnter/ExitVehicle escritos; falta validar." },
        { id: "F1.05", title: "Cambio de mapeos de entrada entre personaje y máquina", status: "doing", note: "Mapping contexts por código; falta probar con mando." },
        { id: "F1.06", title: "HUD discreto con modo de control e instrucciones", status: "doing", note: "ACWHUD en Canvas (provisional), sin UMG." },
        { id: "F1.07", title: "Test con teclado/ratón y mando, cambios repetidos de foco y cámara", status: "todo" }
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
      detail: "Solo existían 19 GB en .egstore y ninguna carpeta Engine. El README decía 'instalándose'. Reanudada la descarga el 06/11.",
      severity: "Crítico",
      status: "En curso"
    },
    {
      id: "BUG-002",
      title: "El proyecto C++ nunca se ha compilado",
      detail: "No hay Binaries, Intermediate ni .sln. Es probable que Enhanced Input por código (MapKey, UInputModifierNegate) dé errores de compilación en 5.8.",
      severity: "Crítico",
      status: "Pendiente"
    },
    {
      id: "BUG-003",
      title: "Falta el mapa /Game/Maps/TestLevel",
      detail: "DefaultEngine.ini apunta a TestLevel pero Content/Maps está vacía: el editor arrancaría sin nivel de pruebas.",
      severity: "Alto",
      status: "Pendiente"
    },
    {
      id: "BUG-004",
      title: "Repositorio git sin ningún commit",
      detail: "Todo el proyecto está sin trackear. Un fallo de disco perdería el trabajo y no hay historial.",
      severity: "Alto",
      status: "Pendiente"
    },
    {
      id: "BUG-005",
      title: "Documentación desincronizada de la versión del motor",
      detail: "README/Hojaruta/promt dicen UE 5.6+ y el .uproject está asociado a 5.8. Confunde los pasos de instalación.",
      severity: "Bajo",
      status: "Pendiente"
    },
    {
      id: "BUG-006",
      title: "Salida del vehículo sin comprobación de espacio",
      detail: "ExitVehicle coloca al personaje en GetActorLocation() + ExitOffset con teleport físico, sin comprobar si hay geometría. Puede incrustarlo en un muro o en el suelo.",
      severity: "Medio",
      status: "Pendiente"
    },
    {
      id: "BUG-007",
      title: "Socket DriverSeat inexistente en el vehículo placeholder",
      detail: "TryEnter adjunta al jugador al socket 'DriverSeat', que no existe en el cubo placeholder. Unreal avisará y usará el origen del actor.",
      severity: "Bajo",
      status: "Pendiente"
    },
    {
      id: "BUG-008",
      title: "El HUD no funciona dentro del vehículo",
      detail: "ACWHUD busca UCWInteractionComponent en el pawn poseído; al conducir, el pawn es el vehículo y se pierde el prompt. No hay HUD de vehículo.",
      severity: "Bajo",
      status: "Pendiente"
    }
  ]
};
