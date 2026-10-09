# proyecto-ecu-v3

> ECU-Hito IV

## Integrantes

> Luis Alan Morales Trejo
> Hector Eduardo Romero Altamirano
> Francisco Ebgueny Pérez José

## Descripción

> Proyecto que simula la ejecucion de ECU en un Automovil
> recibiendo datos a traves de una simulacion, procesandolos
> e imprimiendo los resultados a un dashboard en terminal

## Arquitectura general

El sistema son cuatro componentes en cadena. Cada uno hace una sola cosa y solo conoce al anterior:

```text
VehicleSimulator ──SignalReading──▶ GatewayECU ──Sensor&──▶ ControlECU ──ECUData&──▶ Dashboard
   genera señales      (vector)     ¿es confiable?          ¿qué hacer?               muestra
```

| Componente | Pregunta que responde | Entrada | Salida |
| --- | --- | --- | --- |
| Simulador | ¿Qué mide el vehículo? | — | `std::vector<SignalReading>` con las lecturas del ciclo, con fallas simuladas |
| ECU Gateway | ¿La señal es confiable? | Lecturas del ciclo | 6 `Sensor` con valor y `SignalState` |
| ECU de Control | ¿En qué estado debe operar? | Referencias a los sensores de la Gateway | `ECUData`: sensores, `ECUState` y ciclo |
| Dashboard | — (solo muestra) | `ECUData` | Cuadro en la terminal |

### Ciclo de ejecución

`main.cpp` crea los cuatro componentes y los conecta una sola vez; después solo repite el ciclo:

1. Dibuja el ciclo 0: señales NO DISPONIBLE y la Control en `INIT`.
2. Cada 500 ms:
   1. `simulator.updateSignal()` genera las lecturas nuevas.
   2. `gateway.processCycle(simulator.exposeSignals())` las valida.
   3. `control.runControlCycle()` decide el estado.
   4. `dashboard.render()` dibuja el resultado.
3. Se detiene cuando la Control llega a `SAFE_STATE`, después de dibujar ese cuadro.

### Datos compartidos por referencia

Las señales no se copian entre componentes:

- La Gateway es dueña de los 6 `Sensor` y nunca los agrega ni los quita.
- La Control guarda en su `ECUData` referencias a esos sensores y a su propio estado y contador de ciclos.
- El Dashboard guarda una referencia a ese `ECUData`.

Por eso se conectan una vez al arrancar y en cada ciclo todos ven los datos actuales sin pasárselos de nuevo. Las dependencias van en un solo sentido: la Gateway no conoce a la Control y el Dashboard no conoce a la Gateway.

### ECU Gateway

Recibe las señales del vehículo en cada ciclo, las guarda, valida su rango, supervisa que se sigan actualizando y marca cada una como válida o inválida. Su trabajo es decidir si una señal es confiable.

No controla el vehículo, no determina el estado de operación, no ejecuta la máquina de estados ni decide entrar a `SAFE_STATE`: eso es de la ECU de Control. Tampoco imprime nada; mostrar es tarea del Dashboard.

La Gateway crea sus propios sensores y es dueña de ellos (`std::vector<Sensor>`). Los demás componentes la usan así:

| Para qué | Cómo |
| --- | --- |
| Crear la Gateway con sus 6 sensores | `GatewayECU gateway;` |
| Entregar las lecturas de un ciclo | `gateway.processCycle(readings);` con `const std::vector<SignalReading>&` |
| Leer todos los sensores | `gateway.getSensors()` devuelve `const std::vector<Sensor>&` |
| Buscar una señal | `gateway.findSensor(SignalId::TEMPERATURE)` devuelve `const Sensor&`; lanza `std::out_of_range` si el ID no existe |
| Contar las señales inválidas | `gateway.countInvalidSignals()` cuenta las que no están en `VALID` |

Los sensores se crean una sola vez y nunca se agregan ni se quitan, así que las referencias a ellos siguen válidas mientras exista la Gateway y muestran cada ciclo nuevo.

### Simulador

El componente `VehicleSimulator` simula el comportamiento de un vehículo mediante la generación y actualización de seis señales: velocidad, RPM, temperatura, acelerador, voltaje de batería y presión de aceite.

