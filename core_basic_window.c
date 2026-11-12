/*
 * Harry Potter: A Busca pela Pedra Filosofal
 * Jogo 2D em C com raylib
 *
 * Estruturas:
 *  - Lista encadeada de SALAS (lista principal)
 *  - Cada sala possui LISTAS encadeadas de ITENS e de INIMIGOS (lista de listas)
 *  - Struct Jogador (vidas, pontuacao, itens coletados)
 *  - Bubble Sort para o ranking (ordem decrescente), salvo em ranking.txt
 *
 * Compilar (MinGW, raylib instalada em C:\raylib):
 *   gcc jogo.c -o jogo.exe -lraylib -lopengl32 -lgdi32 -lwinmm
 *   (se estiver usando o pacote w64devkit do raylib, ajuste -I e -L conforme sua instalacao)
 *
 * Controles: WASD ou setas para mover | ENTER para iniciar/voltar | R para ver ranking
 */

#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LARGURA          800
#define ALTURA           600
#define HUD_ALTURA       50
#define VIDAS_INICIAIS   3
#define MAX_RANKING      10
#define MAX_OBSTACULOS   4
#define ARQUIVO_RANKING  "ranking.txt"

#define PONTOS_ITEM      100
#define PONTOS_SALA      200
#define PONTOS_VIDA_FIM  500

/* ===================== ESTRUTURAS ===================== */

typedef enum {
    ESTADO_MENU,
    ESTADO_JOGANDO,
    ESTADO_VITORIA,
    ESTADO_DERROTA,
    ESTADO_RANKING
} EstadoJogo;

/* No da lista de itens de uma sala */
typedef struct Item {
    Vector2 pos;
    float raio;
    const char *nome;
    bool coletado;
    struct Item *prox;
} Item;

/* No da lista de inimigos de uma sala */
typedef struct Inimigo {
    Rectangle rect;
    float vel;
    bool horizontal;
    float min;
    float max;
    struct Inimigo *prox;
} Inimigo;

typedef struct Sala {
    int id;
    char nome[48];
    Color cor;
    Item *itens;                 /* lista de itens */
    Inimigo *inimigos;           /* lista de inimigos */
    Rectangle obstaculos[MAX_OBSTACULOS];
    int numObstaculos;
    int itensTotal;
    int itensColetados;
    struct Sala *prox; 
} Sala;

typedef struct {
    Rectangle rect;
    float velocidade;
    int vidas;
    int pontuacao;
    int itensColetados;
    float invencivel;            /* tempo restante de invencibilidade */
} Jogador;

typedef struct {
    Sala *inicio;
    Sala *atual;
    Jogador jogador;
    EstadoJogo estado;
    int ranking[MAX_RANKING + 1];
    int qtdRanking;
    int ultimaPontuacao;
} Jogo;

/* Posicao inicial do jogador e porta de saida */
static const Vector2 INICIO_JOGADOR = { 50.0f, 300.0f };
static const Rectangle PORTA = { 765.0f, 260.0f, 35.0f, 120.0f };

/* ===================== Funcoes ===================== */

void inicializarJogo(Jogo *jogo);
Sala *criarListaSalas(void);
void exibirCenario(const Jogo *jogo);
void movimentarJogador(Jogo *jogo, float dt);
void avancarSala(Jogo *jogo);
void coletarItem(Jogo *jogo);
void verificarColisao(Jogo *jogo);
void atualizarPontuacao(Jogador *jogador, int pontos);
void ordenarRanking(int v[], int n);
void verificarResultado(Jogo *jogo);
void liberarListaSalas(Sala *inicio);

/* Auxiliares */
static Sala *criarSala(int id, const char *nome, Color cor);
static void adicionarItem(Sala *sala, float x, float y, const char *nome);
static void adicionarInimigo(Sala *sala, float x, float y, float vel, bool horizontal, float min, float max);
static void adicionarObstaculo(Sala *sala, float x, float y, float w, float h);
static void atualizarInimigos(Jogo *jogo, float dt);
static bool colideComObstaculos(const Sala *sala, Rectangle r);
static void reposicionarJogador(Jogador *jogador);
static bool salaConcluida(const Sala *sala);
static void carregarRanking(Jogo *jogo);
static void salvarRanking(const Jogo *jogo);
static void registrarPontuacao(Jogo *jogo, int pontos);
static void exibirHUD(const Jogo *jogo);
static void exibirRanking(const Jogo *jogo);
static void exibirTelaTexto(const char *titulo, const char *linha1, const char *linha2, Color cor);

