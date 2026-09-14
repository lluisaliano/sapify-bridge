# Sapify Bridge

Conector entre **Shopify y SAP para Mascaró**. El objetivo del proyecto es integrar ambos sistemas y facilitar el intercambio de datos entre la tienda online y el ERP.

## Estado actual

Se ha preparado la base del proyecto:

- Proyecto C++23 con CMake y ejecutable `sapify-bridge`.
- Búsqueda de las dependencias Drogon y nlohmann/json mediante CMake.
- Activación de avisos del compilador y generación de `compile_commands.json`.
- Tareas de Zed para configurar, compilar, ejecutar, recompilar y limpiar el proyecto.
- Comprobación de la compilación y ejecución en Debug y Release, así como de la recompilación y limpieza en Debug.

La integración con Shopify y SAP está pendiente de implementar. Por ahora, el ejecutable imprime `Hello` y termina. Todavía no hay tests configurados.

## Requisitos

- CMake 3.15 o superior.
- Compilador y biblioteca estándar compatibles con C++23, incluido `<print>`.
- Un sistema de compilación disponible para CMake, como Ninja o Make.
- Drogon y nlohmann/json instalados y localizables por CMake.

## Compilar y ejecutar desde la terminal

Ejecutar los siguientes comandos desde la raíz del proyecto.

### Debug

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
./build/sapify-bridge
```

### Release

```sh
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release --parallel
./build/release/sapify-bridge
```

### Recompilar y limpiar

Para recompilar Debug desde cero después de configurarlo:

```sh
cmake --build build --clean-first --parallel
```

Para limpiar los artefactos de Debug conservando la configuración:

```sh
cmake --build build --target clean
```

## Tareas de Zed

Las tareas están definidas en [`.zed/tasks.json`](.zed/tasks.json). Abrir la paleta de comandos de Zed, seleccionar `task: spawn` y buscar `CMake:`.

| Tarea | Acción |
| --- | --- |
| `CMake: configurar (Debug)` | Configura Debug en `build/`. |
| `CMake: compilar (Debug)` | Configura y compila Debug. |
| `CMake: compilar y ejecutar (Debug)` | Configura, compila y ejecuta Debug. |
| `CMake: recompilar (Debug)` | Configura, limpia y vuelve a compilar Debug. |
| `CMake: limpiar (Debug)` | Limpia los artefactos de Debug; requiere haber configurado el proyecto. |
| `CMake: compilar (Release)` | Configura y compila Release en `build/release/`. |
| `CMake: compilar y ejecutar (Release)` | Configura, compila y ejecuta Release. |

Las tareas de configuración y compilación guardan los archivos abiertos antes de ejecutarse. Las tareas de ejecución compilan primero y solo arrancan el programa si la compilación termina correctamente.
