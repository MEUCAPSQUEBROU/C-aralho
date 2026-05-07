# Plane City — Night Flight

Um clone simples do Flappy Bird feito em C++ com [Raylib](https://www.raylib.com/).
Você controla um aviãozinho voando por um skyline noturno e precisa desviar dos prédios passando pelos vãos.

![cena](https://img.shields.io/badge/lang-C%2B%2B17-blue) ![lib](https://img.shields.io/badge/raylib-6.0-orange) ![platform](https://img.shields.io/badge/platform-Linux%20%7C%20Windows-lightgrey)

## Controles

| Tecla / botão                     | Ação            |
| --------------------------------- | --------------- |
| `Espaço` / `↑` / clique do mouse  | Bater asa       |
| `Espaço` no menu / Game Over      | Iniciar / Reiniciar |
| Fechar a janela                   | Sair            |

## Pré-requisitos

- Linux (testado em Arch)
- `g++` com suporte a C++17
- `make`
- `raylib` ≥ 5.0 (instruções abaixo)

## Instalação das dependências

### Arch Linux / Manjaro

```bash
sudo pacman -Syu raylib
```

> O `-Syu` evita problemas de "partial upgrade" e mantém o sistema consistente.

### Ubuntu / Debian

A raylib não está no repositório padrão. Compile a partir do código:

```bash
sudo apt install build-essential git cmake libasound2-dev libx11-dev libxrandr-dev \
                 libxi-dev libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev libxinerama-dev
git clone https://github.com/raysan5/raylib.git
cd raylib/src
make PLATFORM=PLATFORM_DESKTOP
sudo make install
```

### Fedora

```bash
sudo dnf install raylib raylib-devel
```

### Windows (MSYS2 / MinGW-w64)

1. Instale o [MSYS2](https://www.msys2.org/) e abra o terminal **"MSYS2 MINGW64"**
   (não o "MSYS" comum — precisa ser o ambiente MINGW64).
2. Atualize e instale toolchain + raylib:

   ```bash
   pacman -Syu
   pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-raylib make git
   ```

3. Clone o repo e entre nele:

   ```bash
   git clone https://github.com/MEUCAPSQUEBROU/C-aralho.git
   cd C-aralho
   ```

4. Compile direto pelo `g++` (o `Makefile` deste projeto é específico de Linux):

   ```bash
   g++ -std=c++17 -O2 -Wall main.cpp -o plane.exe -lraylib -lopengl32 -lgdi32 -lwinmm
   ./plane.exe
   ```

> **Dica:** se o `g++` não for encontrado, confira que o terminal aberto é o
> *MINGW64* — o ícone tem o "64" verde. O ambiente "MSYS" puro não tem o
> compilador na PATH.

#### Windows com Visual Studio (alternativa)

Se preferir o MSVC, use o [`vcpkg`](https://vcpkg.io/):

```powershell
vcpkg install raylib:x64-windows
```

Depois crie um projeto C++ no Visual Studio, integre o `vcpkg`
(`vcpkg integrate install`) e adicione `main.cpp` como código-fonte.

## Compilando e rodando

### Linux

```bash
make        # gera o executável "plane"
make run    # compila (se preciso) e executa
make clean  # remove o executável
```

Ou manualmente, sem o Makefile:

```bash
g++ -std=c++17 -O2 -Wall main.cpp -o plane -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
./plane
```

### Windows (MSYS2 MINGW64)

O `Makefile` deste repo é Linux-only. Use o comando direto:

```bash
g++ -std=c++17 -O2 -Wall main.cpp -o plane.exe -lraylib -lopengl32 -lgdi32 -lwinmm
./plane.exe
```

## GPUs antigas (OpenGL < 3.3)

A `raylib` empacotada na maioria das distros é compilada exigindo OpenGL 3.3.
Se sua placa for antiga (ex.: Intel HD Graphics 1ª/2ª geração), o jogo vai
crashar com `GLXBadFBConfig`. Solução: forçar renderização por software via Mesa:

```bash
LIBGL_ALWAYS_SOFTWARE=1 ./plane
```

A 480×720 e 60 FPS qualquer CPU moderna roda tranquilo via `llvmpipe`.

## Ajustando a dificuldade

As constantes no topo de `main.cpp` controlam a "sensação" do jogo:

| Constante         | Padrão | Efeito                                              |
| ----------------- | ------ | --------------------------------------------------- |
| `GAP_HEIGHT`      | 180    | Vão entre prédios — maior = mais fácil              |
| `BUILDING_SPACE`  | 240    | Distância horizontal entre prédios                  |
| `BUILDING_WIDTH`  | 80     | Largura de cada prédio                              |
| `SCROLL_SPD`      | 180    | Velocidade do mundo (px/s)                          |
| `GRAVITY`         | 1400   | Quão rápido o avião cai                             |
| `FLAP_SPEED`      | -460   | Impulso da batida de asa (negativo = pra cima)      |

Recompile com `make` depois de alterar.

## Estrutura

```
.
├── main.cpp     # todo o jogo num arquivo só
├── Makefile     # build + run + clean
└── README.md
```

## Como funciona (rápido)

- **Game loop** padrão `while (!WindowShouldClose())` da raylib, a 60 FPS.
- **Avião** é um conjunto de triângulos rotacionados manualmente com `cosf`/`sinf`.
- **Prédios** são gerados proceduralmente: a cada um que sai da tela à esquerda,
  um novo entra à direita com posição de vão aleatória.
- **Janelas iluminadas** usam um hash determinístico baseado na posição do prédio,
  então o padrão é estável enquanto o prédio está visível.
- **Colisão** é AABB (`CheckCollisionRecs`) entre o retângulo do avião e os
  retângulos do topo/base do prédio.
- **Estados**: `Menu` → `Playing` → `GameOver` → `Playing` (ciclo).
