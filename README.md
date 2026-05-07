# Plane City — Night Flight

Um clone simples do Flappy Bird feito em C++ com [Raylib](https://www.raylib.com/).
Você controla um aviãozinho voando por um skyline noturno e precisa desviar dos prédios passando pelos vãos.

![cena](https://img.shields.io/badge/lang-C%2B%2B17-blue) ![lib](https://img.shields.io/badge/raylib-6.0-orange) ![platform](https://img.shields.io/badge/platform-Linux-lightgrey)

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

## Compilando e rodando

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