En cada ciclo, actualiza los valores de las señales considerando las relaciones entre ellas y pequeñas variaciones aleatorias para representar un comportamiento dinámico.

También permite generar fallas de manera aleatoria para comprobar cómo responde el sistema ante señales fuera de rango o que dejan de recibirse.

**Funcionamiento principal:**

| Función | Descripción |
| --- | --- |
| `VehicleSimulator()` | Inicializa los valores de las señales y configura el generador aleatorio. |
| `updateSignal()` | Actualiza el comportamiento normal de las seis señales y administra las fallas. |
| `exposeSignals()` | Devuelve un `std::vector<SignalReading>` con las señales disponibles del ciclo, considerando las fallas activas. |

**Generación de señales:**

- **Acelerador:** cambia aleatoriamente en cada ciclo y se mantiene entre 0 y 100 %.
- **Velocidad:** aumenta o disminuye progresivamente según la posición del acelerador.
- **RPM:** se calculan considerando la velocidad, el acelerador y una pequeña variación aleatoria.
- **Temperatura:** evoluciona gradualmente hacia una temperatura objetivo que depende del acelerador.
- **Voltaje de batería:** se mantiene alrededor de 13.8 V con pequeñas variaciones.
- **Presión de aceite:** depende de las RPM y presenta pequeñas variaciones aleatorias.

**Simulación de fallas:**

En cada ciclo existe un 5 % de probabilidad de intentar generar una nueva falla. Cuando esto ocurre, se selecciona aleatoriamente una de las seis señales, el tipo de falla y su duración.

| Tipo de falla | Comportamiento |
| --- | --- |
| `OUT_OF_RANGE` | Sustituye el valor normal por uno fuera del rango válido de la señal. |
| `MISSING` | Omite la señal del vector de lecturas, simulando que el dato no fue recibido. |

Las fallas tienen una duración aleatoria de 3 a 10 ciclos. El simulador almacena las fallas en un `std::vector<SignalFault>`, donde registra la señal afectada, el tipo de falla y los ciclos restantes.

Una señal no puede tener más de una falla activa simultáneamente. Cuando termina una falla, su registro se marca como `NONE` y puede reutilizarse posteriormente.

**Funciones internas:**

| Función | Descripción |
| --- | --- |
| `updateFaults()` | Actualiza la duración de las fallas activas e intenta generar nuevas fallas. |
| `hasActiveFault()` | Comprueba si una señal ya tiene una falla activa. |
| `getFaultType()` | Obtiene el tipo de falla activa de una señal o devuelve `NONE`. |
| `addSignal()` | Agrega una lectura normal, una lectura fuera de rango u omite la señal según su falla activa. |

### ECU de Control

Lee los sensores de la Gateway, evalúa en cada ciclo si hay fallas y ejecuta la máquina de estados (ver Estados de la ECU de Control). No valida señales ni dibuja nada.

Al construirse busca sus 6 sensores en la Gateway y guarda referencias a ellos en un `ECUData` (`include/ControlData.hpp`), junto con referencias a su estado actual y a su contador de ciclos. Ese `ECUData` es todo lo que la Control expone hacia afuera:

| Para qué | Cómo |
| --- | --- |
| Crear la Control | `ControlECU control(gateway.getSensors());`; lanza `std::out_of_range` si falta un sensor |
| Ejecutar un ciclo | `control.runControlCycle();` después de `gateway.processCycle(...)` |
| Leer sensores, estado y ciclo | `control.getControlData()` devuelve `const ECUData&` |

```cpp
struct ECUData {
    ControlSensors sensors;          // speed, rpm, temperature, batteryVoltage, throttle, oilPressure
    const ECUState& currentState;
    const unsigned int& cycleCount;  // ciclos ejecutados; vale 0 antes del primero
};
```

Como todo son referencias, quien guarde el `ECUData` ve siempre los valores del ciclo actual sin pedirlos de nuevo.

### Dashboard

Muestra en la terminal, en cada ciclo, un tablero de instrumentos con el estado de las señales y, aparte, el estado de la ECU de Control. Todo lo que dibuja sale del `ECUData` de la Control; no conoce a la Gateway. Solo muestra: no decide ningún estado ni deduce el de la Control a partir de las señales.

| Para qué | Cómo |
| --- | --- |
| Crear el Dashboard (dibuja en la pantalla) | `Dashboard dashboard(control.getControlData());` |
| Dibujar un cuadro | `dashboard.render();` |

