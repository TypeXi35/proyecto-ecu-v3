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

Los sensores se crean una sola vez y nunca se agregan ni se quitan, así que las referencias a ellos siguen válidas mientras exista la Gateway y muestran cada ciclo nuevo.

### Simulador

> Pendiente.

### ECU de Control

> Pendiente.

### Dashboard

> Pendiente.

## Estructura de archivos

Archivos de la ECU Gateway:

```text
include/
├── SignalTypes.hpp     SignalId, SignalState y SignalReading, compartidos por todos los componentes
├── SignalLimits.hpp    Rangos físicos de cada señal y MAX_MISSED_CYCLES
├── Sensor.hpp          Valor, rango, ciclos sin dato y estado de una señal
└── GatewayECU.hpp      ECU Gateway
src/
├── Sensor.cpp
└── GatewayECU.cpp
tests/
├── gateway_tests.cpp   Pruebas de la Gateway
└── DriveScenario.hpp   Guion de 15 ciclos que sustituye al simulador en las pruebas
```

> Pendiente: archivos del simulador, la ECU de Control, el Dashboard, `main.cpp` y `CMakeLists.txt`.

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

> Pendiente.

## Instrucciones de compilación

> Pendiente.

## Instrucciones de ejecución

> Pendiente: ejecución del sistema completo.

### Pruebas de la Gateway

```bash
./build/gateway_tests
```

Imprime `[OK]` o `[FALLO]` por cada comprobación y termina con código 0 solo si todas pasan. Incluye un recorrido de 15 ciclos (`tests/DriveScenario.hpp`) con valores fuera de rango, señales que dejan de llegar y un apagón total.

> Pendiente: depende de que `CMakeLists.txt` genere el ejecutable `gateway_tests`.