/* ===================== LISTAS: CRIACAO ===================== */

static Sala *criarSala(int id, const char *nome, Color cor)
{
    Sala *s = (Sala *)calloc(1, sizeof(Sala));
    if (s == NULL) {
        fprintf(stderr, "Erro: sem memoria para criar sala.\n");
        exit(1);
    }
    s->id = id;
    strncpy(s->nome, nome, sizeof(s->nome) - 1);
    s->cor = cor;
    return s;
}

/* Insere no INICIO da lista de itens da sala */
static void adicionarItem(Sala *sala, float x, float y, const char *nome)
{
    Item *novo = (Item *)calloc(1, sizeof(Item));
    if (novo == NULL) exit(1);
    novo->pos = (Vector2){ x, y };
    novo->raio = 14.0f;
    novo->nome = nome;
    novo->coletado = false;
    novo->prox = sala->itens;
    sala->itens = novo;
    sala->itensTotal++;
}

/* Insere no INICIO da lista de inimigos da sala */
static void adicionarInimigo(Sala *sala, float x, float y, float vel, bool horizontal, float min, float max)
{
    Inimigo *novo = (Inimigo *)calloc(1, sizeof(Inimigo));
    if (novo == NULL) exit(1);
    novo->rect = (Rectangle){ x, y, 30.0f, 30.0f };
    novo->vel = vel;
    novo->horizontal = horizontal;
    novo->min = min;
    novo->max = max;
    novo->prox = sala->inimigos;
    sala->inimigos = novo;
}

static void adicionarObstaculo(Sala *sala, float x, float y, float w, float h)
{
    if (sala->numObstaculos < MAX_OBSTACULOS) {
        sala->obstaculos[sala->numObstaculos++] = (Rectangle){ x, y, w, h };
    }
}

/* Cria a lista encadeada de salas e retorna o primeiro no */
Sala *criarListaSalas(void)
{
    Sala *s1 = criarSala(1, "Salao Principal", (Color){ 70, 50, 40, 255 });
    adicionarItem(s1, 200, 520, "Varinha");
    adicionarItem(s1, 650, 130, "Capa da Invisibilidade");
    adicionarObstaculo(s1, 300, 20, 40, 400);
    adicionarObstaculo(s1, 500, 350, 40, 250);
    adicionarInimigo(s1, 380, 450, 140.0f, false, 350, 560);
    adicionarInimigo(s1, 600, 250, 150.0f, true, 345, 750);
    

    Sala *s2 = criarSala(2, "Biblioteca", (Color){ 55, 40, 70, 255 });
    adicionarItem(s2, 150, 130, "Livro de Feiticos");
    adicionarItem(s2, 750, 500, "Pocao");
    adicionarItem(s2, 680, 120, "Pergaminho");
    adicionarObstaculo(s2, 220, 200, 200, 30);
    adicionarObstaculo(s2, 450, 400, 700, 30);
    adicionarInimigo(s2, 110, 100, 150.0f, false, 70, 400);
    adicionarInimigo(s2, 550, 450, 175.0f, true, 300, 720);
    adicionarInimigo(s2, 650, 550, 200.0f, true, 300, 720);

    Sala *s3 = criarSala(3, "Corredor do Cao de Tres Cabecas", (Color){ 60, 35, 35, 255 });
    adicionarItem(s3, 330, 130, "Flauta");
    adicionarItem(s3, 700, 520, "Pata de Coelho");
    adicionarObstaculo(s3, 220, 20, 40, 300);
    adicionarObstaculo(s3, 400, 200, 40, 550);
    adicionarObstaculo(s3, 580, 20, 40, 480);
    adicionarInimigo(s3, 320, 300, 250.0f, false, 70, 550);
    adicionarInimigo(s3, 500, 300, 350.0f, false, 70, 550);
    adicionarInimigo(s3, 320, 130, 250.0f, true, 270, 550);

    Sala *s4 = criarSala(4, "Sala do Xadrez de Bruxo", (Color){ 40, 45, 55, 255 });
    adicionarItem(s4, 180, 540, "Peao de Xadrez");
    adicionarItem(s4, 420, 100, "Chave Alada");
    adicionarItem(s4, 700, 300, "Rei Branco");
    adicionarObstaculo(s4, 300, 200, 150, 150);
    adicionarObstaculo(s4, 560, 420, 150, 100);
    adicionarInimigo(s4, 120, 150, 200.0f, true, 100, 700);
    adicionarInimigo(s4, 120, 450, 230.0f, true, 100, 700);

    Sala *s5 = criarSala(5, "Camara da Pedra Filosofal", (Color){ 30, 30, 60, 255 });
    adicionarItem(s5, 650, 320, "Pedra Filosofal");
    adicionarObstaculo(s5, 100, 20, 40, 400);
    adicionarObstaculo(s5, 100, 480, 40, 400);
    adicionarInimigo(s5, 450, 100, 220.0f, false, 70, 540);
    adicionarInimigo(s5, 550, 540, 240.0f, false, 70, 540);

    /* Encadeia as salas */
    s1->prox = s2;
    s2->prox = s3;
    s3->prox = s4;
    s4->prox = s5;
    s5->prox = NULL;

    return s1;
}