- Recibe el `ECUData` una sola vez al construirse; como guarda referencias, cada `render()` muestra el ciclo actual.
- Cada cuadro se escribe encima del anterior, sin parpadeo.
- Se dibuja desde el arranque: en el ciclo 0, antes del primer dato, todas las señales aparecen como NO DISPONIBLE y la ECU de Control en INIT.
- El número de ciclo es `ECUData::cycleCount`, el contador de la ECU de Control, que avanza una vez por cada `runControlCycle()`.
- Una señal NO DISPONIBLE muestra `---`, porque al arrancar el 0.0 no es una lectura y el último valor recibido ya no es confiable. Una señal FUERA DE RANGO muestra el valor que llegó, completo aunque no quepa en su columna.
- Los textos van en español sin acentos y la temperatura usa `C` en lugar de `°C`.

Ejemplo del ciclo 8 del guion de pruebas, con la ECU de Control en DEGRADED (aquí sin colores):

```text
╭─ ECU GATEWAY / CONTROL ──────────────────────────────────────────── CICLO 8 ─╮
│                                                                              │
│           ⣀⣤⠤⠒⠒⠒⠒⠢⢤⣄⡀                                   ⣀⡤⠤⠒⠒⡖⠒⠢⠤⣄⡀          │
│        ⢀⡴⠚⠁⠘      ⠘ ⠙⠲⣄                              ⢀⡴⠚⠙⠄   ⠁   ⠜⠙⠲⣄        │
│       ⣰⠋   100  150   ⠈⢳⡀                           ⣰⢏  ⡀ 3  4 5    ⢈⢷⡀      │
│      ⣰⠧⠄               ⠤⢷⡀       ▲ ATENCION        ⣰⠃ ⠁ 2⢦⡀      6  ⠁ ⢳⡀     │
│      ⡇  50         200   ⡇                         ⣇⣀     ⠙⢦⡀        ⢀⣀⡇     │
│     ⢸⠁      ⢀⣀⠤⠄         ⢹         2 de 6         ⢸⠁   1    ⠙⠆    7    ⢹     │
│     ⠈⡇  ⡠0⠒⠋⠉      250   ⡏       invalidas        ⠈⡇   0          8    ⡏     │
│      ⠙⠉       -5       ⠈⠙⠁                         ⠙⠉      2330      ⠈⠙⠁     │
│              km/h                                           rpm              │
│           VELOCIDAD                                     RPM x1000            │
│         FUERA DE RANGO                                    VALIDA             │
│                                                                              │
│ TEMPERATURA   ███████████████░░░░░░░    89.6 C   VALIDA                      │
│ ACELERADOR    ███████▋░░░░░░░░░░░░░░    35.0 %   VALIDA                      │
│ BATERIA       ░░░░░░░░░░░░░░░░░░░░░░     --- V   NO DISPONIBLE · 5 ciclos    │
│ ACEITE        █████░░░░░░░░░░░░░░░░░     2.3 bar VALIDA · 1 ciclo            │
├──────────────────────────────────────────────────────────────────────────────┤
│                                                  ECU DE CONTROL   DEGRADED   │
╰──────────────────────────────────────────────────────────────────────────────╯
```

- **Relojes:** velocidad y RPM tienen aguja, y su escala sale del rango válido de cada señal. Fuera de rango, la aguja se queda en el tope; sin dato o con NaN no hay aguja y el arco se apaga.
- **Aviso central:** cuántas de las 6 señales de `ECUData::sensors` no están en `VALID`.
- **Barras:** temperatura, acelerador, batería y presión de aceite, con su valor, su estado y los ciclos sin dato cuando hay alguno. La barra marca la posición dentro del rango válido.
- **ECU de Control:** etiqueta con fondo de color, separada de las señales.

| Color | Señal (aguja, valor y estado) | Aviso central | ECU de Control |
| --- | --- | --- | --- |
| Verde | VALIDA | Ninguna inválida | OPERATIONAL |
| Ámbar | NO DISPONIBLE | Algunas inválidas | DEGRADED |
| Rojo | FUERA DE RANGO | Todas inválidas | SAFE_STATE |
| Gris | | | INIT |

