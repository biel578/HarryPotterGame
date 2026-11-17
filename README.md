# Harry Potter: A Busca pela Pedra Filosofal

Jogo 2D em C com raylib, inspirado no primeiro filme da saga (tema "Ode ao Cinema").

O jogador controla o Harry por 5 salas do castelo. Em cada sala é preciso coletar todos os itens para abrir a porta, desviando dos inimigos. Na última sala, o objetivo é pegar a Pedra Filosofal. O jogador começa com 3 vidas, e cada colisão com um inimigo tira uma.

## Estruturas de dados e algoritmos

- **Lista encadeada de salas:** cada sala aponta para a próxima, da primeira até a Câmara da Pedra Filosofal.
- **Listas encadeadas de itens e de inimigos:** cada sala guarda as suas, formando uma lista de listas.
- **Bubble Sort:** usado em dois pontos. Ordena o ranking em ordem decrescente e ordena os itens da sala por distância até o jogador, para mostrar o item mais próximo na tela.

## Requisitos

- Windows
- Compilador gcc (MinGW)
- Biblioteca raylib

O jeito mais simples de ter os dois é instalar o **raylib installer for Windows**, disponível em https://www.raylib.com, que já traz o compilador e a biblioteca.

## Como compilar

Abra um terminal na pasta do projeto e rode:

```
gcc jogo.c -o jogo.exe -lraylib -lopengl32 -lgdi32 -lwinmm
```

Se o compilador não encontrar a raylib, informe a pasta onde ela está instalada:

```
gcc jogo.c -o jogo.exe -I C:\raylib\raylib\src -L C:\raylib\raylib\src -lraylib -lopengl32 -lgdi32 -lwinmm
```

## Como executar

```
.\jogo.exe
```

## Controles

| Tecla | Ação |
|---|---|
| W A S D ou setas | Mover o Harry |
| ENTER | Iniciar o jogo e voltar ao menu |
| R | Ver o ranking (no menu) |

## Observações

- O arquivo `ranking.txt` é criado automaticamente na pasta do jogo quando uma partida termina.
- O jogo precisa ser executado a partir da pasta onde o `jogo.exe` está, para o ranking ser salvo e lido no lugar certo.