/* ===================== LISTAS: LIBERACAO ===================== */

void liberarListaSalas(Sala *inicio)
{
    while (inicio != NULL) {
        Sala *proxSala = inicio->prox;

        Item *it = inicio->itens;
        while (it != NULL) {
            Item *tmp = it->prox;
            free(it);
            it = tmp;
        }

        Inimigo *in = inicio->inimigos;
        while (in != NULL) {
            Inimigo *tmp = in->prox;
            free(in);
            in = tmp;
        }

        free(inicio);
        inicio = proxSala;
    }
}

/* ===================== LOGICA DO JOGO ===================== */

static void reposicionarJogador(Jogador *jogador)
{
    jogador->rect.x = INICIO_JOGADOR.x;
    jogador->rect.y = INICIO_JOGADOR.y;
}

static bool salaConcluida(const Sala *sala)
{
    return sala->itensColetados >= sala->itensTotal;
}

void inicializarJogo(Jogo *jogo)
{
    if (jogo->inicio != NULL) {
        liberarListaSalas(jogo->inicio);
        jogo->inicio = NULL;
    }

    jogo->inicio = criarListaSalas();
    jogo->atual = jogo->inicio;

    jogo->jogador.rect = (Rectangle){ INICIO_JOGADOR.x, INICIO_JOGADOR.y, 30.0f, 30.0f };
    jogo->jogador.velocidade = 220.0f;
    jogo->jogador.vidas = VIDAS_INICIAIS;
    jogo->jogador.pontuacao = 0;
    jogo->jogador.itensColetados = 0;
    jogo->jogador.invencivel = 0.0f;
}

static bool colideComObstaculos(const Sala *sala, Rectangle r)
{
    for (int i = 0; i < sala->numObstaculos; i++) {
        if (CheckCollisionRecs(r, sala->obstaculos[i])) return true;
    }
    return false;
}

void movimentarJogador(Jogo *jogo, float dt)
{
    Jogador *j = &jogo->jogador;
    float dx = 0.0f, dy = 0.0f;

    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) dx += 1.0f;
    if (IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A)) dx -= 1.0f;
    if (IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S)) dy += 1.0f;
    if (IsKeyDown(KEY_UP)    || IsKeyDown(KEY_W)) dy -= 1.0f;

    /* Eixo X */
    Rectangle teste = j->rect;
    teste.x += dx * j->velocidade * dt;
    if (teste.x < 0) teste.x = 0;
    if (teste.x + teste.width > LARGURA) teste.x = LARGURA - teste.width;
    if (!colideComObstaculos(jogo->atual, teste)) j->rect.x = teste.x;

    /* Eixo Y */
    teste = j->rect;
    teste.y += dy * j->velocidade * dt;
    if (teste.y < HUD_ALTURA) teste.y = HUD_ALTURA;
    if (teste.y + teste.height > ALTURA) teste.y = ALTURA - teste.height;
    if (!colideComObstaculos(jogo->atual, teste)) j->rect.y = teste.y;
}