Requiere una terminal con UTF-8, de al menos 80 × 24 y con una fuente que tenga caracteres braille, como DejaVu Sans Mono.

## Estructura de archivos

```text
CMakeLists.txt             Biblioteca ecu_core, el ejecutable ecu_simulator y las dos pruebas
include/
├── SignalTypes.hpp        SignalId, SignalState y SignalReading, compartidos por todos los componentes
├── SignalLimits.hpp       Rangos físicos de cada señal y MAX_MISSED_CYCLES
├── Sensor.hpp             Valor, rango, ciclos sin dato y estado de una señal
├── VehicleSimulator.hpp   Declaración del simulador y estructura SignalFault
├── GatewayECU.hpp         ECU Gateway
├── ECUState.hpp           Estados de la ECU de Control
├── ControlData.hpp        ControlSensors y ECUData, lo que la Control expone al Dashboard
├── ControlECU.hpp         ECU de Control
├── Dashboard.hpp          Dashboard
└── BrailleCanvas.hpp      Lienzo de puntos braille para dibujar los relojes
src/
├── main.cpp               Ciclo principal: simulador → Gateway → Control → Dashboard
├── Sensor.cpp
├── VehicleSimulator.cpp   Generación de señales y simulación de fallas
├── GatewayECU.cpp
├── ControlECU.cpp
├── Dashboard.cpp
└── BrailleCanvas.cpp
tests/
├── gateway_tests.cpp      Pruebas de la Gateway
├── dashboard_tests.cpp    Pruebas del Dashboard
└── DriveScenario.hpp      Guion de 15 ciclos que sustituye al simulador en las pruebas
```

## Señales implementadas

La Gateway supervisa 6 señales: las 5 obligatorias, y el acelerador, del que dependen las RPM y la velocidad en la simulación.

| Señal | `SignalId` | Unidad | Rango válido |
| --- | --- | --- | --- |
| Velocidad | `SPEED` | km/h | 0 a 250 |
| RPM | `RPM` | rpm | 0 a 8000 |
| Temperatura | `TEMPERATURE` | °C | −40 a 150 |
| Acelerador | `THROTTLE` | % | 0 a 100 |
| Voltaje de batería | `BATTERY_VOLTAGE` | V | 9 a 16 |
| Presión de aceite | `OIL_PRESSURE` | bar | 0 a 10 |

Los rangos están en `include/SignalLimits.hpp`.

## Criterios de validez

Cada señal tiene uno de estos estados (`SignalState`):

| Estado | Significa |
| --- | --- |
| `VALID` | El último valor recibido está dentro de su rango y la señal se sigue actualizando. |
| `OUT_OF_RANGE` | El último valor recibido está fuera de su rango. |
| `NOT_AVAILABLE` | Todavía no llega el primer dato, o la señal dejó de actualizarse (ver Criterio temporal). |

- Los límites cuentan como válidos: 150 °C es `VALID` y 150.1 °C es `OUT_OF_RANGE`.
- NaN e infinito son `OUT_OF_RANGE`. La validación pregunta "¿está dentro del rango?" y cualquier comparación con NaN es falsa, así que nunca pasa como válido.
- Un valor fuera de rango se guarda tal cual, para que la ECU de Control y el Dashboard vean qué llegó.
- La recuperación es inmediata, el primer dato dentro de rango devuelve la señal a `VALID`.

### Por qué estos rangos

Son límites físicos: lo que un sensor de ese tipo puede reportar. Un valor fuera de ellos no significa que el vehículo esté en problemas, sino que la lectura no es creíble. Los umbrales de operación, como una temperatura crítica, los decide la ECU de Control. Por eso 0 a 110 °C no es un rango de la Gateway: una lectura de 130 °C es creíble y queda `VALID`, y es la ECU de Control la que decide si es crítica.

- Velocidad, 0 a 250 km/h: la máxima de un auto de calle.
- RPM, 0 a 8000: arriba del corte de un motor típico.
- Temperatura, −40 a 150 °C: rango típico de un sensor de temperatura de refrigerante.
- Acelerador, 0 a 100 %: posición del pedal.
- Voltaje de batería, 9 a 16 V: rango en que opera la electrónica de un sistema de 12 V.
- Presión de aceite, 0 a 10 bar: rango típico de un sensor de presión de aceite.

## Criterio temporal

