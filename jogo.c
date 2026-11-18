/*
 * Estruturas que usei:
 *  - Lista encadeada de SALAS (lista principal)
 *  - Cada sala possui LISTAS encadeadas de ITENS e de INIMIGOS (lista de listas)
 *  - Struct Jogador (vidas, pontuacao, itens coletados)
 *  - Bubble Sort para o ranking (ordem decrescente), salvo em ranking.txt
 *  - Bubble Sort para ordenar os itens da sala por distancia ate o jogador
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


typedef enum {
    ESTADO_MENU,
    ESTADO_JOGANDO,
    ESTADO_VITORIA,
    ESTADO_DERROTA,
    ESTADO_RANKING
} EstadoJogo;


typedef struct Item {
    Vector2 pos;
    float raio;
    const char *nome;
    bool coletado;
    struct Item *prox;
} Item;


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
    Item *itens;                 
    Inimigo *inimigos;           
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
    float invencivel;            
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


static const Vector2 INICIO_JOGADOR = { 50.0f, 300.0f };
static const Rectangle PORTA = { 765.0f, 260.0f, 35.0f, 120.0f };



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
static float distanciaQuadrada(const Item *item, const Jogador *jogador);
static int ordenarItensPorDistancia(const Sala *sala, const Jogador *jogador, const Item *ordenados[]);
static void exibirItemMaisProximo(const Jogo *jogo);



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
    adicionarObstaculo(s2, 220, 200, 250, 30);
    adicionarObstaculo(s2, 450, 400, 700, 30);
    adicionarObstaculo(s2, 450, 200, 30, 200);
    adicionarInimigo(s2, 180, 100, 150.0f, false, 50, 200);
    adicionarInimigo(s2, 500, 100, 180.0f, false, 50, 200);
    adicionarInimigo(s2, 550, 450, 175.0f, true, 450, 760);
    adicionarInimigo(s2, 600, 500, 400.0f, true, 300, 760);
    adicionarInimigo(s2, 650, 550, 200.0f, true, 450, 760);

    Sala *s3 = criarSala(3, "Corredor do Cao de Tres Cabecas", (Color){ 60, 35, 35, 255 });
    adicionarItem(s3, 330, 130, "Flauta");
    adicionarItem(s3, 700, 520, "Pata de Coelho");
    adicionarObstaculo(s3, 220, 20, 40, 300);
    adicionarObstaculo(s3, 400, 200, 40, 550);
    adicionarObstaculo(s3, 580, 20, 40, 480);
    adicionarInimigo(s3, 320, 300, 250.0f, false, 70, 550);
    adicionarInimigo(s3, 500, 300, 350.0f, false, 70, 550);
    adicionarInimigo(s3, 320, 130, 450.0f, true, 270, 550);
    adicionarInimigo(s3, 450, 530, 400.0f, true, 440, 770);

    Sala *s4 = criarSala(4, "Sala do Xadrez de Bruxo", (Color){ 40, 45, 55, 255 });
    adicionarItem(s4, 176, 545, "Peao de Xadrez");
    adicionarItem(s4, 420, 100, "Chave Alada");
    adicionarItem(s4, 700, 340, "Rei Branco");
    adicionarObstaculo(s4, 150, 120, 50, 400);
    adicionarObstaculo(s4, 500, 50, 50, 200);
    adicionarObstaculo(s4, 500, 420, 50, 400);
    adicionarInimigo(s4, 120, 70, 397.0f, true, 100, 470);
    adicionarInimigo(s4, 120, 530, 405.0f, true, 100, 470);
    adicionarInimigo(s4, 250, 530, 405.0f, false, 60, 550);
    adicionarInimigo(s4, 380, 279, 405.0f, false, 60, 550);
    adicionarInimigo(s4, 210, 335, 405.0f, true, 200, 750);
    adicionarInimigo(s4, 470, 530, 452.0f, false, 100, 470);
    adicionarInimigo(s4, 555, 530, 471.0f, false, 100, 470);

    Sala *s5 = criarSala(5, "Camara da Pedra Filosofal", (Color){ 30, 30, 60, 255 });
    adicionarItem(s5, 752, 320, "Pedra Filosofal");
    adicionarObstaculo(s5, 95, 50, 40, 250);
    adicionarObstaculo(s5, 95, 400, 40, 350);
    adicionarObstaculo(s5, 690, 100, 40, 450);
    adicionarInimigo(s5, 140, 100, 120.0f, false, 70, 540);
    adicionarInimigo(s5, 180, 100, 300.0f, false, 70, 540);
    adicionarInimigo(s5, 220, 100, 203.0f, false, 70, 540);
    adicionarInimigo(s5, 260, 100, 332.0f, false, 70, 540);
    adicionarInimigo(s5, 300, 100, 503.0f, false, 70, 540);
    adicionarInimigo(s5, 340, 100, 100.0f, false, 70, 540);
    adicionarInimigo(s5, 380, 100, 345.0f, false, 70, 540);
    adicionarInimigo(s5, 420, 100, 200.0f, false, 70, 540);
    adicionarInimigo(s5, 460, 100, 150.0f, false, 70, 540);
    adicionarInimigo(s5, 500, 100, 50.0f, false, 70, 540);
    adicionarInimigo(s5, 540, 100, 250.0f, false, 70, 540);
    adicionarInimigo(s5, 580, 100, 500.0f, false, 70, 540);
    adicionarInimigo(s5, 620, 100, 50.0f, false, 70, 540);
    adicionarInimigo(s5, 660, 100, 390.0f, false, 70, 540);
    adicionarInimigo(s5, 150, 550, 50.0f, true, 150, 725);
    adicionarInimigo(s5, 150, 70, 50.0f, true, 150, 725);

    
    s1->prox = s2;
    s2->prox = s3;
    s3->prox = s4;
    s4->prox = s5;
    s5->prox = NULL;

    return s1;
}



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

    
    Rectangle teste = j->rect;
    teste.x += dx * j->velocidade * dt;
    if (teste.x < 0) teste.x = 0;
    if (teste.x + teste.width > LARGURA) teste.x = LARGURA - teste.width;
    if (!colideComObstaculos(jogo->atual, teste)) j->rect.x = teste.x;

    
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



void verificarResultado(Jogo *jogo)
{
    Jogador *j = &jogo->jogador;

    if (j->vidas <= 0) {
        jogo->estado = ESTADO_DERROTA;
        registrarPontuacao(jogo, j->pontuacao);
        return;
    }

    
    if (jogo->atual->prox == NULL && salaConcluida(jogo->atual)) {
        atualizarPontuacao(j, j->vidas * PONTOS_VIDA_FIM);
        jogo->estado = ESTADO_VITORIA;
        registrarPontuacao(jogo, j->pontuacao);
    }
}



static void exibirHUD(const Jogo *jogo)
{
    DrawRectangle(0, 0, LARGURA, HUD_ALTURA, (Color){ 20, 20, 20, 255 });
    DrawText(TextFormat("Vidas: %d", jogo->jogador.vidas), 15, 14, 22, RED);
    DrawText(TextFormat("Pontos: %d", jogo->jogador.pontuacao), 150, 14, 22, GOLD);
    DrawText(TextFormat("Itens: %d/%d", jogo->atual->itensColetados, jogo->atual->itensTotal),
             320, 14, 22, SKYBLUE);
    DrawText(TextFormat("Sala %d: %s", jogo->atual->id, jogo->atual->nome), 470, 16, 18, RAYWHITE);
}



#define MAX_ITENS_SALA 8


static float distanciaQuadrada(const Item *item, const Jogador *jogador)
{
    float dx = item->pos.x - (jogador->rect.x + jogador->rect.width / 2);
    float dy = item->pos.y - (jogador->rect.y + jogador->rect.height / 2);
    return dx * dx + dy * dy;
}


static int ordenarItensPorDistancia(const Sala *sala, const Jogador *jogador,
                                    const Item *ordenados[])
{
    int n = 0;
    for (const Item *it = sala->itens; it != NULL && n < MAX_ITENS_SALA; it = it->prox) {
        if (!it->coletado) ordenados[n++] = it;
    }

    for (int i = 0; i < n - 1; i++) {
        bool trocou = false;
        for (int j = 0; j < n - 1 - i; j++) {
            if (distanciaQuadrada(ordenados[j], jogador) >
                distanciaQuadrada(ordenados[j + 1], jogador)) {
                const Item *tmp = ordenados[j];
                ordenados[j] = ordenados[j + 1];
                ordenados[j + 1] = tmp;
                trocou = true;
            }
        }
        if (!trocou) break;
    }
    return n;
}

static void exibirItemMaisProximo(const Jogo *jogo)
{
    const Item *ordenados[MAX_ITENS_SALA];
    int n = ordenarItensPorDistancia(jogo->atual, &jogo->jogador, ordenados);
    if (n == 0) return;

    const Item *alvo = ordenados[0];
    Vector2 centro = { jogo->jogador.rect.x + jogo->jogador.rect.width / 2,
                       jogo->jogador.rect.y + jogo->jogador.rect.height / 2 };
    DrawLineV(centro, alvo->pos, Fade(RAYWHITE, 0.4f));
    DrawText(TextFormat("Item mais proximo: %s", alvo->nome), 15, ALTURA - 22, 18, RAYWHITE);
}

void exibirCenario(const Jogo *jogo)
{
    const Sala *sala = jogo->atual;
    const Jogador *j = &jogo->jogador;

    ClearBackground(sala->cor);

    
    for (int i = 0; i < sala->numObstaculos; i++) {
        DrawRectangleRec(sala->obstaculos[i], (Color){ 110, 110, 110, 255 });
        DrawRectangleLinesEx(sala->obstaculos[i], 2, DARKGRAY);
    }

    
    if (sala->prox != NULL) {
        Color corPorta = salaConcluida(sala) ? GREEN : MAROON;
        DrawRectangleRec(PORTA, corPorta);
        DrawText(salaConcluida(sala) ? "ABERTA" : "FECHADA", 700, 390, 14, RAYWHITE);
    }

    
    for (const Item *it = sala->itens; it != NULL; it = it->prox) {
        if (!it->coletado) {
            Color c = (sala->prox == NULL) ? RED : GOLD;   /* Pedra Filosofal em vermelho */
            DrawCircleV(it->pos, it->raio, c);
            DrawCircleLines((int)it->pos.x, (int)it->pos.y, it->raio + 3, RAYWHITE);
            DrawText(it->nome, (int)it->pos.x - MeasureText(it->nome, 12) / 2,
                     (int)it->pos.y + 20, 12, RAYWHITE);
        }
    }

    
    for (const Inimigo *in = sala->inimigos; in != NULL; in = in->prox) {
        DrawRectangleRec(in->rect, (Color){ 30, 120, 30, 255 });
        DrawRectangleLinesEx(in->rect, 2, BLACK);
        DrawCircle((int)(in->rect.x + 9), (int)(in->rect.y + 11), 3, RED);
        DrawCircle((int)(in->rect.x + 21), (int)(in->rect.y + 11), 3, RED);
    }

    
    bool visivel = (j->invencivel <= 0.0f) || (((int)(j->invencivel * 10.0f)) % 2 == 0);
    if (visivel) {
        DrawRectangleRec(j->rect, (Color){ 240, 200, 160, 255 });             /* tunica */
        DrawCircle((int)(j->rect.x + 15), (int)(j->rect.y + 9), 8, (Color){ 240, 200, 160, 255 }); /* rosto */
        DrawCircleLines((int)(j->rect.x + 11), (int)(j->rect.y + 9), 3, BLACK);  /* oculos */
        DrawCircleLines((int)(j->rect.x + 19), (int)(j->rect.y + 9), 3, BLACK);
    }

    exibirItemMaisProximo(jogo);
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

int main(void)
{
    InitWindow(LARGURA, ALTURA, "Harry Potter: A Busca pela Pedra Filosofal");
    SetTargetFPS(60);

    Jogo jogo = { 0 };
    jogo.estado = ESTADO_MENU;
    carregarRanking(&jogo);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        
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