static void atualizarInimigos(Jogo *jogo, float dt)
{
    for (Inimigo *in = jogo->atual->inimigos; in != NULL; in = in->prox) {
        if (in->horizontal) {
            in->rect.x += in->vel * dt;
            if (in->rect.x < in->min) { in->rect.x = in->min; in->vel = -in->vel; }
            if (in->rect.x > in->max) { in->rect.x = in->max; in->vel = -in->vel; }
        } else {
            in->rect.y += in->vel * dt;
            if (in->rect.y < in->min) { in->rect.y = in->min; in->vel = -in->vel; }
            if (in->rect.y > in->max) { in->rect.y = in->max; in->vel = -in->vel; }
        }
    }
}

void atualizarPontuacao(Jogador *jogador, int pontos)
{
    jogador->pontuacao += pontos;
    if (jogador->pontuacao < 0) jogador->pontuacao = 0;
}

void coletarItem(Jogo *jogo)
{
    Sala *sala = jogo->atual;
    for (Item *it = sala->itens; it != NULL; it = it->prox) {
        if (!it->coletado && CheckCollisionCircleRec(it->pos, it->raio, jogo->jogador.rect)) {
            it->coletado = true;
            sala->itensColetados++;
            jogo->jogador.itensColetados++;
            atualizarPontuacao(&jogo->jogador, PONTOS_ITEM);
        }
    }
}

void verificarColisao(Jogo *jogo)
{
    Jogador *j = &jogo->jogador;
    if (j->invencivel > 0.0f) return;

    for (Inimigo *in = jogo->atual->inimigos; in != NULL; in = in->prox) {
        if (CheckCollisionRecs(j->rect, in->rect)) {
            j->vidas--;
            j->invencivel = 1.5f;
            reposicionarJogador(j);
            break;
        }
    }
}

void avancarSala(Jogo *jogo)
{
    Sala *sala = jogo->atual;
    if (sala->prox != NULL && salaConcluida(sala) &&
        CheckCollisionRecs(jogo->jogador.rect, PORTA)) {
        jogo->atual = sala->prox;
        reposicionarJogador(&jogo->jogador);
        atualizarPontuacao(&jogo->jogador, PONTOS_SALA);
    }
}

/* ===================== RANKING ===================== */

/* Ordena em ordem DECRESCENTE */
void ordenarRanking(int v[], int n)
{
    for (int i = 0; i < n - 1; i++) {
        bool trocou = false;
        for (int j = 0; j < n - 1 - i; j++) {
            if (v[j] < v[j + 1]) {
                int tmp = v[j];
                v[j] = v[j + 1];
                v[j + 1] = tmp;
                trocou = true;
            }
        }
        if (!trocou) break;
    }
}

static void carregarRanking(Jogo *jogo)
{
    jogo->qtdRanking = 0;
    FILE *f = fopen(ARQUIVO_RANKING, "r");
    if (f == NULL) return;

    int valor;
    while (jogo->qtdRanking < MAX_RANKING && fscanf(f, "%d", &valor) == 1) {
        jogo->ranking[jogo->qtdRanking++] = valor;
    }
    fclose(f);
    ordenarRanking(jogo->ranking, jogo->qtdRanking);
}

static void salvarRanking(const Jogo *jogo)
{
    FILE *f = fopen(ARQUIVO_RANKING, "w");
    if (f == NULL) return;
    for (int i = 0; i < jogo->qtdRanking; i++) {
        fprintf(f, "%d\n", jogo->ranking[i]);
    }
    fclose(f);
}