La Gateway trabaja por ciclos. Si una señal no viene en las lecturas de un ciclo, se considera que no llegó.

- Cada sensor cuenta sus ciclos seguidos sin dato. Cuando llega un dato, el contador vuelve a 0.
- El límite es `MAX_MISSED_CYCLES = 3`, igual para todas las señales (`include/SignalLimits.hpp`).
- Se toleran hasta 3 ciclos seguidos sin dato. En el 4.º la señal pasa a `NOT_AVAILABLE`, con la regla `ciclosSinActualizar > límite` de la diapositiva 19.
- Mientras se tolera, la señal conserva su último valor y su estado. Una señal `OUT_OF_RANGE` que deja de llegar sigue `OUT_OF_RANGE` hasta el 4.º ciclo.
- El sensor nunca se elimina, solo cambia su estado, y conserva el último valor recibido.
- Al arrancar, todas las señales están en `NOT_AVAILABLE` con valor 0.0 hasta que llega su primer dato.

### Por qué 3 ciclos

Tolera pérdidas aisladas de 1 a 3 ciclos sin falsas alarmas y detecta una pérdida real en el 4.º ciclo.

## Estados de la ECU de Control

| Estado | Significa |
| --- | --- |
| `INIT` | Arranque; todavía no se ejecuta ningún ciclo. |
| `OPERATIONAL` | Todas las señales son válidas, están al día y son coherentes. |
| `DEGRADED` | Alguna señal no crítica falla; el vehículo sigue, con aviso. Se recupera sola. |
| `SAFE_STATE` | Falla crítica. Es final: no se sale de él y el programa termina. |

En cada ciclo, `runControlCycle()` hace tres revisiones en este orden y se queda con la primera que falla:

1. **Falla crítica** (`hasCriticalFault`) → `SAFE_STATE`: temperatura o voltaje de batería en `OUT_OF_RANGE` o `NOT_AVAILABLE`.
2. **Degradado** (`isDegraded`) → `DEGRADED`:
   - velocidad, RPM, acelerador o presión de aceite no están en `VALID`;
   - velocidad, RPM o acelerador no llegaron en este ciclo (`getMissedCycles() > 0`). La Gateway las deja en `VALID` con su valor anterior hasta 3 ciclos, pero ese valor viejo no sirve para comparar;
   - acelerador arriba de 80 % con menos de 500 rpm.
3. **Incoherencia** (`isIncoherent`) → `SAFE_STATE`: la velocidad se aleja más de 15 km/h de la esperada según RPM y acelerador, con la misma relación del simulador (`rpm = 800 + 22 · velocidad + 18 · acelerador`).

El orden importa: la coherencia solo se revisa cuando la revisión 2 ya confirmó que velocidad, RPM y acelerador son válidas y del ciclo actual.

| Desde | Si hay falla crítica | Si está degradado | Si es incoherente | Si todo está bien |
| --- | --- | --- | --- | --- |
| `INIT` | `SAFE_STATE` | `DEGRADED` | `SAFE_STATE` | `OPERATIONAL` |
| `OPERATIONAL` | `SAFE_STATE` | `DEGRADED` | `SAFE_STATE` | `OPERATIONAL` |
| `DEGRADED` | `SAFE_STATE` | `DEGRADED` | `SAFE_STATE` | `OPERATIONAL` |
| `SAFE_STATE` | `SAFE_STATE` | `SAFE_STATE` | `SAFE_STATE` | `SAFE_STATE` |

## STL utilizado

### ECU Gateway

| Elemento | Dónde | Para qué |
| --- | --- | --- |
| `std::vector` | `GatewayECU` | Guardar los 6 sensores |
| `std::for_each` | `GatewayECU::processCycle` | Recorrer los sensores en cada ciclo |
| `std::find_if` | `GatewayECU::processCycle` | Buscar la lectura de cada sensor entre las lecturas del ciclo |
| `std::find_if` | `GatewayECU::findSensor` | Buscar un sensor por su `SignalId` |
| `std::count_if` | `GatewayECU::countInvalidSignals` | Contar las señales que no están en `VALID` |

En las pruebas también se usan `std::optional` (una señal que no llegó en el guion), `std::all_of`, `std::count_if` y `std::remove_if`.

### Simulador

