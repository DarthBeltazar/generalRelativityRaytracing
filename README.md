# General Relativity Raytracing
![Облёт чёрной дыры](docs/flyby.gif)
## Структура проекта

<!-- tree:start -->
```
├── docs/
│   └── flyby.gif
├── src/
│   ├── core/
│   │   ├── Constants.h           # константы: PI, RECIP_PI
│   │   └── Vec3.h/.cpp           # базовые типы: Vec3 (x, y, z)
│   ├── physics/
│   │   └── physics.h/.cpp        # интегрирование геодезического уравнения, трассировка одного луча (traceRay), HitInfo
│   ├── render/
│   │   ├── AccretionDisc.h/.cpp  # цвет точки аккреционного диска (discColor)
│   │   ├── Background.h/.cpp     # загрузка background.exr и сэмплирование фона по направлению луча
│   │   ├── Camera.h/.cpp         # базис и генерация луча
│   │   ├── PerlinNoise.h/.cpp    # процедурный шум, используется для текстуры диска
│   │   └── Renderer.h/.cpp       # трассировка всех лучей кадра (traceRays), запись PNG (writeImage/renderImage)
│   ├── tools/
│   │   └── PixelStepTrace.cpp    # диагностика одного луча, сохранение в csv
│   └── main.cpp        # точка входа: рендер одного кадра + последовательности облёта
├── tools/
│   └── wsl-profile.sh  # сборка + профилирование через perf в WSL
├── background.exr  # HDR-панорама фона, на который проецируются улетевшие лучи
└── CMakeLists.txt  # конфигурация сборки, подтягивает stb и tinyexr через FetchContent
```
<!-- tree:end -->
*Это дерево сгенерировано с помощью [readmetree](https://github.com/DarthBeltazar/readmetree)*