static void registrarPontuacao(Jogo *jogo, int pontos)
{
    jogo->ultimaPontuacao = pontos;
    jogo->ranking[jogo->qtdRanking++] = pontos;   /* vetor tem MAX_RANKING + 1 posicoes */
    ordenarRanking(jogo->ranking, jogo->qtdRanking);
    if (jogo->qtdRanking > MAX_RANKING) jogo->qtdRanking = MAX_RANKING;
    salvarRanking(jogo);
}

/* ===================== RESULTADO ===================== */

void verificarResultado(Jogo *jogo)
{
    Jogador *j = &jogo->jogador;

    if (j->vidas <= 0) {
        jogo->estado = ESTADO_DERROTA;
        registrarPontuacao(jogo, j->pontuacao);
        return;
    }

    /* Vitoria: sala final concluida (Pedra Filosofal coletada) */
    if (jogo->atual->prox == NULL && salaConcluida(jogo->atual)) {
        atualizarPontuacao(j, j->vidas * PONTOS_VIDA_FIM);
        jogo->estado = ESTADO_VITORIA;
        registrarPontuacao(jogo, j->pontuacao);
    }
}

/* ===================== DESENHO ===================== */

static void exibirHUD(const Jogo *jogo)
{
    DrawRectangle(0, 0, LARGURA, HUD_ALTURA, (Color){ 20, 20, 20, 255 });
    DrawText(TextFormat("Vidas: %d", jogo->jogador.vidas), 15, 14, 22, RED);
    DrawText(TextFormat("Pontos: %d", jogo->jogador.pontuacao), 150, 14, 22, GOLD);
    DrawText(TextFormat("Itens: %d/%d", jogo->atual->itensColetados, jogo->atual->itensTotal),
             320, 14, 22, SKYBLUE);
    DrawText(TextFormat("Sala %d: %s", jogo->atual->id, jogo->atual->nome), 470, 16, 18, RAYWHITE);
}

void exibirCenario(const Jogo *jogo)
{
    const Sala *sala = jogo->atual;
    const Jogador *j = &jogo->jogador;

    ClearBackground(sala->cor);

    /* Obstaculos */
    for (int i = 0; i < sala->numObstaculos; i++) {
        DrawRectangleRec(sala->obstaculos[i], (Color){ 110, 110, 110, 255 });
        DrawRectangleLinesEx(sala->obstaculos[i], 2, DARKGRAY);
    }

    /* Porta (ou saida final) */
    if (sala->prox != NULL) {
        Color corPorta = salaConcluida(sala) ? GREEN : MAROON;
        DrawRectangleRec(PORTA, corPorta);
        DrawText(salaConcluida(sala) ? "ABERTA" : "FECHADA", 700, 390, 14, RAYWHITE);
    }

    /* Itens */
    for (const Item *it = sala->itens; it != NULL; it = it->prox) {
        if (!it->coletado) {
            Color c = (sala->prox == NULL) ? RED : GOLD;   /* Pedra Filosofal em vermelho */
            DrawCircleV(it->pos, it->raio, c);
            DrawCircleLines((int)it->pos.x, (int)it->pos.y, it->raio + 3, RAYWHITE);
            DrawText(it->nome, (int)it->pos.x - MeasureText(it->nome, 12) / 2,
                     (int)it->pos.y + 20, 12, RAYWHITE);
        }
    }

    /* Inimigos */
    for (const Inimigo *in = sala->inimigos; in != NULL; in = in->prox) {
        DrawRectangleRec(in->rect, (Color){ 30, 120, 30, 255 });
        DrawRectangleLinesEx(in->rect, 2, BLACK);
        DrawCircle((int)(in->rect.x + 9), (int)(in->rect.y + 11), 3, RED);
        DrawCircle((int)(in->rect.x + 21), (int)(in->rect.y + 11), 3, RED);
    }

    /* Jogador (Harry): pisca durante a invencibilidade */
    bool visivel = (j->invencivel <= 0.0f) || (((int)(j->invencivel * 10.0f)) % 2 == 0);
    if (visivel) {
        DrawRectangleRec(j->rect, (Color){ 120, 20, 20, 255 });             /* tunica */
        DrawCircle((int)(j->rect.x + 15), (int)(j->rect.y + 9), 8, (Color){ 240, 200, 160, 255 }); /* rosto */
        DrawCircleLines((int)(j->rect.x + 11), (int)(j->rect.y + 9), 3, BLACK);  /* oculos */
        DrawCircleLines((int)(j->rect.x + 19), (int)(j->rect.y + 9), 3, BLACK);
    }

    exibirHUD(jogo);
}

