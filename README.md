# proyecto-ecu-v3

> Pendiente: nombre del proyecto.

## Integrantes

> Pendiente.

## Descripción

> Pendiente.

## Arquitectura general

> Pendiente: flujo general del sistema.

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
| Saber cuántos ciclos lleva | `gateway.getCycleCount()`; vale 0 antes del primer ciclo |

Los sensores se crean una sola vez y nunca se agregan ni se quitan, así que las referencias a ellos siguen válidas mientras exista la Gateway y muestran cada ciclo nuevo.

### Simulador

> Pendiente.

### ECU de Control

> Pendiente.

### Dashboard

Muestra en la terminal, en cada ciclo, un tablero de instrumentos con el estado de las señales que entrega la Gateway y, aparte, el estado de la ECU de Control. Solo muestra: no decide ningún estado ni deduce el de la Control a partir de las señales.

| Para qué | Cómo |
| --- | --- |
| Crear el Dashboard (dibuja en la pantalla) | `Dashboard dashboard;` |
| Dibujar un cuadro | `dashboard.render(gateway, control.getState());` |

- Cada cuadro se escribe encima del anterior, sin parpadeo.
- Se dibuja desde el arranque: en el ciclo 0, antes del primer dato, todas las señales aparecen como NO DISPONIBLE y la ECU de Control en INIT.
- El número de ciclo lo da la Gateway con `getCycleCount()`.
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
- **Aviso central:** cuántas señales están inválidas.
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

Archivos de la ECU Gateway y del Dashboard:

```text
include/
├── SignalTypes.hpp     SignalId, SignalState y SignalReading, compartidos por todos los componentes
├── SignalLimits.hpp    Rangos físicos de cada señal y MAX_MISSED_CYCLES
├── Sensor.hpp          Valor, rango, ciclos sin dato y estado de una señal
├── GatewayECU.hpp      ECU Gateway
├── Dashboard.hpp       Dashboard
└── BrailleCanvas.hpp   Lienzo de puntos braille para dibujar los relojes
src/
├── Sensor.cpp
├── GatewayECU.cpp
├── Dashboard.cpp
└── BrailleCanvas.cpp
tests/
├── gateway_tests.cpp   Pruebas de la Gateway
├── dashboard_tests.cpp Pruebas del Dashboard
└── DriveScenario.hpp   Guion de 15 ciclos que sustituye al simulador en las pruebas
```

> Pendiente: archivos del simulador, la ECU de Control, `main.cpp` y `CMakeLists.txt`.

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

> Pendiente.

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

> Pendiente.

### ECU de Control

> Pendiente.

### Dashboard

| Elemento | Dónde | Para qué |
| --- | --- | --- |
| `std::vector` | `Dashboard.cpp`, `BrailleCanvas` | Lista de barras, celdas de los relojes y puntos del lienzo |
| `std::array` | `BrailleCanvas.cpp` | Bit de cada punto dentro de su celda braille |
| `std::for_each` | `Dashboard::render` | Dibujar una barra por señal y escribir cada fila del cuadro |
| `std::count_if` | `displayWidth` en `Dashboard.cpp` | Contar columnas y no bytes en el texto con caracteres de cuadro y braille |
| `std::clamp` | `scaleFraction` en `Dashboard.cpp` | Dejar la aguja en el tope cuando el valor está fuera de rango |
| `std::optional` | `scaleFraction` en `Dashboard.cpp` | Indicar que no hay aguja que dibujar (sin dato o NaN) |
| `std::ostringstream`, `std::fixed`, `std::setprecision` | Formato de valores | Valores con decimales fijos |

Cada señal se localiza con `GatewayECU::findSensor` (`std::find_if`). En las pruebas también se usan `std::find_if`, `std::all_of`, `std::none_of`, `std::count`, `std::ostringstream` y `std::istringstream`.

## Instrucciones de compilación

> Pendiente.

## Instrucciones de ejecución

> Pendiente: ejecución del sistema completo.

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

> Pendiente: depende de que `CMakeLists.txt` genere los ejecutables `gateway_tests` y `dashboard_tests`.