| Elemento | Dónde | Para qué |
| --- | --- | --- |
| `std::vector` | `VehicleSimulator` | Almacenar las fallas y devolver las lecturas generadas. |
| `std::mt19937` | `VehicleSimulator` | Generar números pseudoaleatorios. |
| `std::normal_distribution` | `updateSignal()` | Agregar pequeñas variaciones a las señales. |
| `std::uniform_real_distribution` | `updateSignal()`, `updateFaults()` | Simular cambios del acelerador y determinar la probabilidad de falla. |
| `std::uniform_int_distribution` | `updateFaults()` | Seleccionar la señal afectada, el tipo de falla y su duración. |
| `std::clamp` | `updateSignal()` | Mantener el acelerador y las RPM dentro de sus límites definidos. |
| `std::max` | `updateSignal()` | Evitar valores negativos de velocidad y presión de aceite. |

### ECU de Control

| Elemento | Dónde | Para qué |
| --- | --- | --- |
| `std::vector` | `ControlECU::ControlECU`, `ControlECU::getSensor` | Recibir los sensores de la Gateway y buscar cada uno por su `SignalId` |
| `std::out_of_range` | `ControlECU::getSensor` | Avisar si la Gateway no tiene un sensor que la Control necesita |
| `std::abs` | `ControlECU::isIncoherent` | Diferencia entre la velocidad medida y la esperada |

### Dashboard

| Elemento | Dónde | Para qué |
| --- | --- | --- |
| `std::vector` | `Dashboard.cpp`, `BrailleCanvas` | Lista de barras, celdas de los relojes y puntos del lienzo |
| `std::array` | `BrailleCanvas.cpp` | Bit de cada punto dentro de su celda braille |
| `std::for_each` | `Dashboard::render` | Dibujar una barra por señal y escribir cada fila del cuadro |
| `std::count_if` | `Dashboard::render` | Contar las señales que no están en `VALID` para el aviso central |
| `std::count_if` | `displayWidth` en `Dashboard.cpp` | Contar columnas y no bytes en el texto con caracteres de cuadro y braille |
| `std::clamp` | `scaleFraction` en `Dashboard.cpp` | Dejar la aguja en el tope cuando el valor está fuera de rango |
| `std::optional` | `scaleFraction` en `Dashboard.cpp` | Indicar que no hay aguja que dibujar (sin dato o NaN) |
| `std::ostringstream`, `std::fixed`, `std::setprecision` | Formato de valores | Valores con decimales fijos |

Cada señal se lee directo de `ECUData::sensors`, sin buscarla. En las pruebas también se usan `std::find_if`, `std::all_of`, `std::none_of`, `std::count`, `std::ostringstream` y `std::istringstream`.

## Instrucciones de compilación

Requiere CMake 3.16 o más reciente y un compilador con C++17.

```bash
cmake -S . -B build
cmake --build build
```

Con Visual Studio los ejecutables quedan en `build/Debug/` en lugar de `build/`.

## Instrucciones de ejecución

### Sistema completo

```bash
./build/ecu_simulator
```

Cada 500 ms ejecuta un ciclo (simulador → Gateway → ECU de Control) y redibuja el Dashboard. Corre hasta que la ECU de Control entra a `SAFE_STATE`: dibuja ese último cuadro, imprime `ECU de Control en SAFE_STATE, simulación detenida` y termina con código 1. También se puede detener antes con Ctrl+C. En Windows cambia la consola a UTF-8 para que se vean los caracteres de cuadro y braille.

### Pruebas de la Gateway

```bash
./build/gateway_tests
```

Imprime `[OK]` o `[FALLO]` por cada comprobación y termina con código 0 solo si todas pasan. Incluye un recorrido de 15 ciclos (`tests/DriveScenario.hpp`) con valores fuera de rango, señales que dejan de llegar y un apagón total.

### Pruebas del Dashboard

```bash
./build/dashboard_tests
```

Igual que las de la Gateway, imprime `[OK]` o `[FALLO]` y termina con código 0 solo si todas pasan. Compara cuadros completos con el texto esperado (arranque, fallas y apagón), revisa que en cada ciclo del guion el cuadro mida 21 líneas de 80 columnas, el color de las agujas, del aviso y de la etiqueta de la ECU de Control, valores fuera de lo común y el lienzo braille.