static void exibirRanking(const Jogo *jogo)
{
    ClearBackground((Color){ 25, 20, 40, 255 });
    DrawText("RANKING", LARGURA / 2 - MeasureText("RANKING", 40) / 2, 60, 40, GOLD);

    if (jogo->qtdRanking == 0) {
        DrawText("Nenhuma pontuacao registrada.", 250, 250, 22, RAYWHITE);
    }
    for (int i = 0; i < jogo->qtdRanking; i++) {
        DrawText(TextFormat("%2d.  %d pontos", i + 1, jogo->ranking[i]), 280, 130 + i * 35, 26,
                 (i == 0) ? GOLD : RAYWHITE);
    }
    DrawText("ENTER - voltar ao menu", 290, 550, 20, LIGHTGRAY);
}

static void exibirTelaTexto(const char *titulo, const char *linha1, const char *linha2, Color cor)
{
    ClearBackground((Color){ 20, 15, 35, 255 });
    DrawText(titulo, LARGURA / 2 - MeasureText(titulo, 40) / 2, 150, 40, cor);
    DrawText(linha1, LARGURA / 2 - MeasureText(linha1, 24) / 2, 260, 24, RAYWHITE);
    DrawText(linha2, LARGURA / 2 - MeasureText(linha2, 20) / 2, 340, 20, LIGHTGRAY);
}

/* ===================== MAIN ===================== */

int main(void)
{
    InitWindow(LARGURA, ALTURA, "Harry Potter: A Busca pela Pedra Filosofal");
    SetTargetFPS(60);

    Jogo jogo = { 0 };
    jogo.estado = ESTADO_MENU;
    carregarRanking(&jogo);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        /* ---------- ATUALIZACAO ---------- */
        switch (jogo.estado) {
            case ESTADO_MENU:
                if (IsKeyPressed(KEY_ENTER)) {
                    inicializarJogo(&jogo);
                    jogo.estado = ESTADO_JOGANDO;
                } else if (IsKeyPressed(KEY_R)) {
                    jogo.estado = ESTADO_RANKING;
                }
                break;

            case ESTADO_JOGANDO:
                if (jogo.jogador.invencivel > 0.0f) jogo.jogador.invencivel -= dt;
                movimentarJogador(&jogo, dt);
                atualizarInimigos(&jogo, dt);
                coletarItem(&jogo);
                verificarColisao(&jogo);
                avancarSala(&jogo);
                verificarResultado(&jogo);
                break;

            case ESTADO_VITORIA:
            case ESTADO_DERROTA:
            case ESTADO_RANKING:
                if (IsKeyPressed(KEY_ENTER)) jogo.estado = ESTADO_MENU;
                break;
        }

        /* ---------- DESENHO ---------- */
        BeginDrawing();
        switch (jogo.estado) {
            case ESTADO_MENU:
                exibirTelaTexto("HARRY POTTER",
                                "A Busca pela Pedra Filosofal",
                                "ENTER - Jogar     R - Ranking",
                                GOLD);
                break;

            case ESTADO_JOGANDO:
                exibirCenario(&jogo);
                break;

            case ESTADO_VITORIA:
                exibirTelaTexto("VITORIA!",
                                TextFormat("Voce encontrou a Pedra Filosofal! Pontos: %d", jogo.ultimaPontuacao),
                                "ENTER - voltar ao menu",
                                GREEN);
                break;

            case ESTADO_DERROTA:
                exibirTelaTexto("FIM DE JOGO",
                                TextFormat("Suas vidas acabaram. Pontos: %d", jogo.ultimaPontuacao),
                                "ENTER - voltar ao menu",
                                RED);
                break;

            case ESTADO_RANKING:
                exibirRanking(&jogo);
                break;
        }
        EndDrawing();
    }

    liberarListaSalas(jogo.inicio);
    CloseWindow();
    return 0;
}